#pragma once
#ifndef MEMORY_HH
#define MEMORY_HH

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <new> 
#include <string>
#include <thread>
#include "../resource/resource.hh"
#include "../util/backtrace.hh"
#include "file_desc.hh"

/*
    mmap_area起始地址是char[],长度在mmap_deleter中保存.
    思考：为什么unique_ptr第一个参数是char[]，而不是char *，或者void *，或者char__
*/
mmap_area mmap_anonymous(void* addr, size_t length, int prot, int flags);
namespace memory {
    static constexpr size_t page_bits = 12;
    static constexpr size_t page_size = 1 << page_bits;       // 4K
    static constexpr size_t huge_page_size = 512 * page_size; // 2M
    void configure(std::vector<resource::memory> m, std::optional<std::string> hugetlbfs_path = {});
    void enable_abort_on_allocation_failure();
    class disable_abort_on_alloc_failure_temporarily {
        public:
            disable_abort_on_alloc_failure_temporarily();
            ~disable_abort_on_alloc_failure_temporarily() noexcept;
    };
    void set_heap_profiling_enabled(bool);
enum class reclaiming_result {
    reclaimed_nothing,
    reclaimed_something
};

enum class reclaimer_scope {
    async,
    sync
};

class reclaimer {
public:
    using reclaim_fn = std::function<reclaiming_result ()>;
private:
    reclaim_fn _reclaim;
    reclaimer_scope _scope;
public:
    reclaimer(reclaim_fn reclaim, reclaimer_scope scope = reclaimer_scope::async);
    ~reclaimer();
    reclaiming_result do_reclaim() { return _reclaim(); }
    reclaimer_scope scope() const { return _scope; }
};
bool drain_cross_cpu_freelist();
void set_reclaim_hook(std::function<void (std::function<void ()>)> hook);
using physical_address = uint64_t;
struct translation {
    translation() = default;
    translation(physical_address a, size_t s) : addr(a), size(s) {}
    physical_address addr = 0;
    size_t size = 0;
};
translation translate(const void* addr, size_t size);
class statistics;
statistics stats();

/// Memory allocation statistics.
class statistics {
    uint64_t _mallocs;
    uint64_t _frees;
    uint64_t _cross_cpu_frees;
    size_t _total_memory;
    size_t _free_memory;
    uint64_t _reclaims;
private:
    statistics(uint64_t mallocs, uint64_t frees, uint64_t cross_cpu_frees,
            uint64_t total_memory, uint64_t free_memory, uint64_t reclaims)
        : _mallocs(mallocs), _frees(frees), _cross_cpu_frees(cross_cpu_frees)
        , _total_memory(total_memory), _free_memory(free_memory), _reclaims(reclaims) {}
public:
    uint64_t mallocs() const { return _mallocs; }
    uint64_t frees() const { return _frees; }
    uint64_t cross_cpu_frees() const { return _cross_cpu_frees; }
    /// Total number of objects which were allocated but not freed.
    size_t live_objects() const { return mallocs() - frees(); }
    size_t free_memory() const { return _free_memory; }
    /// Total allocated memory (in bytes)
    size_t allocated_memory() const { return _total_memory - _free_memory; }
    /// Total memory (in bytes)
    size_t total_memory() const { return _total_memory; }
    /// Number of reclaims performed due to low memory
    uint64_t reclaims() const { return _reclaims; }
    friend statistics stats();
};

struct memory_layout {
    uintptr_t start;
    uintptr_t end;
};

// Discover virtual address range used by the allocator on current shard.
// Supported only when seastar allocator is enabled.
memory_layout get_memory_layout();
/// Returns the value of free memory low water mark in bytes.
/// When free memory is below this value, reclaimers are invoked until it goes above again.
size_t min_free_memory();

/// Sets the value of free memory low water mark in memory::page_size units.
void set_min_free_pages(size_t pages);
// Memory allocation functions
void* allocate(size_t size);
void* allocate_aligned(size_t align, size_t size);
void* allocate_large(size_t size);
void* allocate_large_aligned(size_t align, size_t size);
void free(void* ptr);
void free(void* ptr, size_t size);
void free_large(void* ptr);
size_t object_size(void* ptr);
void shrink(void* ptr, size_t new_size);

}


