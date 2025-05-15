// /*
//  * This file is open source software, licensed to you under the terms
//  * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
//  * distributed with this work for additional information regarding copyright
//  * ownership.  You may not use this file except in compliance with the License.
//  *
//  * You may obtain a copy of the License at
//  *
//  *   http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing,
//  * software distributed under the License is distributed on an
//  * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  * KIND, either express or implied.  See the License for the
//  * specific language governing permissions and limitations
//  * under the License.
//  */
// /*
//  * Copyright (C) 2014 Cloudius Systems, Ltd.
//  */

// #ifndef MEMORY_HH_
// #define MEMORY_HH_

// #include "../resource/resource.hh"
// #include <new>
// #include <functional>
// #include <vector>
// #include <cassert>
















// namespace memory {

// /// \cond internal
// // TODO: Use getpagesize() in order to learn a size of a system PAGE.
// static constexpr size_t page_bits = 12;
// static constexpr size_t page_size = 1 << page_bits;       // 4K
// static constexpr size_t huge_page_size = 512 * page_size; // 2M

// void configure(std::vector<resource::memory> m,
//         std::experimental::optional<std::string> hugetlbfs_path = {});

// void enable_abort_on_allocation_failure();

// class disable_abort_on_alloc_failure_temporarily {
// public:
//     disable_abort_on_alloc_failure_temporarily();
//     ~disable_abort_on_alloc_failure_temporarily() noexcept;
// };

// void set_heap_profiling_enabled(bool);

// enum class reclaiming_result {
//     reclaimed_nothing,
//     reclaimed_something
// };

// enum class reclaimer_scope {
//     async,
//     sync
// };

// class reclaimer {
// public:
//     using reclaim_fn = std::function<reclaiming_result ()>;
// private:
//     reclaim_fn _reclaim;
//     reclaimer_scope _scope;
// public:
//     reclaimer(reclaim_fn reclaim, reclaimer_scope scope = reclaimer_scope::async);
//     ~reclaimer();
//     reclaiming_result do_reclaim() { return _reclaim(); }
//     reclaimer_scope scope() const { return _scope; }
// };

// bool drain_cross_cpu_freelist();

// void set_reclaim_hook(
//         std::function<void (std::function<void ()>)> hook);

// using physical_address = uint64_t;

// struct translation {
//     translation() = default;
//     translation(physical_address a, size_t s) : addr(a), size(s) {}
//     physical_address addr = 0;
//     size_t size = 0;
// };

// translation translate(const void* addr, size_t size);
// class statistics;
// statistics stats();

// /// Memory allocation statistics.
// class statistics {
//     uint64_t _mallocs;
//     uint64_t _frees;
//     uint64_t _cross_cpu_frees;
//     size_t _total_memory;
//     size_t _free_memory;
//     uint64_t _reclaims;
// private:
//     statistics(uint64_t mallocs, uint64_t frees, uint64_t cross_cpu_frees,
//             uint64_t total_memory, uint64_t free_memory, uint64_t reclaims)
//         : _mallocs(mallocs), _frees(frees), _cross_cpu_frees(cross_cpu_frees)
//         , _total_memory(total_memory), _free_memory(free_memory), _reclaims(reclaims) {}
// public:
//     uint64_t mallocs() const { return _mallocs; }
//     uint64_t frees() const { return _frees; }
//     uint64_t cross_cpu_frees() const { return _cross_cpu_frees; }
//     /// Total number of objects which were allocated but not freed.
//     size_t live_objects() const { return mallocs() - frees(); }
//     size_t free_memory() const { return _free_memory; }
//     /// Total allocated memory (in bytes)
//     size_t allocated_memory() const { return _total_memory - _free_memory; }
//     /// Total memory (in bytes)
//     size_t total_memory() const { return _total_memory; }
//     /// Number of reclaims performed due to low memory
//     uint64_t reclaims() const { return _reclaims; }
//     friend statistics stats();
// };

// struct memory_layout {
//     uintptr_t start;
//     uintptr_t end;
// };

// // Discover virtual address range used by the allocator on current shard.
// // Supported only when seastar allocator is enabled.
// memory_layout get_memory_layout();

// /// Returns the value of free memory low water mark in bytes.
// /// When free memory is below this value, reclaimers are invoked until it goes above again.
// size_t min_free_memory();

// /// Sets the value of free memory low water mark in memory::page_size units.
// void set_min_free_pages(size_t pages);
// // Memory allocation functions
//     void* allocate(size_t size);
//     void* allocate_aligned(size_t align, size_t size);
//     void* allocate_large(size_t size);
//     void* allocate_large_aligned(size_t align, size_t size);
//     void free(void* ptr);
//     void free(void* ptr, size_t size);
//     void free_large(void* ptr);
//     size_t object_size(void* ptr);
//     void shrink(void* ptr, size_t new_size);

// }

// class with_alignment {
//     size_t _align;
// public:
//     with_alignment(size_t align) : _align(align) {}
//     size_t alignment() const { return _align; }
// };

// void* operator new(size_t size, with_alignment wa);
// void* operator new[](size_t size, with_alignment wa);
// void operator delete(void* ptr, with_alignment wa);
// void operator delete[](void* ptr, with_alignment wa);

// #endif /* MEMORY_HH_ */
