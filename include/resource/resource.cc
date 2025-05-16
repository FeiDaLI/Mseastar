#include "resource.hh"
#include "../util/align.hh"
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sched.h>
#include <cassert>
#include <unordered_map>
#include <boost/range/irange.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/lexical_cast.hpp>
#include <regex>
void validate(boost::any& v,
              const std::vector<std::string>& values,
              cpuset_bpo_wrapper* target_type, int) {
    using namespace boost::program_options;
    static std::regex r("(\\d+-)?(\\d+)(,(\\d+-)?(\\d+))*");
    validators::check_first_occurrence(v);
    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    auto&& s = validators::get_single_string(values);
    std::smatch match;
    if (std::regex_match(s, match, r)) {
        std::vector<std::string> ranges;
        boost::split(ranges, s, boost::is_any_of(","));
        cpuset_bpo_wrapper ret;
        for (auto&& range: ranges) {
            std::string beg = range;
            std::string end = range;
            auto dash = range.find('-');
            if (dash != range.npos) {
                beg = range.substr(0, dash);
                end = range.substr(dash + 1);
            }
            auto b = boost::lexical_cast<unsigned>(beg);
            auto e = boost::lexical_cast<unsigned>(end);
            if (b > e) {
                throw validation_error(validation_error::invalid_option_value);
            }
            for (auto i = b; i <= e; ++i) {
                ret.value.insert(i);
            }
        }
        v = std::move(ret);
    } else {
        throw validation_error(validation_error::invalid_option_value);
    }
}



// Utility function to read from proc filesystem
std::string read_file_content(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    
    std::string content;
    std::getline(file, content);
    return content;
}

cpu_set_t cpuid_to_cpuset(unsigned cpuid) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpuid, &cs);
    return cs;
}

namespace resource {

// Helper function to detect available CPUs
std::vector<unsigned> get_available_cpus() {
    std::vector<unsigned> available_cpus;
    
    // Try to read from /proc/cpuinfo
    try {
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("processor") == 0) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    std::string cpu_id_str = line.substr(pos + 1);
                    unsigned cpu_id = std::stoi(cpu_id_str);
                    available_cpus.push_back(cpu_id);
                }
            }
        }
    } catch (...) {
        // Fallback to sysconf
        int nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        for (int i = 0; i < nprocs; i++) {
            available_cpus.push_back(i);
        }
    }
    
    return available_cpus;
}

// Helper function to get machine memory
size_t get_machine_memory() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        // Fallback to sysconf
        return ::sysconf(_SC_PAGESIZE) * size_t(::sysconf(_SC_PHYS_PAGES));
    }
    
    return info.totalram * info.mem_unit;
}

// Replacement for distribute_objects
std::vector<unsigned> distribute_cpus(unsigned num_cpus) {
    auto available_cpus = get_available_cpus();
    unsigned requested = std::min(num_cpus, (unsigned)available_cpus.size());
    
    std::vector<unsigned> result;
    for (unsigned i = 0; i < requested; i++) {
        result.push_back(available_cpus[i]);
    }
    
    return result;
}

// IO queue allocation function that doesn't depend on hwloc
static io_queue_topology allocate_io_queues(configuration c, std::vector<cpu> cpus) {
    unsigned num_io_queues = c.io_queues.value_or(cpus.size());
    unsigned max_io_requests = c.max_io_requests.value_or(128 * num_io_queues);
    
    // Simplified node mapping - assuming single node for virtualized environment
    auto node_of_shard = [&cpus](unsigned shard) {
        return 0; // All CPUs in same node (0)
    };
    
    std::unordered_map<int, std::set<unsigned>> numa_nodes;
    for (auto shard: boost::irange(0, int(cpus.size()))) {
        auto node_id = node_of_shard(shard);
        if (numa_nodes.count(node_id) == 0) {
            numa_nodes.emplace(node_id, std::set<unsigned>());
        }
        numa_nodes.at(node_id).insert(shard);
    }
    
    io_queue_topology ret;
    ret.shard_to_coordinator.resize(cpus.size());
    
    // Adjust if num_io_queues > cpus.size()
    if (num_io_queues > cpus.size()) {
        std::cout << "Warning: number of IO queues (" << num_io_queues << ") greater than logical cores (" 
                  << cpus.size() << "). Adjusting downwards.\n";
        num_io_queues = cpus.size();
    }
    
    auto find_shard = [&cpus](unsigned cpu_id) {
        auto idx = 0u;
        for (auto& c: cpus) {
            if (c.cpu_id == cpu_id) {
                return idx;
            }
            idx++;
        }
        assert(0);
        return 0u;
    };
    
    // Distribute io queues
    std::vector<unsigned> distributed_cpus = distribute_cpus(num_io_queues);
    std::unordered_map<int, std::vector<unsigned>> node_coordinators;
    
    for (unsigned i = 0; i < num_io_queues; i++) {
        auto io_coordinator = find_shard(distributed_cpus[i]);
        ret.coordinators.emplace_back(io_queue{io_coordinator, std::max(max_io_requests / num_io_queues, 1u)});
        ret.shard_to_coordinator[io_coordinator] = io_coordinator;
        
        auto node_id = node_of_shard(io_coordinator);
        if (node_coordinators.count(node_id) == 0) {
            node_coordinators.emplace(node_id, std::vector<unsigned>());
        }
        node_coordinators.at(node_id).push_back(io_coordinator);
        numa_nodes[node_id].erase(io_coordinator);
    }
    
    // Assign remaining processors to existing coordinators within the same NUMA node
    for (auto& node: numa_nodes) {
        auto cid_idx = 0;
        for (auto& remaining_shard: node.second) {
            auto idx = cid_idx++ % node_coordinators.at(node.first).size();
            auto io_coordinator = node_coordinators.at(node.first)[idx];
            ret.shard_to_coordinator[remaining_shard] = io_coordinator;
        }
    }
    
    return ret;
}