struct allocation_site {
    mutable size_t count = 0; // number of live objects allocated at backtrace.
    mutable size_t size = 0; // amount of bytes in live objects allocated at backtrace.
    mutable const allocation_site* next = nullptr;
    saved_backtrace backtrace;
    bool operator==(const allocation_site& o) const {
        return backtrace == o.backtrace;
    }
    bool operator!=(const allocation_site& o) const {
        return !(*this == o);
    }
};

using allocation_site_ptr = const allocation_site*;





namespace std {

template<> struct hash<::allocation_site> {
    size_t operator()(const ::allocation_site& bi) const {
        return std::hash<saved_backtrace>()(bi.backtrace);
    }
};
}




namespace memory {

static allocation_site_ptr get_allocation_site() __attribute__((unused));

static std::atomic<bool> abort_on_allocation_failure{false};


static constexpr unsigned cpu_id_shift = 36; // FIXME: make dynamic
static constexpr unsigned max_cpus = 256;
static constexpr size_t cache_line_size = 64;
using pageidx = uint32_t;
struct page;
class page_list;
static std::atomic<bool> live_cpus[max_cpus];
static thread_local uint64_t g_allocs;
static thread_local uint64_t g_frees;
static thread_local uint64_t g_cross_cpu_frees;
static thread_local uint64_t g_reclaims;
#include <functional>  // for std::function
#include <optional>    // for std::optional
#include <memory>      // for std::unique_ptr
using allocate_system_memory_fn = std::function<mmap_area(std::optional<void*> where, size_t how_much)>;

namespace bi = boost::intrusive;

inline
unsigned object_cpu_id(const void* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) >> cpu_id_shift) & 0xff;
}

class page_list_link {
    uint32_t _prev;
    uint32_t _next;
    friend class page_list;
};

static char* mem_base() {
    static char* known;
    static std::once_flag flag;
    std::call_once(flag, [] {
        size_t alloc = size_t(1) << 44;
        auto r = ::mmap(NULL, 2 * alloc,
                    PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                    -1, 0);
        if (r == MAP_FAILED) {
            abort();
        }
        ::madvise(r, 2 * alloc, MADV_DONTDUMP);
        auto cr = reinterpret_cast<char*>(r);
        known = align_up(cr, alloc);
        ::munmap(cr, known - cr);
        ::munmap(known + alloc, cr + 2 * alloc - (known + alloc));
    });
    return known;
}

constexpr bool is_page_aligned(size_t size) {
    return (size & (page_size - 1)) == 0;
}

constexpr size_t next_page_aligned(size_t size) {
    return (size + (page_size - 1)) & ~(page_size - 1);
}

class small_pool;

struct free_object {
    free_object* next;
};

struct page {
    bool free;
    uint8_t offset_in_span;
    uint16_t nr_small_alloc;
    uint32_t span_size; // in pages, if we're the head or the tail
    page_list_link link;
    small_pool* pool;  // if used in a small_pool
    free_object* freelist;
};

class page_list {
    uint32_t _front = 0;
    uint32_t _back = 0;
public:
    page& front(page* ary) { return ary[_front]; }
    page& back(page* ary) { return ary[_back]; }
    bool empty() const { return !_front; }
    void erase(page* ary, page& span) {
        if (span.link._next) {
            ary[span.link._next].link._prev = span.link._prev;
        } else {
            _back = span.link._prev;
        }
        if (span.link._prev) {
            ary[span.link._prev].link._next = span.link._next;
        } else {
            _front = span.link._next;
        }
    }
    void push_front(page* ary, page& span) {
        auto idx = &span - ary;
        if (_front) {
            ary[_front].link._prev = idx;
        } else {
            _back = idx;
        }
        span.link._next = _front;
        span.link._prev = 0;
        _front = idx;
    }
    void pop_front(page* ary) {
        if (ary[_front].link._next) {
            ary[ary[_front].link._next].link._prev = 0;
        } else {
            _back = 0;
        }
        _front = ary[_front].link._next;
    }
    page* find(uint32_t n_pages, page* ary) {
        auto n = _front;
        while (n && ary[n].span_size < n_pages) {
            n = ary[n].link._next;
        }
        if (!n) {
            return nullptr;
        }
        return &ary[n];
    }
};

