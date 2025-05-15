/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 */

#ifndef RESOURCE_HH_
#define RESOURCE_HH_

#include <cstdlib>
#include <string>
#include <iostream>
#include <optional>
#include <vector>
#include <set>
#include <sched.h>
#include <boost/any.hpp>

cpu_set_t cpuid_to_cpuset(unsigned cpuid);

namespace resource {
using cpuset = std::set<unsigned>; 
struct configuration {
    std::optional<size_t> total_memory;
    std::optional<size_t> reserve_memory;  // if total_memory not specified
    std::optional<size_t> cpus;
    std::optional<cpuset> cpu_set;
    std::optional<unsigned> max_io_requests;
    std::optional<unsigned> io_queues;
};
struct memory {
    size_t bytes;
    unsigned nodeid;
};
struct io_queue {
    unsigned id;
    unsigned capacity;
};
// Since this is static information, we will keep a copy at each CPU.
// This will allow us to easily find who is the IO coordinator for a given
// node without a trip to a remote CPU.
struct io_queue_topology {
    std::vector<unsigned> shard_to_coordinator;
    std::vector<io_queue> coordinators;
};
/*
coordinator0 128
coordinator1 128
coordinator2 128
coordinator3 128
coordinator4 128
coordinator5 128
coordinator6 128
coordinator7 128
coordinator8 128
coordinator9 128
coordinator10 128
coordinator11 128
shard_to_coordinator0
shard_to_coordinator1
shard_to_coordinator2
shard_to_coordinator3
shard_to_coordinator4
shard_to_coordinator5
shard_to_coordinator6
shard_to_coordinator7
shard_to_coordinator8
shard_to_coordinator9
shard_to_coordinator10
shard_to_coordinator11
*/

struct cpu {
    unsigned cpu_id;
    std::vector<memory> mem;
};
/*
这里mem的size为1，所以struct cpu可以化简为
cpu_id。
bytes。
node_id(0)。
memory中的node_id一一直为0。
因为只有服务器只有一个NUMA节点。
*/

struct resources {
    std::vector<cpu> cpus;
    io_queue_topology io_queues;
};
// Calculate the amount of memory to use based on the configuration
size_t calculate_memory(configuration c, size_t available_memory, float panic_factor = 1);//这个panic_factor是什么意思?
resources allocate(configuration c);
unsigned nr_processing_units();
}

// We need a wrapper class, because boost::program_options wants validate()
// (below) to be in the same namespace as the type it is validating.
struct cpuset_bpo_wrapper {
    resource::cpuset value;
};

// Overload for boost program options parsing/validation
extern
void validate(boost::any& v,
              const std::vector<std::string>& values,
              cpuset_bpo_wrapper* target_type, int);
#endif /* RESOURCE_HH_ */