void print_io_queue(io_queue_topology& io_queues) {
    for (auto& coordinator : io_queues.coordinators) {
        std::cout << "coordinator" << coordinator.id << " " << coordinator.capacity << std::endl;
    }
    for(auto& shard_to_coordinator : io_queues.shard_to_coordinator){
        std::cout << "shard_to_coordinator" << shard_to_coordinator << std::endl;
    }
}

void print_resource(resources& res) {
    std::cout << "res.cpus.size():" << res.cpus.size() << std::endl;
    for (auto& cpu : res.cpus) {
        std::cout << "cpu.cpu_id:" << cpu.cpu_id << std::endl;
        for(auto &mem : cpu.mem){
            std::cout << "mem.size:" << mem.bytes << " mem.node_id:" << mem.nodeid << std::endl;
        }
    }
}

size_t calculate_memory(configuration c, size_t available_memory, float panic_factor) {
    size_t default_reserve_memory = std::max<size_t>(1536 * 1024 * 1024, 0.07 * available_memory) * panic_factor;
    auto reserve = c.reserve_memory.value_or(default_reserve_memory);
    size_t min_memory = 500'000'000;
    std::cout << "ca::Available memory: " << available_memory << std::endl;
    std::cout << "ca::Default reserve memory: " << default_reserve_memory << std::endl;
    std::cout << "ca::Reserve memory: " << reserve << std::endl;
    std::cout << "ca::Minimum memory: " << min_memory << std::endl;
    if (available_memory >= reserve + min_memory) {
        available_memory -= reserve;
        std::cout << "ca::Adjusted available memory after reserve: " << available_memory << std::endl;
    } else {
        // Allow starting up even in low memory configurations (e.g. 2GB boot2docker VM)
        available_memory = min_memory;
        std::cout << "ca::Setting available memory to minimum: " << available_memory << std::endl;
    }

    size_t mem = c.total_memory.value_or(available_memory);
    std::cout << "Requested memory: " << mem << std::endl;

    if (mem > available_memory) {
        throw std::runtime_error("insufficient physical memory");
    }
    return mem;
}



resources allocate(configuration c) {
    // Get available memory and CPUs without using hwloc
    auto available_memory = get_machine_memory();
    auto available_cpus = get_available_cpus();
    unsigned available_procs = available_cpus.size();
    
    size_t mem = calculate_memory(c, available_memory);
    unsigned procs = c.cpus.value_or(available_procs);
    
    if (procs > available_procs) {
        throw std::runtime_error("insufficient processing units");
    }
    
    auto mem_per_proc = align_down(mem/procs, 2 << 20);
    
    std::cout << "Available memory: " << available_memory << std::endl;
    std::cout << "Requested memory: " << mem << std::endl;
    std::cout << "Available processing units: " << available_procs << std::endl;
    std::cout << "Requested processing units: " << procs << std::endl;
    std::cout << "Memory per processing unit: " << mem_per_proc << std::endl;
    
    resources ret;
    
    // Process CPU restrictions if specified
    std::vector<unsigned> cpu_ids;
    if (c.cpu_set) {
        std::cout << "分配CPU_SET\n";
        for (auto idx : *c.cpu_set) {
            if (idx < available_procs) {
                cpu_ids.push_back(idx);
            }
        }
        
        // If no valid CPUs were specified, fall back to all available
        if (cpu_ids.empty()) {
            for (unsigned i = 0; i < procs && i < available_procs; i++) {
                cpu_ids.push_back(available_cpus[i]);
            }
        }
    } else {
        // Use first 'procs' number of CPUs
        for (unsigned i = 0; i < procs && i < available_procs; i++) {
            cpu_ids.push_back(available_cpus[i]);
        }
    }
    
    // Assign memory to CPUs - assuming single NUMA node in virtualized environment
    for (unsigned i = 0; i < cpu_ids.size(); i++) {
        cpu this_cpu;
        this_cpu.cpu_id = cpu_ids[i];
        
        // Assign memory to this CPU - all from node 0 since we're assuming single node
        this_cpu.mem.push_back({mem_per_proc, 0});
        
        ret.cpus.push_back(std::move(this_cpu));
    }
    
    // Allocate IO queues
    ret.io_queues = allocate_io_queues(c, ret.cpus);
    
    print_io_queue(ret.io_queues);
    print_resource(ret);
    
    return ret;
}

unsigned nr_processing_units() {
    return ::sysconf(_SC_NPROCESSORS_ONLN);
}

} // namespace resource