class small_pool {
    unsigned _object_size;
    unsigned _span_size;
    free_object* _free = nullptr;
    size_t _free_count = 0;
    unsigned _min_free;
    unsigned _max_free;
    unsigned _spans_in_use = 0;
    page_list _span_list;
    static constexpr unsigned idx_frac_bits = 2;
private:
    size_t span_bytes() const { return _span_size * page_size; }
public:
    explicit small_pool(unsigned object_size) noexcept;
    ~small_pool();
    void* allocate();
    void deallocate(void* object);
    unsigned object_size() const { return _object_size; }
    bool objects_page_aligned() const { return is_page_aligned(_object_size); }
    static constexpr unsigned size_to_idx(unsigned size);
    static constexpr unsigned idx_to_size(unsigned idx);
    allocation_site_ptr& alloc_site_holder(void* ptr);
private:
    void add_more_objects();
    void trim_free_list();
    float waste();
};

// index 0b0001'1100 -> size (1 << 4) + 0b11 << (4 - 2)

constexpr unsigned 
small_pool::idx_to_size(unsigned idx) {
    return (((1 << idx_frac_bits) | (idx & ((1 << idx_frac_bits) - 1)))
              << (idx >> idx_frac_bits))
                  >> idx_frac_bits;
}

constexpr unsigned small_pool::size_to_idx(unsigned size) {
    return ((log2floor(size) << idx_frac_bits) - ((1 << idx_frac_bits) - 1))
            + ((size - 1) >> (log2floor(size) - idx_frac_bits));
}

class small_pool_array {
public:
    static constexpr unsigned nr_small_pools = small_pool::size_to_idx(4 * page_size) + 1;
private:
    union u {
        small_pool a[nr_small_pools];
        u() {
            for (unsigned i = 0; i < nr_small_pools; ++i) {
                new (&a[i]) small_pool(small_pool::idx_to_size(i));
            }
        }
        ~u() {
            // cannot really call destructor, since other
            // objects may be freed after we are gone.
        }
    } _u;
public:
    small_pool& operator[](unsigned idx) { return _u.a[idx]; }
};

static constexpr size_t max_small_allocation
    = small_pool::idx_to_size(small_pool_array::nr_small_pools - 1);

constexpr size_t object_size_with_alloc_site(size_t size) {
    return size;
}

struct cross_cpu_free_item {
    cross_cpu_free_item* next;
};

struct cpu_pages {
    uint32_t min_free_pages = 20000000 / page_size;
    char* memory;
    page* pages;
    uint32_t nr_pages;
    uint32_t nr_free_pages;
    uint32_t current_min_free_pages = 0;
    unsigned cpu_id = -1U;
    std::function<void (std::function<void ()>)> reclaim_hook;
    std::vector<reclaimer*> reclaimers;
    static constexpr unsigned nr_span_lists = 32;
    union pla {
        pla() {
            for (auto&& e : free_spans) {
                // std::cout<<"开始初始化free_spans"<<std::endl;
                new (&e) page_list;
            }
        }
        ~pla() {
            // no destructor -- might be freeing after we die
        }
        page_list free_spans[nr_span_lists];  // contains spans with span_size >= 2^idx
    } fsu;
    small_pool_array small_pools;
    alignas(cache_line_size) std::atomic<cross_cpu_free_item*> xcpu_freelist;
    alignas(cache_line_size) std::vector<physical_address> virt_to_phys_map;
    static std::atomic<unsigned> cpu_id_gen;
    static cpu_pages* all_cpus[max_cpus];
    union asu {
        using alloc_sites_type = std::unordered_set<allocation_site>;
        asu() {
            new (&alloc_sites) alloc_sites_type();
        }
        ~asu() {} // alloc_sites live forever
        alloc_sites_type alloc_sites;
    } asu;
    allocation_site_ptr alloc_site_list_head = nullptr; // For easy traversal of asu.alloc_sites from scylla-gdb.py
    bool collect_backtrace = false;
    char* mem() { return memory; }

    void link(page_list& list, page* span);
    void unlink(page_list& list, page* span);
    struct trim {
        unsigned offset;
        unsigned nr_pages; // 这个是什么意思?
    };
    void maybe_reclaim();
    template <typename Trimmer>
    void* allocate_large_and_trim(unsigned nr_pages, Trimmer trimmer);
    void* allocate_large(unsigned nr_pages);
    void* allocate_large_aligned(unsigned align_pages, unsigned nr_pages);
    page* find_and_unlink_span(unsigned nr_pages);
    page* find_and_unlink_span_reclaiming(unsigned n_pages);
    void free_large(void* ptr);
    void free_span(pageidx start, uint32_t nr_pages);
    void free_span_no_merge(pageidx start, uint32_t nr_pages);
    void* allocate_small(unsigned size);
    void free(void* ptr);
    void free(void* ptr, size_t size);
    bool try_cross_cpu_free(void* ptr);
    void shrink(void* ptr, size_t new_size);
    void free_cross_cpu(unsigned cpu_id, void* ptr);
    bool drain_cross_cpu_freelist();
    size_t object_size(void* ptr);
    page* to_page(void* p) {
        return &pages[(reinterpret_cast<char*>(p) - mem()) / page_size];
    }

    bool is_initialized() const;
    bool initialize();
    reclaiming_result run_reclaimers(reclaimer_scope);
    void schedule_reclaim();
    void set_reclaim_hook(std::function<void (std::function<void ()>)> hook);
    void set_min_free_pages(size_t pages);
    void resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
    void do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
    void replace_memory_backing(allocate_system_memory_fn alloc_sys_mem);
    void init_virt_to_phys_map();
    memory::memory_layout memory_layout();
    translation translate(const void* addr, size_t size);
    ~cpu_pages();
};

static thread_local cpu_pages cpu_mem;



//Free spans are store in the largest index i such that nr_pages >= 1 << i.
static inline
unsigned index_of(unsigned pages) {
    return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages) - 1;
}

// Smallest index i such that all spans stored in the index are >= pages.
static inline
unsigned index_of_conservative(unsigned pages) {
    if (pages == 1) {
        return 0;
    }
    return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages - 1);
}
/*
这个函数的作用是：给定一个页面数量 pages，找到最小的 i，使得 2^i >= pages，
从而定位到第一个可能满足请求的 free_span 列表。
*/


struct disable_backtrace_temporarily {
    ~disable_backtrace_temporarily() {}
};



static
saved_backtrace get_backtrace() noexcept {
    disable_backtrace_temporarily dbt;
    return current_backtrace();
}


static allocation_site_ptr get_allocation_site();
mmap_area allocate_hugetlbfs_memory(file_desc& fd, std::optional<void*> where, size_t how_much);
void abort_on_underflow(size_t size);
void* allocate_large(size_t size);
static void on_allocation_failure(size_t size);
void* allocate_large_aligned(size_t align, size_t size);
void free_large(void* ptr);
size_t object_size(void* ptr);
void* allocate(size_t size);
void* allocate_aligned(size_t align, size_t size);
void free(void* obj);
void free(void* obj, size_t size);
void shrink(void* obj, size_t new_size);
void set_reclaim_hook(std::function<void (std::function<void ()>)> hook);

mmap_area allocate_anonymous_memory(std::optional<void*> where, size_t how_much);

void configure(std::vector<resource::memory> m, std::optional<std::string> hugetlbfs_path);







}

class with_alignment {
    size_t _align;
public:
    with_alignment(size_t align) : _align(align) {}
    size_t alignment() const { return _align; }
};


void* operator new(size_t size, with_alignment wa);
void* operator new[](size_t size, with_alignment wa);
void operator delete(void* ptr, with_alignment wa);
void operator delete[](void* ptr, with_alignment wa);



#endif