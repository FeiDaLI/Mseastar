// #include "future_all11.hh"
//
// void server_socket::abort_accept() {
//     _ssi->abort_accept();
// }
//
// socket_address::socket_address(ipv4_addr addr)
//     : socket_address(make_ipv4_address(addr))
// {}
//
//
// bool socket_address::operator==(const socket_address& a) const {
//     // TODO: handle ipv6
//     return std::tie(u.in.sin_family, u.in.sin_port, u.in.sin_addr.s_addr)
//                     == std::tie(a.u.in.sin_family, a.u.in.sin_port,
//                                     a.u.in.sin_addr.s_addr);
// }
//
//
// server_socket
// posix_network_stack::listen(socket_address sa, listen_options opt) {
//     if (opt.proto == transport::TCP) {
//         return _reuseport ?
//             server_socket(std::make_unique<posix_reuseport_server_tcp_socket_impl>(sa, engine().posix_listen(sa, opt)))
//             :
//             server_socket(std::make_unique<posix_server_tcp_socket_impl>(sa, engine().posix_listen(sa, opt)));
//     } else {
//         return _reuseport ?
//             server_socket(std::make_unique<posix_reuseport_server_sctp_socket_impl>(sa, engine().posix_listen(sa, opt)))
//             :
//             server_socket(std::make_unique<posix_server_sctp_socket_impl>(sa, engine().posix_listen(sa, opt)));
//     }
// }
//
// net::socket posix_network_stack::socket() {
//     return net::socket(std::make_unique<posix_socket_impl>());
// }
//
//
// server_socket
// posix_ap_network_stack::listen(socket_address sa, listen_options opt) {
//     if (opt.proto == transport::TCP) {
//         return _reuseport ?
//             server_socket(std::make_unique<posix_reuseport_server_tcp_socket_impl>(sa, engine().posix_listen(sa, opt)))
//             :
//             server_socket(std::make_unique<posix_tcp_ap_server_socket_impl>(sa));
//     } else {
//         return _reuseport ?
//             server_socket(std::make_unique<posix_reuseport_server_sctp_socket_impl>(sa, engine().posix_listen(sa, opt)))
//             :
//             server_socket(std::make_unique<posix_sctp_ap_server_socket_impl>(sa));
//     }
// }
//
//
// future<> posix_udp_channel::send(ipv4_addr dst, const char *message) {
//     auto len = strlen(message);
//     return _fd->sendto(make_ipv4_address(dst), message, len)
//             .then([len] (size_t size) { assert(size == len); });
// }
//
// future<> posix_udp_channel::send(ipv4_addr dst, packet p) {
//     auto len = p.len();
//     _send.prepare(dst, std::move(p));
//     return _fd->sendmsg(&_send._hdr)
//             .then([len] (size_t size) { assert(size == len); });
// }
//
// udp_channel
// posix_network_stack::make_udp_channel(ipv4_addr addr) {
//     return udp_channel(std::make_unique<posix_udp_channel>(addr));
// }
//
// io_queue::io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology)
//         : _coordinator(coordinator)
//         , _capacity(capacity)
//         , _io_topology(std::move(topology))
//         , _priority_classes()
//         , _fq(capacity) {
// }
//
// io_queue::~io_queue() {
//     // It is illegal to stop the I/O queue with pending requests.
//     // Technically we would use a gate to guarantee that. But here, it is not
//     // needed since this is expected to be destroyed only after the reactor is destroyed.
//     //
//     // And that will happen only when there are no more fibers to run. If we ever change
//     // that, then this has to change.
//     for (auto&& pclasses: _priority_classes) {
//         _fq.unregister_priority_class(pclasses.second->ptr);
//     }
// }
//
//
// void io_queue::fill_shares_array() {
//     for (unsigned i = 0; i < _max_classes; ++i) {
//         _registered_shares[i].store(0);
//     }
// }
//
// io_priority_class io_queue::register_one_priority_class(std::string name, uint32_t shares) {
//     for (unsigned i = 0; i < _max_classes; ++i) {
//         uint32_t unused = 0;
//         auto s = _registered_shares[i].compare_exchange_strong(unused, shares, std::memory_order_acq_rel);
//         if (s) {
//             io_priority_class p;
//             _registered_names[i] = name;
//             p.val = i;
//             return std::move(p);
//         };
//     }
//     throw std::runtime_error("No more room for new I/O priority classes");
// }
//
// // seastar::metrics::label io_queue_shard("ioshard");
//
// io_queue::priority_class_data::priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner)
//     : ptr(ptr)
//     , bytes(0)
//     , ops(0)
//     , nr_queued(0)
//     , queue_time(1s){}
//
// io_queue::priority_class_data& io_queue::find_or_create_class(const io_priority_class& pc, shard_id owner) {
//     auto it_pclass = _priority_classes.find(pc.id());
//     if (it_pclass == _priority_classes.end()) {
//         auto shares = _registered_shares.at(pc.id()).load(std::memory_order_acquire);
//         auto name = _registered_names.at(pc.id());
//         auto ret = _priority_classes.emplace(pc.id(), make_lw_shared<priority_class_data>(name, _fq.register_priority_class(shares), owner));
//         it_pclass = ret.first;
//     }
//     return *(it_pclass->second);
// }
//
//
//
//
//
//
//
//
//
//
// bool drain_cross_cpu_freelist() {
//     return cpu_mem.drain_cross_cpu_freelist();
// }
//
//
// void smp::join_all()
// {
//
//     for (auto&& t: smp::_threads) {
//         t.join();
//     }
// }
//
//
// void* posix_thread::start_routine(void* arg) noexcept {
//     auto pfunc = reinterpret_cast<std::function<void ()>*>(arg);
//     (*pfunc)();
//     return nullptr;
// }
//
// posix_thread::posix_thread(std::function<void ()> func)
//     : posix_thread(attr{}, std::move(func)) {
// }
//
// posix_thread::posix_thread(attr a, std::function<void ()> func)
//     : _func(std::make_unique<std::function<void ()>>(std::move(func))) {
//     pthread_attr_t pa;
//     auto r = pthread_attr_init(&pa);
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
//     auto stack_size = a._stack_size.size;
//     if (!stack_size) {
//         stack_size = 2 << 20;
//     }
//     // allocate guard area as well
//     _stack = mmap_anonymous(nullptr, stack_size + (4 << 20),
//         PROT_NONE, MAP_PRIVATE | MAP_NORESERVE);
//     auto stack_start = align_up(_stack.get() + 1, 2 << 20);
//     mmap_area real_stack = mmap_anonymous(stack_start, stack_size,
//         PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_STACK);
//     real_stack.release(); // protected by @_stack
//     ::madvise(stack_start, stack_size, MADV_HUGEPAGE);
//     r = pthread_attr_setstack(&pa, stack_start, stack_size);
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
//     r = pthread_create(&_pthread, &pa,
//                 &posix_thread::start_routine, _func.get());
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
// }
//
// posix_thread::posix_thread(posix_thread&& x)
//     : _func(std::move(x._func)), _pthread(x._pthread), _valid(x._valid)
//     , _stack(std::move(x._stack)) {
//     x._valid = false;
// }
//
// posix_thread::~posix_thread() {
//     assert(!_valid);
// }
//
// void posix_thread::join() {
//     assert(_valid);
//     pthread_join(_pthread, NULL);
//     _valid = false;
// }
//
// void smp::start_all_queues()
// {
//     for (unsigned c = 0; c < count; c++) {
//         if (c != engine().cpu_id()) {
//             _qs[c][engine().cpu_id()].start(c);
//         }
//     }
// }
//
//
// boost::program_options::options_description
// smp::get_options_description()
// {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("SMP options");
//     opts.add_options()
//         ("smp,c", bpo::value<unsigned>(), "number of threads (default: one per CPU)")
//         ("cpuset", bpo::value<cpuset_bpo_wrapper>(), "CPUs to use (in cpuset(7) format; default: all))")
//         ("memory,m", bpo::value<std::string>(), "memory to use, in bytes (ex: 4G) (default: all)")
//         ("reserve-memory", bpo::value<std::string>(), "memory reserved to OS (if --memory not specified)")
//         ("hugepages", bpo::value<std::string>(), "path to accessible hugetlbfs mount (typically /dev/hugepages/something)")
//         ("lock-memory", bpo::value<bool>(), "lock all memory (prevents swapping)")
//         ("thread-affinity", bpo::value<bool>()->default_value(true), "pin threads to their cpus (disable for overprovisioning)")
//         ("num-io-queues", bpo::value<unsigned>(), "Number of IO queues. Each IO unit will be responsible for a fraction of the IO requests. Defaults to the number of threads")
//         ("max-io-requests", bpo::value<unsigned>(), "Maximum amount of concurrent requests to be sent to the disk. Defaults to 128 times the number of IO queues")
//         ;
//     return opts;
// }
//
// void smp::configure(boost::program_options::variables_map configuration)
// {
//     // 初始化信号集，屏蔽所有信号
//     sigset_t sigs;
//     sigfillset(&sigs);
//     for (auto sig : {SIGHUP, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV,
//             SIGALRM, SIGCONT, SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU}) {
//         sigdelset(&sigs, sig);  // 从信号集中移除特定信号
//     }
//     pthread_sigmask(SIG_BLOCK, &sigs, nullptr);  // 设置线程信号掩码
//
//
//     // 安装一次性信号处理器
//     install_oneshot_signal_handler<SIGSEGV, sigsegv_action>();
//     install_oneshot_signal_handler<SIGABRT, sigabrt_action>();
//     std::cout<<"设置thread affinity"<<std::endl;
//     // 获取配置中的线程亲和性设置
//     auto thread_affinity = configuration["thread-affinity"].as<bool>(); //这里出错
//     std::cout<<"thread affinity end"<<std::endl;
//     if (configuration.count("overprovisioned")
//            && configuration["thread-affinity"].defaulted()) {
//         thread_affinity = false;  // 如果过载且未显式设置，则关闭线程亲和性
//     }
//     if (!thread_affinity && _using_dpdk) {
//         printf("警告: 在 DPDK 模式下忽略 --thread-affinity 0\n");
//     }
//
//     // 初始化 SMP（对称多处理）相关参数
//     smp::count = 1;  // 默认 CPU 数量为 1
//     smp::_tmain = std::this_thread::get_id();  // 主线程 ID
//     auto nr_cpus = resource::nr_processing_units();  // 获取可用的 CPU 数量
//     resource::cpuset cpu_set;  // CPU 集合
//     for (unsigned i = 0; i < nr_cpus; ++i) {
//         cpu_set.insert(i);  // 将所有 CPU 添加到集合中
//     }
//
//     // 根据配置覆盖 CPU 集合和数量
//     if (configuration.count("cpuset")) {
//         cpu_set = configuration["cpuset"].as<cpuset_bpo_wrapper>().value;
//     }
//     if (configuration.count("smp")) {
//         nr_cpus = configuration["smp"].as<unsigned>();
//     } else {
//         nr_cpus = cpu_set.size();
//     }
//     smp::count = nr_cpus;  // 更新 CPU 数量
//     _reactors.resize(nr_cpus);  // 调整反应器数组大小
//     // 配置资源分配
//     resource::configuration rc;
//     if (configuration.count("memory")) {
//         //没有走到这行
//         std::cout<<"配置中含有memory"<<std::endl;
//         rc.total_memory = parse_memory_size(configuration["memory"].as<std::string>());
//     }
//     if (configuration.count("reserve-memory")) {
//         std::cout<<"配置中含有reserve memory"<<std::endl;
//         rc.reserve_memory = parse_memory_size(configuration["reserve-memory"].as<std::string>());
//     }
//     // 处理大页内存路径和内存锁定
//     std::optional<std::string> hugepages_path;
//     if (configuration.count("hugepages")) {
//         std::cout<<"配置中含有hugepages"<<std::endl;
//         hugepages_path = configuration["hugepages"].as<std::string>();
//     }
//     auto mlock = false;
//     if (configuration.count("lock-memory")) {
//         std::cout<<"配置中含有lock memory"<<std::endl;
//         mlock = configuration["lock-memory"].as<bool>();
//     }
//     if (mlock) {
//         std::cout<<"lock memory"<<std::endl;
//         auto r = mlockall(MCL_CURRENT | MCL_FUTURE);  // 锁定内存
//         if (r) {
//             printf("警告: mlockall 失败\n");
//         }
//     }
//     // 配置资源分配参数
//     rc.cpus = smp::count;//12
//     rc.cpu_set = std::move(cpu_set);
//     if (configuration.count("max-io-requests")) {
//         std::cout<<"配置中含有max io requests"<<std::endl;
//         rc.max_io_requests = configuration["max-io-requests"].as<unsigned>();
//     }
//     if (configuration.count("num-io-queues")) {
//         std::cout<<"配置中含有num io queues"<<std::endl;
//         rc.io_queues = configuration["num-io-queues"].as<unsigned>();
//     }
//     // 分配资源并初始化 CPU 和内存
//     auto resources = resource::allocate(rc);
//     std::vector<resource::cpu> allocations = std::move(resources.cpus);//allocations是CPU的vector.
// /*
// allocations format:
// [cpu][cpu]...[cpu]
// or
// [cpuid(0~11),bytes,nodeid(0)]
// */
//
//     if (thread_affinity) {
//         std::cout<<"thread pind to 0"<<std::endl;
//         smp::pin(allocations[0].cpu_id);  // 绑定主线程到CPU 0
//     }
//     std::cout<<"mem config begin "<<std::endl;
//     // std::cout<<"hugepages_path is "<<hugepages_path<<std::endl;
//     //这里报错
//     memory::configure(allocations[0].mem, hugepages_path);//hugepages_path是空的.
//
//     std::cout<<"mem config end "<<std::endl;
//     // 启用或禁用内存分配失败时的终止行为
//     if (configuration.count("abort-on-seastar-bad-alloc")) {
//         memory::enable_abort_on_allocation_failure();
//     }
//     // 启用堆内存分析
//     bool heapprof_enabled = configuration.count("heapprof");
//     memory::set_heap_profiling_enabled(heapprof_enabled);
//     // 创建同步屏障
//     static boost::barrier reactors_registered(smp::count);
//     static boost::barrier smp_queues_constructed(smp::count);
//     static boost::barrier inited(smp::count);
//
//     // 初始化 IO 队列信息
//     auto io_info = std::move(resources.io_queues);
//     std::vector<io_queue*> all_io_queues;
//     all_io_queues.resize(io_info.coordinators.size());
//     io_queue::fill_shares_array();
//
//     // 分配 IO 队列
//     auto alloc_io_queue = [io_info, &all_io_queues] (unsigned shard) {
//         auto cid = io_info.shard_to_coordinator[shard];
//         int vec_idx = 0;
//         for (auto& coordinator: io_info.coordinators) {
//             if (coordinator.id != cid) {
//                 vec_idx++;
//                 continue;
//             }
//             if (shard == cid) {
//                 all_io_queues[vec_idx] = new io_queue(coordinator.id, coordinator.capacity, io_info.shard_to_coordinator);
//             }
//             return vec_idx;
//         }
//         assert(0); // 不可能到达这里
//     };
//     // 分配 IO 队列给线程
//     auto assign_io_queue = [&all_io_queues] (shard_id id, int queue_idx) {
//         if (all_io_queues[queue_idx]->coordinator() == id) {
//             engine().my_io_queue.reset(all_io_queues[queue_idx]);
//         }
//         engine()._io_queue = all_io_queues[queue_idx];
//         engine()._io_coordinator = all_io_queues[queue_idx]->coordinator();
//     };
//
//     _all_event_loops_done.emplace(smp::count);
//
//     // 创建额外的线程来运行反应器
//     unsigned i;
//     for (i = 1; i < smp::count; i++) {
//         auto allocation = allocations[i];
//         create_thread([configuration, hugepages_path, i, allocation, assign_io_queue, alloc_io_queue, thread_affinity, heapprof_enabled] {
//             std::cout<<"create thread "<<i<<std::endl;
//             auto thread_name = format("reactor-{}", i);
//             pthread_setname_np(pthread_self(), thread_name.c_str());  // 设置线程名称
//             if (thread_affinity) {
//                 smp::pin(allocation.cpu_id);  // 绑定线程到指定 CPU
//             }
//             memory::configure(allocation.mem, hugepages_path);
//             memory::set_heap_profiling_enabled(heapprof_enabled);
//             sigset_t mask;
//             sigfillset(&mask);
//             for (auto sig : { SIGSEGV }) {
//                 sigdelset(&mask, sig);  // 移除特定信号
//             }
//             auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
//             throw_pthread_error(r);
//             allocate_reactor(i);
//             _reactors[i] = &engine();
//             auto queue_idx = alloc_io_queue(i);
//             reactors_registered.wait();
//             smp_queues_constructed.wait();
//             start_all_queues();
//             assign_io_queue(i, queue_idx);
//             inited.wait();
//             engine().configure(configuration);
//             engine().run();
//         });
//     }
//     // 主线程分配反应器
//     allocate_reactor(0);
//     _reactors[0] = &engine();
//     auto queue_idx = alloc_io_queue(0);
//     // 等待所有反应器注册完成
//     reactors_registered.wait();
//     std::cout<<"reactors registered done"<<std::endl;
//     /*----------------------------上面代码成功执行-------------------------------------------------- */
//     smp::_qs = new smp_message_queue* [smp::count];
//
//     std::cout<<"smp qs begin"<<std::endl;
//     for(unsigned i = 0; i < smp::count; i++) {
//         smp::_qs[i] = reinterpret_cast<smp_message_queue*>(operator new[] (sizeof(smp_message_queue) * smp::count));
//         for (unsigned j = 0; j < smp::count; ++j) {
//             new (&smp::_qs[i][j]) smp_message_queue(_reactors[j], _reactors[i]);
//         }
//     }
//     std::cout<<"smp qs end"<<std::endl;
//     smp_queues_constructed.wait();
//     std::cout<<"start all queues"<<std::endl;
//     start_all_queues();
//     std::cout<<"start all queues done"<<std::endl;
//     assign_io_queue(0, queue_idx);
//     std::cout<<"assign io queues done"<<std::endl;
//     inited.wait();
//     std::cout<<"inited done"<<std::endl;
//     // 配置引擎并启动低分辨率时钟
//     engine().configure(configuration);//这里出错(为什么之前的没有报错?)
//     std::cout<<"engine configure done"<<std::endl;
//     engine()._lowres_clock = std::make_unique<lowres_clock>();
// }
//
// bool smp::poll_queues() {
//     size_t got = 0;
//     for (unsigned i = 0; i < count; i++) {
//         if (engine().cpu_id() != i) {
//             auto& rxq = _qs[engine().cpu_id()][i];
//             rxq.flush_response_batch();
//             got += rxq.has_unflushed_responses();
//             got += rxq.process_incoming();
//             auto& txq = _qs[i][engine()._id];
//             txq.flush_request_batch();
//             got += txq.process_completions();
//         }
//     }
//     return got != 0;
// }
//
// bool smp::pure_poll_queues() {
//     for (unsigned i = 0; i < count; i++) {
//         if (engine().cpu_id() != i) {
//             auto& rxq = _qs[engine().cpu_id()][i];
//             rxq.flush_response_batch();
//             auto& txq = _qs[i][engine()._id];
//             txq.flush_request_batch();
//             if (rxq.pure_poll_rx() || txq.pure_poll_tx() || rxq.has_unflushed_responses()) {
//                 return true;
//             }
//         }
//     }
//     return false;
// }
//
// int reactor::run(){
//     auto signal_stack = install_signal_handler_stack();
//     poller io_poller(std::make_unique<io_pollfn>(*this));
//     poller sig_poller(std::make_unique<signal_pollfn>(*this));
//     poller aio_poller(std::make_unique<aio_batch_submit_pollfn>(*this));
//     poller batch_flush_poller(std::make_unique<batch_flush_pollfn>(*this));
//     poller execution_stage_poller(std::make_unique<execution_stage_pollfn>());
//     start_aio_eventfd_loop();
//     if (_id == 0) {
//        if (_handle_sigint) {
//         //这里为true.
//           _signals.handle_signal_once(SIGINT, [this] { stop(); });
//        }
//        _signals.handle_signal_once(SIGTERM, [this] { stop(); });
//     }
//     _signals.handle_signal(alarm_signal(), [this] {
//         complete_timers(_timers, _expired_timers, [this] {
//         if (!_timers.empty()) {
//                 // std::cout << "Enabling timer for " << _timers.get_next_timeout()<<std::endl;//这行是有执行的.
//                 enable_timer(_timers.get_next_timeout());
//             }
//         });
//     });
//     _cpu_started.wait(smp::count).then([this] {
//         _network_stack->initialize().then([this] {
//             _start_promise.set_value();
//         });
//     });
//     _network_stack_ready_promise.get_future().then([this] (std::unique_ptr<network_stack> stack) {
//         _network_stack = std::move(stack);
//         for (unsigned c = 0; c < smp::count; c++) {
//             smp::submit_to(c, [] {
//                     engine()._cpu_started.signal();
//             });
//         }
//     });
//     // Register smp queues poller
//     std::optional<poller> smp_poller;
//     if (smp::count > 1) {
//         smp_poller = poller(std::make_unique<smp_pollfn>(*this));
//     }
//     poller syscall_poller(std::make_unique<syscall_pollfn>(*this));
//     _signals.handle_signal(alarm_signal(), [this] {
//         complete_timers(_timers, _expired_timers, [this] {
//             if (!_timers.empty()) {
//                 enable_timer(_timers.get_next_timeout());
//             }
//         });
//     });
//     poller drain_cross_cpu_freelist(std::make_unique<drain_cross_cpu_freelist_pollfn>());
//     poller expire_lowres_timers(std::make_unique<lowres_timer_pollfn>(*this));
//     using namespace std::chrono_literals;
//     timer<lowres_clock> load_timer;
//     auto last_idle = _total_idle;
//     auto idle_start = steady_clock_type::now(), idle_end = idle_start;
//     load_timer.set_callback([this, &last_idle, &idle_start, &idle_end] () mutable {
//         _total_idle += idle_end - idle_start;
//         auto load = double((_total_idle - last_idle).count()) / double(std::chrono::duration_cast<steady_clock_type::duration>(1s).count());
//         last_idle = _total_idle;
//         load = std::min(load, 1.0);
//         idle_start = idle_end;
//         _loads.push_front(load);
//         if (_loads.size() > 5) {
//             auto drop = _loads.back();
//             _loads.pop_back();
//             _load -= (drop/5);
//         }
//         _load += (load/5);
//     });
//     load_timer.arm_periodic(1s);
//     itimerspec its = posix::to_relative_itimerspec(_task_quota, _task_quota);
//     _task_quota_timer.timerfd_settime(0, its);
//     auto& task_quote_itimerspec = its;
//     struct sigaction sa_block_notifier = {};
//     sa_block_notifier.sa_handler = &reactor::block_notifier;
//     sa_block_notifier.sa_flags = SA_RESTART;
//     auto r = sigaction(block_notifier_signal(), &sa_block_notifier, nullptr);
//     assert(r == 0);
//     bool idle = false;
//     std::function<bool()> check_for_work = [this] () {
//         return poll_once() || !_pending_tasks.empty() || thread::try_run_one_yielded_thread();
//     };
//     std::function<bool()> pure_check_for_work = [this] () {
//         return pure_poll_once() || !_pending_tasks.empty() || thread::try_run_one_yielded_thread();
//     };
//
//     while(true){
//         run_tasks(_pending_tasks);
//         if (_stopped) {
//             load_timer.cancel();
//             // Final tasks may include sending the last response to cpu 0, so run them
//             while (!_pending_tasks.empty()) {
//                 run_tasks(_pending_tasks);
//             }
//             while (!_at_destroy_tasks.empty()) {
//                 run_tasks(_at_destroy_tasks);
//             }
//             smp::arrive_at_event_loop_end();
//             if (_id == 0) {
//                 smp::join_all();
//             }
//             break;
//         }
//
//         increment_nonatomically(_polls);
//
//         if (check_for_work()) {
//             if (idle) {
//                 _total_idle += idle_end - idle_start;
//                 idle_start = idle_end;
//                 idle = false;
//             }
//         } else {
//             idle_end = steady_clock_type::now();
//             if (!idle) {
//                 idle_start = idle_end;
//                 idle = true;
//             }
//             bool go_to_sleep = true;
//             try {
//                 // we can't run check_for_work(), because that can run tasks in the context
//                 // of the idle handler which change its state, without the idle handler expecting
//                 // it.  So run pure_check_for_work() instead.
//                 auto handler_result = _idle_cpu_handler(pure_check_for_work);
//                 go_to_sleep = handler_result == idle_cpu_handler_result::no_more_work;
//             } catch (...) {
//                 throw std::runtime_error("idle_cpu_handler() threw exception");
//             }
//             if (go_to_sleep) {
//                 _mm_pause();
//                 if (idle_end - idle_start > _max_poll_time) {
//                     // Turn off the task quota timer to avoid spurious wakeiups
//                     struct itimerspec zero_itimerspec = {};
//                     _task_quota_timer.timerfd_settime(0, zero_itimerspec);
//                     sleep();
//                     // We may have slept for a while, so freshen idle_end
//                     idle_end = steady_clock_type::now();
//                     _task_quota_timer.timerfd_settime(0, task_quote_itimerspec);
//                 }
//             } else {
//                 // We previously ran pure_check_for_work(), might not actually have performed
//                 // any work.
//                 check_for_work();
//             }
//         }
//     }
//     my_io_queue.reset(nullptr);
//     return _return;
// }
//
//
// boost::program_options::options_description
// reactor::get_options_description() {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("Core options");
//     // auto net_stack_names = network_stack_registry::list();
//     opts.add_options()
//         // ("network-stack", bpo::value<std::string>(),
//         //         sprint("select network stack (valid values: %s)",
//         //                 format_separated(net_stack_names.begin(), net_stack_names.end(), ", ")).c_str())
//         ("no-handle-interrupt", "ignore SIGINT (for gdb)")
//         ("poll-mode", "poll continuously (100% cpu use)")
//         ("idle-poll-time-us", bpo::value<unsigned>()->default_value(200us / 1us),
//                 "idle polling time in microseconds (reduce for overprovisioned environments or laptops)")
//         ("poll-aio", bpo::value<bool>()->default_value(true),
//                 "busy-poll for disk I/O (reduces latency and increases throughput)")
//         ("task-quota-ms", bpo::value<double>()->default_value(2.0), "Max time (ms) between polls")
//         ("max-task-backlog", bpo::value<unsigned>()->default_value(1000), "Maximum number of task backlog to allow; above this we ignore I/O")
//         ("blocked-reactor-notify-ms", bpo::value<unsigned>()->default_value(2000), "threshold in miliseconds over which the reactor is considered blocked if no progress is made")
//         ("relaxed-dma", "allow using buffered I/O if DMA is not available (reduces performance)")
//         ("overprovisioned", "run in an overprovisioned environment (such as docker or a laptop); equivalent to --idle-poll-time-us 0 --thread-affinity 0 --poll-aio 0")
//         ("abort-on-seastar-bad-alloc", "abort when seastar allocator cannot allocate memory");
//     // opts.add(network_stack_registry::options_description());
//     return opts;
// }
//
//
// void reactor::configure(boost::program_options::variables_map vm) {
//     std::cout<<"reactor::configure"<<std::endl;
//     auto network_stack_ready =
//        vm.count("network-stack")
//         ? network_stack_registry::create(sstring(vm["network-stack"].as<std::string>()), vm)
//         : network_stack_registry::create(vm);
//
//
//     network_stack_ready.then([this] (std::unique_ptr<network_stack> stack) {
//         _network_stack_ready_promise.set_value(std::move(stack));
//     });
//     std::cout<<"reactor::configure完成"<<std::endl;
//     /*上面这段代码什么意思？*/
//     std::cout<<"reactor::configure"<<std::endl;
//     _handle_sigint = !vm.count("no-handle-interrupt");
//     std::cout<<"reactor::configure  _handle_sigint:"<<_handle_sigint<<std::endl;
//     _task_quota = vm["task-quota-ms"].as<double>()*1ms;
//     std::cout<<"reactor::configure  _task_quota:"<<std::endl;
//     auto blocked_time = vm["blocked-reactor-notify-ms"].as<unsigned>()*1ms;
//     std::cout<<"reactor::configure  blocked_time:"<<std::endl;
//     _tasks_processed_report_threshold = unsigned(blocked_time / _task_quota);
//     std::cout<<"reactor::configure  _tasks_processed_report_threshold:"<<std::endl;
//     _max_task_backlog = vm["max-task-backlog"].as<unsigned>();
//     std::cout<<"reactor::configure  _max_task_backlog:"<<std::endl;
//     // _max_poll_time = vm["idle-poll-time-us"].as<unsigned>() * 1us;
//     _max_poll_time = 200us;
//     std::cout<<"reactor::configure  _max_poll_time:"<<std::endl;
//     if (vm.count("poll-mode")) {
//         std::cout<<"reactor::configure  poll_mode"<<std::endl;
//         _max_poll_time = std::chrono::nanoseconds::max();
//     }
//     if (vm.count("overprovisioned")
//            && vm["idle-poll-time-us"].defaulted()
//            && !vm.count("poll-mode")) {
//             std::cout<<"reactor::configure  overprovisioned"<<std::endl;
//         _max_poll_time = 0us;
//     }
//     set_strict_dma(!vm.count("relaxed-dma"));
//     std::cout<<"reactor::configure  _strict_dma"<<std::endl;
//
//     // if (!vm["poll-aio"].as<bool>()
//     //         || (vm["poll-aio"].defaulted() && vm.count("overprovisioned"))) {
//     //     _aio_eventfd = pollable_fd(file_desc::eventfd(0, 0));
//     // }
// }
// future<> reactor::run_exit_tasks() {
//     _stop_requested.broadcast();
//     _stopping = true;
//     // // stop_aio_eventfd_loop();
//     // return do_for_each(_exit_funcs.rbegin(), _exit_funcs.rend(), [] (auto& func) {
//     //    return func();
//     // });
// }
//
// reactor::reactor(unsigned id)
//     : _id(id)
//     , _cpu_started(0)
//     , _io_context(0)
//     , _io_context_available(max_aio),_reuseport(posix_reuseport_detect()),_task_quota_timer(file_desc::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC))
//     , _thread_pool("sys"+id)
//     {
//     thread_impl::init();
//     auto r = ::io_setup(max_aio, &_io_context);
//     assert(r >= 0);
//     sigset_t mask;
//     sigemptyset(&mask);
//     sigaddset(&mask, alarm_signal());
//     r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
//     assert(r == 0);
//     struct sigevent sev;
//     sev.sigev_notify = SIGEV_THREAD_ID;
//     sev._sigev_un._tid = syscall(SYS_gettid);
//     sev.sigev_signo = alarm_signal();
//     r = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
//     assert(r >= 0);
//     sigemptyset(&mask);
//     sigaddset(&mask, block_notifier_signal());
//     r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
//     assert(r == 0);
//     memory::set_reclaim_hook([this] (std::function<void ()> reclaim_fn) {
//         add_high_priority_task(make_task([fn = std::move(reclaim_fn)] {
//             fn();
//         }));
//     });
// }
// reactor::~reactor() {
//     if (_steady_clock_timer) {
//         timer_delete(_steady_clock_timer);
//     }
// }
//
// void reactor::at_exit(std::function<future<> ()> func) {
//     assert(!_stopping);
//     _exit_funcs.push_back(std::move(func));
// }
//
// bool
// reactor::posix_reuseport_detect() {
//     return false; // FIXME: reuseport currently leads to heavy load imbalance. Until we fix that, just
//                   // disable it unconditionally.
// /*思考为什么 reuseport会引入负载不均衡*/
//     try {
//         file_desc fd = file_desc::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
//         fd.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1);
//         return true;
//     } catch(std::system_error& e) {
//         return false;
//     }
// }
//
// void reactor::run_tasks(std::deque<std::unique_ptr<task>>& tasks) {
//     while (!tasks.empty()) {
//         std::cout<<"run_task开始执行"<<std::endl;
//         auto tsk = std::move(tasks.front());
//         tasks.pop_front();
//         tsk->run();
//         std::cout<<"run_task结束执行"<<std::endl;
//         tsk.reset();
//     }
// }
//
// void reactor::exit(int ret) {
//     smp::submit_to(0, [this, ret] { _return = ret; stop(); });
// }
//
// void reactor::stop() {
//     assert(engine()._id == 0);
//     smp::cleanup_cpu();
//     if (!_stopping) {
//
//     }
// }
//
//
//
// void smp::pin(unsigned cpu_id) {
//     pin_this_thread(cpu_id);
// }
//
// void smp::arrive_at_event_loop_end() {
//     if (_all_event_loops_done) {
//         _all_event_loops_done->wait();
//     }
// }
//
// void smp::allocate_reactor(unsigned id) {
//     std::cout<<"开始执行smp::allocate_reactor"<<std::endl;
//     assert(!reactor_holder::get());
//     // we cannot just write "local_engin = new reactor" since reactor's constructor
//     // uses local_engine
//     void *buf;
//     int r = posix_memalign(&buf, 64, sizeof(reactor));
//     assert(r == 0);
//     local_engine = reinterpret_cast<reactor*>(buf);
//     new (buf) reactor(id); // 为什么要这样new?
//     reactor_holder::get().reset(local_engine);
//     std::cout<<"smp::allocate_reactor结束"<<std::endl;
//
// }
//
// void smp::cleanup() {
//     smp::_threads = std::vector<posix_thread>();
//     _thread_loops.clear();
// }
//
// void smp::cleanup_cpu() {
//     size_t cpuid = engine().cpu_id();
//
//     if (_qs) {
//         for(unsigned i = 0; i < smp::count; i++) {
//             _qs[i][cpuid].stop();
//         }
//     }
// }
//
// void smp::create_thread(std::function<void ()> thread_loop) {
//     _threads.emplace_back(std::move(thread_loop));
// }
//
// bool
// reactor::poll_once() {
//     bool work = false;
//     for (auto c : _pollers) {
//         work |= c->poll();
//     }
//
//     return work;
// }
//
// // Join function
// future<> thread::join() {
//     _context->_joined = true;
//     return _context->_done.get_future();
// }
//
// signals::signals() : _pending_signals(0) {
// }
//
// signals::~signals() {
//     sigset_t mask;
//     sigfillset(&mask);
//     ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
// }
//
// signals::signal_handler::signal_handler(int signo, std::function<void ()>&& handler)
//         : _handler(std::move(handler)) {
//             std::cout<<"调用signal_handler"<<std::endl;
//     struct sigaction sa;
//     sa.sa_sigaction = action;//这个是信号处理函数.
//     sa.sa_mask = make_empty_sigset_mask();
//     sa.sa_flags = SA_SIGINFO | SA_RESTART;
//     engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
//     std::cout<<"engine().signals"<<engine()._signals._pending_signals<<std::endl;
//     auto r = ::sigaction(signo, &sa, nullptr);
//     // throw_system_error_on(r == -1);
//     auto mask = make_sigset_mask(signo);
//     r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
//     throw_pthread_error(r);
// }
//
// void signals::handle_signal(int signo, std::function<void ()>&& handler) {
//     std::cout<<"handle signal"<<std::endl;
//     _signal_handlers.emplace(std::piecewise_construct,std::make_tuple(signo), std::make_tuple(signo, std::move(handler)));
//     //插入singo,和对应的handler.
// }
//
// void signals::handle_signal_once(int signo, std::function<void ()>&& handler) {
//     return handle_signal(signo, [fired = false, handler = std::move(handler)] () mutable {
//         if (!fired) {
//             fired = true;
//             handler();
//         }
//     });
// }
//
// bool signals::poll_signal() {
//     auto signals = _pending_signals.load(std::memory_order_relaxed);
//     //为什么一直是0.
//
//     if (signals) {
//             std::cout<<"signals "<<signals<<std::endl;
//         _pending_signals.fetch_and(~signals, std::memory_order_relaxed);
//         for (size_t i = 0; i < sizeof(signals)*8; i++) {
//             // std::cout<<"遍历"<<std::endl;
//             if (signals & (1ull << i)) {
//                 //执行handler
//                 // std::cout<<"执行handler"<<std::endl;
//                _signal_handlers.at(i)._handler();
//             }
//         }
//     }
//     return signals;
// }
// bool signals::pure_poll_signal() const {
//     return _pending_signals.load(std::memory_order_relaxed);
// }
//
// void signals::action(int signo, siginfo_t* siginfo, void* ignore) {
//     // std::cout<<"action########"<<std::endl;
//     engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
// }
//
//
// void thread_context::stack_deleter::operator()(char* ptr) const noexcept {
//     delete[] ptr;
// }
//
// void
// thread_context::setup() {
//     // use setcontext() for the initial jump, as it allows us
//     // to set up a stack, but continue with longjmp() as it's much faster.
//     ucontext_t initial_context;
//     auto q = uint64_t(reinterpret_cast<uintptr_t>(this));//将thread_
//     auto main = reinterpret_cast<void (*)()>(&thread_context::s_main);
//     auto r = getcontext(&initial_context);//保存当前上下文到initial_context中.
//     // throw_system_error_on(r == -1);
//     initial_context.uc_stack.ss_sp = _stack.get(); //设置栈空间
//     initial_context.uc_stack.ss_size = _stack_size;
//     initial_context.uc_link = nullptr;
//     makecontext(&initial_context, main, 2, int(q), int(q >> 32));  //makecontext前32位，后32位.
//     _context.thread = this;//_context是jmp_buf_link类型.(绑定父类型)
//     _context.initial_switch_in(&initial_context, _stack.get(), _stack_size);//进入这个函数准备执行了s_main
//     std::cout<<"执行完了回调函数"<<std::endl;
// }
//
// void thread_context::switch_in() {
//     if (_attr.scheduling_group) {
//         _attr.scheduling_group->account_start();
//         _context.set_yield_at(_attr.scheduling_group->_this_run_start + _attr.scheduling_group->_this_period_remain);
//     } else {
//         _context.clear_yield_at();//设置_context的yield_为false
//     }
//     _context.switch_in();
// }
//
// void thread_context::switch_out() {
//     if (_attr.scheduling_group) {
//         _attr.scheduling_group->account_stop();
//     }
//     _context.switch_out();
// }
//
// bool thread_context::should_yield() const {
//     if (!_attr.scheduling_group) {
//         return need_preempt();
//     }
//     return need_preempt() || bool(_attr.scheduling_group->next_scheduling_point());
// }
//
//
// size_t smp_message_queue::process_completions() {
//     auto nr = process_queue<prefetch_cnt*2>(_completed, [] (work_item* wi) {
//         wi->complete();
//         delete wi;
//     });
//     _current_queue_length -= nr;
//     _compl += nr;
//     _last_cmpl_batch = nr;
//     return nr;
// }
//
// void smp_message_queue::flush_request_batch() {
//     if (!_tx.a.pending_fifo.empty()) {
//         move_pending();
//     }
// }
//
// size_t smp_message_queue::process_incoming() {
//     auto nr = process_queue<prefetch_cnt>(_pending, [this] (work_item* wi) {
//         wi->process().then([this, wi] {
//             respond(wi);
//         });
//     });
//     _received += nr;
//     _last_rcv_batch = nr;
//     return nr;
// }
//
// void smp_message_queue::start(unsigned cpuid) {
//     _tx.init();
//     char instance[10];
//     std::snprintf(instance, sizeof(instance), "%u-%u", engine().cpu_id(), cpuid);
// }
//
// bool reactor::do_check_lowres_timers() const{
//     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
//         return false;
//     }
//     return lowres_clock::now() > engine()._lowres_next_timeout;
// }
//
// thread_context::stack_holder
// thread_context::make_stack() {
//     auto stack = stack_holder(new char[_stack_size]);
//     return stack;
// }
//
// thread_context::thread_context(thread_attributes attr, std::function<void ()> func)
//         : _attr(std::move(attr))
//         , _func(std::move(func)) {
//     setup();
//     std::cout<<"添加this到all_threads"<<std::endl;
//     _all_threads.push_front(this);
//     _all_it = _all_threads.begin();
//     //为什么这里是this,而不是*this，而不是_all_it?思考
//     //因为_all_threads存放的就是thread_context*，所以添加的也是指针。
// }
//
// void
// smp_message_queue::lf_queue::maybe_wakeup() {
//     // Called after lf_queue_base::push().
//     //
//     // This is read-after-write, which wants memory_order_seq_cst,
//     // but we insert that barrier using systemwide_memory_barrier()
//     // because seq_cst is so expensive.
//     //
//     // However, we do need a compiler barrier:
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     if (remote->_sleeping.load(std::memory_order_relaxed)) {
//         // We are free to clear it, because we're sending a signal now
//         remote->_sleeping.store(false, std::memory_order_relaxed);
//         remote->wakeup();
//     }
// }
//
// void thread::yield() {
//     thread_impl::get()->yield();
// }
//
// bool thread::should_yield() {
//     return thread_impl::get()->should_yield();
// }
//
//
// // Destructor
// thread::~thread() {
//     assert(!_context || _context->_joined);
// }
//
// void reactor::expire_manual_timers() {
//     complete_timers(engine()._manual_timers, engine()._expired_manual_timers, []{
//         std::cout<<"到期"<<std::endl;
//     });
// }
//
// void manual_clock::expire_timers() {
//     engine().expire_manual_timers();
// }
//
// inline void jmp_buf_link::initial_switch_in(ucontext_t* initial_context, const void*, size_t)
// {
//     if(g_current_context){
//         std::cout<<"g_current_context非空"<<std::endl;
//     }else{
//         std::cout<<"g_current_context不空"<<std::endl;
//     }
//     auto prev = std::exchange(g_current_context, this);
//     link = prev;
//     if (setjmp(prev->jmpbuf) == 0) {
//         std::cout<<"init setjmp"<<std::endl;
//         //  如果第一次setjmp
//         setcontext(initial_context);
//         //  这里会跳转到initial_context的入口函数中去执行.
//         //  使用setcontext而不是longjmp，需要设置完整的初始上下文.
//     }
//     /*
//     在这个过程中,已经执行完了绑定在线程上的回调函数。
//     */
//     std::cout<<"final long jmp"<<std::endl;
// }
//
//
// inline void jmp_buf_link::switch_in()
// {
//     auto prev = std::exchange(g_current_context, this);
//     link = prev;
//     if (setjmp(prev->jmpbuf) == 0) {
//         longjmp(jmpbuf, 1);
//     }
// }
//
// inline void jmp_buf_link::switch_out(){
//     g_current_context = link;
//     if (setjmp(jmpbuf) == 0) {
//         longjmp(g_current_context->jmpbuf, 1);
//     }
// }
//
// inline void jmp_buf_link::initial_switch_in_completed(){}
//
// inline void jmp_buf_link::final_switch_out(){
//     g_current_context = link;//link就是该context对应的上一个context(恢复).
//     std::cout<<"final_switch_out"<<std::endl;
//     longjmp(g_current_context->jmpbuf, 1);//使用longjmp跳转到当前context的jmpbuf
//     //这个可能没用？
// }
//
// thread_context::~thread_context() {
//     std::cout<<"开始析构thread_context"<<std::endl;
//     _all_threads.erase(_all_it);//为什么？
// }
//
//
// void thread_context::yield() {
//     if (!_attr.scheduling_group) {
//         later().get();
//     }
//     else
//     {
//         std::cout<<"yield 有scheduling group"<<std::endl;
//         auto when = _attr.scheduling_group->next_scheduling_point();
//         if (when) {
//             _preempted_it = _preempted_threads.insert(_preempted_threads.end(), this);
//             set_sched_promise();
//             auto fut = get_sched_promise()->get_future();
//             _sched_timer.arm(*when);
//             fut.get();
//             clear_sched_promise();
//         } else if (need_preempt()) {
//             later().get();
//         }
//     }
// }
//
// void thread_context::reschedule() {
//     _preempted_threads.erase(_preempted_it);
//     _sched_promise_ptr->set_value();
// }
//
// void thread_context::s_main(unsigned int lo, unsigned int hi) {
//     uintptr_t q = lo | (uint64_t(hi) << 32);
//     std::cout<<"执行s_main"<<std::endl;
//     reinterpret_cast<thread_context*>(q)->main();
// }
//
// void
// thread_context::main() {
//     _context.initial_switch_in_completed();//这里什么都没有执行.
//     if (_attr.scheduling_group) {
//         std::cout<<"attr有scheduling group"<<std::endl;
//         _attr.scheduling_group->account_start();
//         //没有执行到这里.
//     }
//     try {
//         std::cout<<"开始执行回调函数"<<std::endl;
//         _func();            //执行线程绑定在context的函数.
//         _done.set_value(); // done的类型是promise<>，set_value把done对应的future_state<>状态设置为result.
//     } catch (...) {
//         _done.set_exception(std::current_exception());
//     }
//     if (_attr.scheduling_group) {
//         _attr.scheduling_group->account_stop();
//     }
//     _context.final_switch_out();
// }
//
// smp_message_queue::smp_message_queue(reactor* from, reactor* to) : _pending(to),_completed(from){ }
//
// void smp_message_queue::stop() {
//     // _metrics.clear();
// }
// void smp_message_queue::move_pending() {
//     auto begin = _tx.a.pending_fifo.cbegin();
//     auto end = _tx.a.pending_fifo.cend();
//     end = _pending.push(begin, end);
//     if (begin == end) {
//         return;
//     }
//     auto nr = end - begin;
//     _pending.maybe_wakeup();
//     _tx.a.pending_fifo.erase(begin, end);
//     _current_queue_length += nr;
//     _last_snt_batch = nr;
//     _sent += nr;
// }
//
// bool smp_message_queue::pure_poll_tx() const {
//     // can't use read_available(), not available on older boost
//     // empty() is not const, so need const_cast.
//     return !const_cast<lf_queue&>(_completed).empty();
// }
//
// void smp_message_queue::submit_item(std::unique_ptr<smp_message_queue::work_item> item) {
//     _tx.a.pending_fifo.push_back(item.get());
//     item.release();
//     if (_tx.a.pending_fifo.size() >= batch_size) {
//         move_pending();
//     }
// }
//
// void smp_message_queue::respond(work_item* item) {
//     _completed_fifo.push_back(item);
//     if (_completed_fifo.size() >= batch_size || engine()._stopped) {
//         flush_response_batch();
//     }
// }
//
// void smp_message_queue::flush_response_batch() {
//     if (!_completed_fifo.empty()) {
//         auto begin = _completed_fifo.cbegin();
//         auto end = _completed_fifo.cend();
//         end = _completed.push(begin, end);
//         if (begin == end) {
//             return;
//         }
//         _completed.maybe_wakeup();
//         _completed_fifo.erase(begin, end);
//     }
// }
//
// bool smp_message_queue::has_unflushed_responses() const {
//     return !_completed_fifo.empty();
// }
//
// bool smp_message_queue::pure_poll_rx() const {
//     // can't use read_available(), not available on older boost
//     // empty() is not const, so need const_cast.
//     return !const_cast<lf_queue&>(_pending).empty();
// }
//
// void future_state<>::forward_to(promise<>& pr) noexcept {
//     assert(_u.st != state::future && _u.st != state::invalid);
//     if (_u.st >= state::exception_min) {
//         pr.set_urgent_exception(std::move(_u.ex));
//         _u.ex.~exception_ptr();
//     } else {
//         pr.set_urgent_value(std::tuple<>());
//     }
//     _u.st = state::invalid;
// }
//
//
// bool thread::try_run_one_yielded_thread() {
//     if (thread_context::_preempted_threads.empty()) {
//         return false;
//     }
//     auto* t = thread_context::_preempted_threads.front();
//     t->_sched_timer.cancel();
//     t->_sched_promise_ptr->set_value();
//     thread_context::_preempted_threads.pop_front();
//     return true;
// }
// thread_scheduling_group::thread_scheduling_group(std::chrono::nanoseconds period, float usage)
//         : _period(period), _quota(std::chrono::duration_cast<std::chrono::nanoseconds>(usage * period)) {
// }
//
// void thread_scheduling_group::account_start() {
//     auto now = thread_clock::now();
//     if (now >= _this_period_ends) {
//         _this_period_ends = now + _period;
//         _this_period_remain = _quota;
//     }
//     _this_run_start = now;
// }
//
// void thread_scheduling_group::account_stop() {
//     _this_period_remain -= thread_clock::now() - _this_run_start;
// }
//
// std::chrono::steady_clock::time_point*
// thread_scheduling_group::next_scheduling_point() const {
//     auto now = thread_clock::now();
//     auto current_remain = _this_period_remain - (now - _this_run_start);
//     if (current_remain > std::chrono::nanoseconds(0)) {
//         return nullptr;
//     }
//     static std::chrono::steady_clock::time_point result;
//     result = _this_period_ends - current_remain;
//     return &result;
// }
//
// void reactor::del_timer(manual_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_manual_timers.erase(tmr->expired_it);
//         tmr->_expired = false;
//     } else {
//         _manual_timers.remove(*tmr);
//     }
// }
//
// /*0000000000000000000000000000000000000000000000000000000000*/
// /*0000000000000000000000000000000000000000000000000000000000*/
//
//
//
// void reactor::enable_timer(steady_clock_type::time_point when) {
//     itimerspec its;
//     its.it_interval = {};
//     its.it_value = to_timespec(when);
//     auto ret = timer_settime(_steady_clock_timer, TIMER_ABSTIME, &its, NULL);
//     // throw_system_error_on(ret == -1);
// }
//
//
// /*  信号触发​:默认情况下，定时器到期会发送一个信号（如 SIGALRM）到进程。进程可以通过信号处理函数来处理定时器到期事件. */
//
// void reactor::add_timer(steady_timer *tmr) {
//     std::cout<<"reactor add timer"<<std::endl;
//     if (queue_timer(tmr)) {
//         enable_timer(_timers.get_next_timeout());
//     }
// }
// void reactor::add_timer(lowres_timer* tmr) {
//     if (queue_timer(tmr)) {
//         _lowres_next_timeout = _lowres_timers.get_next_timeout();
//     }
// }
//
// void reactor::add_timer(manual_timer* tmr) {
//     queue_timer(tmr);
// }
// bool reactor::queue_timer(lowres_timer* tmr) {
//     return _lowres_timers.insert(*tmr);
// }
//
// bool reactor::queue_timer(manual_timer* tmr) {
//     return _manual_timers.insert(*tmr);
// }
//
// bool reactor::queue_timer(steady_timer* tmr) {
//     // std::cout<<"reaactor queue timer"<<std::endl;
//     return _timers.insert(*tmr);
// }
//
//
// /*
//     del_timer什么时候调用?
// */
// void reactor::del_timer(steady_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_timers.erase(tmr->expired_it);  // 直接使用保存的迭代器
//         tmr->_expired = false;
//     } else {
//         _timers.remove(*tmr);  // 通过 it 成员快速删除
//     }
// }
//
// // 同理修改其他 del_timer 函数：
// void reactor::del_timer(lowres_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_lowres_timers.erase(tmr->expired_it);
//         tmr->_expired = false;
//     } else {
//         _lowres_timers.remove(*tmr);
//     }
// }
//
// /*not yet implemented for OSv. TODO: do the notification like we do class smp.*/
// readable_eventfd writeable_eventfd::read_side() {
//     return readable_eventfd(_fd.dup());
// }
//
// file_desc writeable_eventfd::try_create_eventfd(size_t initial) {
//     assert(size_t(int(initial)) == initial);
//     return file_desc::eventfd(initial, EFD_CLOEXEC);
// }
//
// void writeable_eventfd::signal(size_t count) {
//     uint64_t c = count;
//     auto r = _fd.write(&c, sizeof(c));
//     assert(r == sizeof(c));
// }
//
// writeable_eventfd readable_eventfd::write_side() {
//     return writeable_eventfd(_fd.get_file_desc().dup());
// }
//
// file_desc readable_eventfd::try_create_eventfd(size_t initial) {
//     assert(size_t(int(initial)) == initial);
//     return file_desc::eventfd(initial, EFD_CLOEXEC | EFD_NONBLOCK);
// }
//
// future<size_t> readable_eventfd::wait() {
//     return engine().readable(*_fd._s).then([this] {
//         uint64_t count;
//         int r = ::read(_fd.get_fd(), &count, sizeof(count));
//         assert(r == sizeof(count));
//         return make_ready_future<size_t>(count);
//     });
// }
//
//
//
// inline
// future<size_t> pollable_fd::write_some(net::packet& p) {
//     return engine().writeable(*_s).then([this, &p] () mutable {
//         static_assert(offsetof(iovec, iov_base) == offsetof(net::fragment, base) &&
//             sizeof(iovec::iov_base) == sizeof(net::fragment::base) &&
//             offsetof(iovec, iov_len) == offsetof(net::fragment, size) &&
//             sizeof(iovec::iov_len) == sizeof(net::fragment::size) &&
//             alignof(iovec) == alignof(net::fragment) &&
//             sizeof(iovec) == sizeof(net::fragment)
//             , "net::fragment and iovec should be equivalent");
//
//         iovec* iov = reinterpret_cast<iovec*>(p.fragment_array());
//         msghdr mh = {};
//         mh.msg_iov = iov;
//         mh.msg_iovlen = p.nr_frags();
//         auto r = get_file_desc().sendmsg(&mh, MSG_NOSIGNAL);
//         if (!r) {
//             return write_some(p);
//         }
//         if (size_t(*r) == p.len()) {
//             _s->speculate_epoll(EPOLLOUT);
//         }
//         return make_ready_future<size_t>(*r);
//     });
// }
//
// inline
// future<> pollable_fd::write_all(net::packet& p) {
//     return write_some(p).then([this, &p] (size_t size) {
//         if (p.len() == size) {
//             return make_ready_future<>();
//         }
//         p.trim_front(size);
//         return write_all(p);
//     });
// }
//
// inline
// future<> pollable_fd::readable() {
//     return engine().readable(*_s);
// }
//
// inline
// future<> pollable_fd::writeable() {
//     return engine().writeable(*_s);
// }
//
// inline
// void
// pollable_fd::abort_reader(std::exception_ptr ex) {
//     engine().abort_reader(*_s, std::move(ex));
// }
//
// inline
// void
// pollable_fd::abort_writer(std::exception_ptr ex) {
//     engine().abort_writer(*_s, std::move(ex));
// }
//
// inline
// future<pollable_fd, socket_address> pollable_fd::accept() {
//     return engine().accept(*_s);
// }
//
// inline
// future<size_t> pollable_fd::recvmsg(struct msghdr *msg) {
//     return engine().readable(*_s).then([this, msg] {
//         auto r = get_file_desc().recvmsg(msg, 0);
//         if (!r) {
//             return recvmsg(msg);
//         }
//         // We always speculate here to optimize for throughput in a workload
//         // with multiple outstanding requests. This way the caller can consume
//         // all messages without resorting to epoll. However this adds extra
//         // recvmsg() call when we hit the empty queue condition, so it may
//         // hurt request-response workload in which the queue is empty when we
//         // initially enter recvmsg(). If that turns out to be a problem, we can
//         // improve speculation by using recvmmsg().
//         _s->speculate_epoll(EPOLLIN);
//         return make_ready_future<size_t>(*r);
//     });
// };
//
// inline
// future<size_t> pollable_fd::sendmsg(struct msghdr* msg) {
//     return engine().writeable(*_s).then([this, msg] () mutable {
//         auto r = get_file_desc().sendmsg(msg, 0);
//         if (!r) {
//             return sendmsg(msg);
//         }
//         // For UDP this will always speculate. We can't know if there's room
//         // or not, but most of the time there should be so the cost of mis-
//         // speculation is amortized.
//         if (size_t(*r) == iovec_len(msg->msg_iov, msg->msg_iovlen)) {
//             _s->speculate_epoll(EPOLLOUT);
//         }
//         return make_ready_future<size_t>(*r);
//     });
// }
//
// inline
// future<size_t> pollable_fd::sendto(socket_address addr, const void* buf, size_t len) {
//     return engine().writeable(*_s).then([this, buf, len, addr] () mutable {
//         auto r = get_file_desc().sendto(addr, buf, len, 0);
//         if (!r) {
//             return sendto(std::move(addr), buf, len);
//         }
//         // See the comment about speculation in sendmsg().
//         if (size_t(*r) == len) {
//             _s->speculate_epoll(EPOLLOUT);
//         }
//         return make_ready_future<size_t>(*r);
//     });
// }
//
//
// inline
// future<size_t> pollable_fd::read_some(char* buffer, size_t size) {
//     return engine().read_some(*_s, buffer, size);
// }
//
// inline
// future<size_t> pollable_fd::read_some(uint8_t* buffer, size_t size) {
//     return engine().read_some(*_s, buffer, size);
// }
//
// inline
// future<size_t> pollable_fd::read_some(const std::vector<iovec>& iov) {
//     return engine().read_some(*_s, iov);
// }
//
// inline
// future<> pollable_fd::write_all(const char* buffer, size_t size) {
//     return engine().write_all(*_s, buffer, size);
// }
//
// inline
// future<> pollable_fd::write_all(const uint8_t* buffer, size_t size) {
//     return engine().write_all(*_s, buffer, size);
// }
//
// std::chrono::nanoseconds
// reactor::calculate_poll_time() {
//     // In a non-virtualized environment, select a poll time
//     // that is competitive with halt/unhalt.
//     // In a virutalized environment, IPIs are slow and dominate
//     // sleep/wake (mprotect/tgkill), so increase poll time to reduce
//     // so we don't sleep in a request/reply workload
//     return 200us; //200us是怎么得到的?
// }
// void
// reactor::block_notifier(int) {
//     auto steps = engine()._tasks_processed_stalled.load(std::memory_order_relaxed);
//     auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(engine()._task_quota * steps);
//
//     // backtrace_buffer buf;
//     // buf.append("Reactor stalled for ");
//     // buf.append_decimal(uint64_t(delta.count()));
//     // buf.append(" ms");
//     // print_with_backtrace(buf);
// }
// future<> reactor_backend_epoll::readable(pollable_fd_state& fd) {
//     return get_epoll_future(fd, &pollable_fd_state::pollin, EPOLLIN);
// }
//
// future<> reactor_backend_epoll::writeable(pollable_fd_state& fd) {
//     return get_epoll_future(fd, &pollable_fd_state::pollout, EPOLLOUT);
// }
//
// void reactor_backend_epoll::abort_reader(pollable_fd_state& fd, std::exception_ptr ex) {
//     abort_fd(fd, std::move(ex), &pollable_fd_state::pollin, EPOLLIN);
// }
//
// void reactor_backend_epoll::abort_writer(pollable_fd_state& fd, std::exception_ptr ex) {
//     abort_fd(fd, std::move(ex), &pollable_fd_state::pollout, EPOLLOUT);
// }
//
// void reactor_backend_epoll::forget(pollable_fd_state& fd) {
//     if (fd.events_epoll) {
//         ::epoll_ctl(_epollfd.get(), EPOLL_CTL_DEL, fd.fd.get(), nullptr);
//     }
// }
//
// future<> reactor_backend_epoll::notified(reactor_notifier *n) {
//     // Currently reactor_backend_epoll doesn't need to support notifiers,
//     // because we add to it file descriptors instead. But this can be fixed
//     // later.
//     std::cout << "reactor_backend_epoll does not yet support notifiers!\n";
//     abort();
// }
//
// void reactor_backend_epoll::complete_epoll_event(pollable_fd_state& pfd, promise<> pollable_fd_state::*pr,
//         int events, int event) {
//     if (pfd.events_requested & events & event) {
//         pfd.events_requested &= ~event;
//         pfd.events_known &= ~event;
//         (pfd.*pr).set_value();
//         pfd.*pr = promise<>();
//     }
// }
//
//
// reactor::poller::poller(poller&& x)
//         : _pollfn(std::move(x._pollfn)), _registration_task(x._registration_task) {
//     if (_pollfn && _registration_task) {
//         _registration_task->moved(this);
//     }
// }
//
// reactor::poller&
// reactor::poller::operator=(poller&& x) {
//     if (this != &x) {
//         this->~poller();
//         new (this) poller(std::move(x));
//     }
//     return *this;
// }
// reactor_backend_epoll::reactor_backend_epoll()
//     : _epollfd(file_desc::epoll_create(EPOLL_CLOEXEC)) {
// }
//
//
// future<> reactor_backend_epoll::get_epoll_future(pollable_fd_state& pfd,
//         promise<> pollable_fd_state::*pr, int event) {
//     if (pfd.events_known & event) {
//         pfd.events_known &= ~event;
//         return make_ready_future();
//     }
//     pfd.events_requested |= event;
//     if (!(pfd.events_epoll & event)) {
//         auto ctl = pfd.events_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
//         pfd.events_epoll |= event;
//         ::epoll_event eevt;
//         eevt.events = pfd.events_epoll;
//         eevt.data.ptr = &pfd;
//         int r = ::epoll_ctl(_epollfd.get(), ctl, pfd.fd.get(), &eevt);
//         assert(r == 0);
//         engine().start_epoll();
//     }
//     pfd.*pr = promise<>();
//     return (pfd.*pr).get_future();
// }
//
// void reactor_backend_epoll::abort_fd(pollable_fd_state& pfd, std::exception_ptr ex,
//                                      promise<> pollable_fd_state::* pr, int event) {
//     if (pfd.events_epoll & event) {
//         pfd.events_epoll &= ~event;
//         auto ctl = pfd.events_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
//         ::epoll_event eevt;
//         eevt.events = pfd.events_epoll;
//         eevt.data.ptr = &pfd;
//         int r = ::epoll_ctl(_epollfd.get(), ctl, pfd.fd.get(), &eevt);
//         assert(r == 0);
//     }
//     if (pfd.events_requested & event) {
//         pfd.events_requested &= ~event;
//         (pfd.*pr).set_exception(std::move(ex));
//     }
//     pfd.events_known &= ~event;
// }
//
// bool reactor::process_io()
// {
//     io_event ev[max_aio];
//     struct timespec timeout = {0, 0};
//     auto n = ::io_getevents(_io_context, 1, max_aio, ev, &timeout);
//     assert(n >= 0);
//     for (size_t i = 0; i < size_t(n); ++i) {
//         auto pr = reinterpret_cast<promise<io_event>*>(ev[i].data);
//         pr->set_value(ev[i]);
//         delete pr;
//     }
//     _io_context_available.signal(n);
//     return n;
// }
//
// void reactor::register_poller(pollfn* p) {
//     _pollers.push_back(p);
// }
//
// void reactor::unregister_poller(pollfn* p) {
//     _pollers.erase(std::find(_pollers.begin(), _pollers.end(), p));
// }
//
//
//
// bool
// reactor::flush_tcp_batches() {
//     bool work = _flush_batching.size();
//     while (!_flush_batching.empty()) {
//         auto os = std::move(_flush_batching.front());
//         _flush_batching.pop_front();
//         os->poll_flush();
//     }
//     return work;
// }
//
//
//
// bool
// reactor::pure_poll_once() {
//     for (auto c : _pollers) {
//         if (c->pure_poll()) {
//             return true;
//         }
//     }
//     return false;
// }
//
// template <typename Func>
// inline
// std::unique_ptr<pollfn>
// reactor::make_pollfn(Func&& func) {
//     struct the_pollfn : pollfn {
//         the_pollfn(Func&& func) : func(std::forward<Func>(func)) {}
//         Func func;
//         virtual bool poll() override final {
//             return func();
//         }
//         virtual bool pure_poll() override final {
//             return poll(); // dubious, but compatible
//         }
//     };
//     return std::make_unique<the_pollfn>(std::forward<Func>(func));
// }
//
// bool reactor::flush_pending_aio() {
//     bool did_work = false;
//     while (!_pending_aio.empty()) {
//         auto nr = _pending_aio.size();
//         struct iocb* iocbs[max_aio];
//         for (size_t i = 0; i < nr; ++i) {
//             iocbs[i] = &_pending_aio[i];
//         }
//         auto r = ::io_submit(_io_context, nr, iocbs);
//         size_t nr_consumed;
//         if (r < 0) {
//             auto ec = -r;
//             switch (ec) {
//                 case EAGAIN:
//                     return did_work;
//                 case EBADF: {
//                     auto pr = reinterpret_cast<promise<io_event>*>(iocbs[0]->data);
//                     try {
//                         // throw_kernel_error(r);
//                         std::cout<<"error"<<std::endl;
//                         throw std::system_error(ec, std::system_category());
//                     } catch (...) {
//                         pr->set_exception(std::current_exception());
//                     }
//                     delete pr;
//                     _io_context_available.signal(1);
//                     // if EBADF, it means that the first request has a bad fd, so
//                     // we will only remove it from _pending_aio and try again.
//                     nr_consumed = 1;
//                     break;
//                 }
//                 default:
//                     throw std::system_error(ec, std::system_category());
//                     abort();
//             }
//         } else {
//             nr_consumed = size_t(r);
//         }
//
//         did_work = true;
//         if (nr_consumed == nr) {
//             _pending_aio.clear();
//         } else {
//             _pending_aio.erase(_pending_aio.begin(), _pending_aio.begin() + nr_consumed);
//         }
//     }
//     return did_work;
// }
// file_desc
// file_desc::temporary(std::string directory) {
//     // FIXME: add O_TMPFILE support one day
//     directory += "/XXXXXX";
//     std::vector<char> templat(directory.c_str(), directory.c_str() + directory.size() + 1);
//     int fd = ::mkstemp(templat.data());
//
//     int r = ::unlink(templat.data());
//     // throw_system_error_on(r == -1); // leaks created file, but what can we do?
//     return file_desc(fd);
// }
//
// namespace net{
//
// inline
// packet::packet(packet&& x) noexcept
//     : _impl(std::move(x._impl)) {
// }
//
// inline
// packet::impl::impl(size_t nr_frags)
//     : _len(0), _allocated_frags(nr_frags) {
// }
//
// inline
// packet::impl::impl(fragment frag, size_t nr_frags)
//     : _len(frag.size), _allocated_frags(nr_frags) {
//     assert(_allocated_frags > _nr_frags);
//     if (frag.size <= internal_data_size) {
//         _headroom -= frag.size;
//         _frags[0] = { _data + _headroom, frag.size };
//     } else {
//         auto buf = static_cast<char*>(::malloc(frag.size));
//         if (!buf) {
//             throw std::bad_alloc();
//         }
//         deleter d = make_free_deleter(buf);
//         _frags[0] = { buf, frag.size };
//         _deleter.append(std::move(d));
//     }
//     std::copy(frag.base, frag.base + frag.size, _frags[0].base);
//     ++_nr_frags;
// }
//
// inline
// packet::packet()
//     : _impl(impl::allocate(1)) {
// }
//
// inline
// packet::packet(size_t nr_frags)
//     : _impl(impl::allocate(nr_frags)) {
// }
//
// inline
// packet::packet(fragment frag) : _impl(new impl(frag)) {
// }
//
// inline
// packet::packet(const char* data, size_t size) : packet(fragment{const_cast<char*>(data), size}) {
// }
//
// inline
// packet::packet(fragment frag, deleter d)
//     : _impl(impl::allocate(1)) {
//     _impl->_deleter = std::move(d);
//     _impl->_frags[_impl->_nr_frags++] = frag;
//     _impl->_len = frag.size;
// }
//
// inline
// packet::packet(std::vector<fragment> frag, deleter d)
//     : _impl(impl::allocate(frag.size())) {
//     _impl->_deleter = std::move(d);
//     std::copy(frag.begin(), frag.end(), _impl->_frags);
//     _impl->_nr_frags = frag.size();
//     _impl->_len = 0;
//     for (auto&& f : _impl->fragments()) {
//         _impl->_len += f.size;
//     }
// }
//
// template <typename Iterator>
// inline
// packet::packet(Iterator begin, Iterator end, deleter del) {
//     unsigned nr_frags = 0, len = 0;
//     nr_frags = std::distance(begin, end);
//     std::for_each(begin, end, [&] (const fragment& frag) { len += frag.size; });
//     _impl = impl::allocate(nr_frags);
//     _impl->_deleter = std::move(del);
//     _impl->_len = len;
//     _impl->_nr_frags = nr_frags;
//     std::copy(begin, end, _impl->_frags);
// }
//
// inline
// packet::packet(packet&& x, fragment frag)
//     : _impl(impl::allocate_if_needed(std::move(x._impl), 1)) {
//     _impl->_len += frag.size;
//     std::unique_ptr<char[]> buf(new char[frag.size]);
//     std::copy(frag.base, frag.base + frag.size, buf.get());
//     _impl->_frags[_impl->_nr_frags++] = {buf.get(), frag.size};
//     _impl->_deleter = make_deleter(std::move(_impl->_deleter), [buf = buf.release()] {
//         delete[] buf;
//     });
// }
//
// inline bool packet::allocate_headroom(size_t size) {
//     if (_impl->_headroom >= size) {
//         _impl->_len += size;
//         if (!_impl->using_internal_data()) {
//             _impl = impl::allocate_if_needed(std::move(_impl), 1);
//             std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
//                     _impl->_frags + _impl->_nr_frags + 1);
//             _impl->_frags[0] = { _impl->_data + internal_data_size, 0 };
//             ++_impl->_nr_frags;
//         }
//         _impl->_headroom -= size;
//         _impl->_frags[0].base -= size;
//         _impl->_frags[0].size += size;
//         return true;
//     } else {
//         return false;
//     }
// }
//
// inline packet::packet(fragment frag, packet&& x)
//     : _impl(std::move(x._impl)) {
//     // try to prepend into existing internal fragment
//     if (allocate_headroom(frag.size)) {
//         std::copy(frag.base, frag.base + frag.size, _impl->_frags[0].base);
//         return;
//     } else {
//         // didn't work out, allocate and copy
//         _impl->unuse_internal_data();
//         _impl = impl::allocate_if_needed(std::move(_impl), 1);
//         _impl->_len += frag.size;
//         std::unique_ptr<char[]> buf(new char[frag.size]);
//         std::copy(frag.base, frag.base + frag.size, buf.get());
//         std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
//                 _impl->_frags + _impl->_nr_frags + 1);
//         ++_impl->_nr_frags;
//         _impl->_frags[0] = {buf.get(), frag.size};
//         _impl->_deleter = make_deleter(std::move(_impl->_deleter),
//                 [buf = std::move(buf)] {});
//     }
// }
//
// inline
// packet::packet(packet&& x, fragment frag, deleter d)
//     : _impl(impl::allocate_if_needed(std::move(x._impl), 1)) {
//     _impl->_len += frag.size;
//     _impl->_frags[_impl->_nr_frags++] = frag;
//     d.append(std::move(_impl->_deleter));
//     _impl->_deleter = std::move(d);
// }
//
// inline
// packet::packet(packet&& x, deleter d)
//     : _impl(std::move(x._impl)) {
//     _impl->_deleter.append(std::move(d));
// }
//
// inline
// packet::packet(packet&& x, temporary_buffer<char> buf)
//     : packet(std::move(x), fragment{buf.get_write(), buf.size()}, buf.release()) {
// }
//
// inline
// packet::packet(temporary_buffer<char> buf)
//     : packet(fragment{buf.get_write(), buf.size()}, buf.release()) {}
//
// inline
// void packet::append(packet&& p) {
//     if (!_impl->_len) {
//         *this = std::move(p);
//         return;
//     }
//     _impl = impl::allocate_if_needed(std::move(_impl), p._impl->_nr_frags);
//     _impl->_len += p._impl->_len;
//     p._impl->unuse_internal_data();
//     std::copy(p._impl->_frags, p._impl->_frags + p._impl->_nr_frags,
//             _impl->_frags + _impl->_nr_frags);
//     _impl->_nr_frags += p._impl->_nr_frags;
//     p._impl->_deleter.append(std::move(_impl->_deleter));
//     _impl->_deleter = std::move(p._impl->_deleter);
// }
//
// inline
// char* packet::get_header(size_t offset, size_t size) {
//     if (offset + size > _impl->_len) {
//         return nullptr;
//     }
//     size_t i = 0;
//     while (i != _impl->_nr_frags && offset >= _impl->_frags[i].size) {
//         offset -= _impl->_frags[i++].size;
//     }
//     if (i == _impl->_nr_frags) {
//         return nullptr;
//     }
//     if (offset + size > _impl->_frags[i].size) {
//         linearize(i, offset + size);
//     }
//     return _impl->_frags[i].base + offset;
// }
//
// template <typename Header>
// inline
// Header* packet::get_header(size_t offset) {
//     return reinterpret_cast<Header*>(get_header(offset, sizeof(Header)));
// }
//
// inline
// void packet::trim_front(size_t how_much) {
//     assert(how_much <= _impl->_len);
//     _impl->_len -= how_much;
//     size_t i = 0;
//     while (how_much && how_much >= _impl->_frags[i].size) {
//         how_much -= _impl->_frags[i++].size;
//     }
//     std::copy(_impl->_frags + i, _impl->_frags + _impl->_nr_frags, _impl->_frags);
//     _impl->_nr_frags -= i;
//     if (!_impl->using_internal_data()) {
//         _impl->_headroom = internal_data_size;
//     }
//     if (how_much) {
//         if (_impl->using_internal_data()) {
//             _impl->_headroom += how_much;
//         }
//         _impl->_frags[0].base += how_much;
//         _impl->_frags[0].size -= how_much;
//     }
// }
//
// inline
// void packet::trim_back(size_t how_much) {
//     assert(how_much <= _impl->_len);
//     _impl->_len -= how_much;
//     size_t i = _impl->_nr_frags - 1;
//     while (how_much && how_much >= _impl->_frags[i].size) {
//         how_much -= _impl->_frags[i--].size;
//     }
//     _impl->_nr_frags = i + 1;
//     if (how_much) {
//         _impl->_frags[i].size -= how_much;
//         if (i == 0 && _impl->using_internal_data()) {
//             _impl->_headroom += how_much;
//         }
//     }
// }
//
// template <typename Header>
// Header*
// packet::prepend_header(size_t extra_size) {
//     auto h = prepend_uninitialized_header(sizeof(Header) + extra_size);
//     return new (h) Header{};
// }
//
// // prepend a header (uninitialized!)
// inline
// char* packet::prepend_uninitialized_header(size_t size) {
//     if (!allocate_headroom(size)) {
//         // didn't work out, allocate and copy
//         _impl->unuse_internal_data();
//         // try again, after unuse_internal_data we may have space after all
//         if (!allocate_headroom(size)) {
//             // failed
//             _impl->_len += size;
//             _impl = impl::allocate_if_needed(std::move(_impl), 1);
//             std::unique_ptr<char[]> buf(new char[size]);
//             std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
//                     _impl->_frags + _impl->_nr_frags + 1);
//             ++_impl->_nr_frags;
//             _impl->_frags[0] = {buf.get(), size};
//             _impl->_deleter = make_deleter(std::move(_impl->_deleter),
//                     [buf = std::move(buf)] {});
//         }
//     }
//     return _impl->_frags[0].base;
// }
//
// inline
// packet packet::share() {
//     return share(0, _impl->_len);
// }
//
// inline
// packet packet::share(size_t offset, size_t len) {
//     _impl->unuse_internal_data(); // FIXME: eliminate?
//     packet n;
//     n._impl = impl::allocate_if_needed(std::move(n._impl), _impl->_nr_frags);
//     size_t idx = 0;
//     while (offset > 0 && offset >= _impl->_frags[idx].size) {
//         offset -= _impl->_frags[idx++].size;
//     }
//     while (n._impl->_len < len) {
//         auto& f = _impl->_frags[idx++];
//         auto fsize = std::min(len - n._impl->_len, f.size - offset);
//         n._impl->_frags[n._impl->_nr_frags++] = { f.base + offset, fsize };
//         n._impl->_len += fsize;
//         offset = 0;
//     }
//     n._impl->_offload_info = _impl->_offload_info;
//     assert(!n._impl->_deleter);
//     n._impl->_deleter = _impl->_deleter.share();
//     return n;
// }
//
// }
//
// inline execution_stage::execution_stage(const sstring& name):_name(name){
//     internal::execution_stage_manager::get().register_execution_stage(*this);
//     auto undo = defer([&] { internal::execution_stage_manager::get().unregister_execution_stage(*this); });
//     undo.cancel();
// }
//
// inline execution_stage::~execution_stage()
// {
//     internal::execution_stage_manager::get().unregister_execution_stage(*this);
// }
//
// inline execution_stage::execution_stage(execution_stage&& other)
//     : _stats(other._stats)
//     , _name(std::move(other._name)){
//         internal::execution_stage_manager::get().update_execution_stage_registration(other, *this);
// }
//
// namespace net{
//     future<>
// posix_data_sink_impl::put(temporary_buffer<char> buf) {
//     return _fd->write_all(buf.get(), buf.size()).then([d = buf.release()] {});
// }
//
// future<>
// posix_data_sink_impl::put(packet p) {
//     _p = std::move(p);
//     return _fd->write_all(_p).then([this] { _p.reset(); });
// }
//
// future<>
// posix_data_sink_impl::close() {
//     _fd->shutdown(SHUT_WR);
//     return make_ready_future<>();
// }
//
// net::udp_channel::udp_channel()
// {}
//
// net::udp_channel::udp_channel(std::unique_ptr<udp_channel_impl> impl) : _impl(std::move(impl))
// {}
//
// net::udp_channel::~udp_channel()
// {}
//
// net::udp_channel::udp_channel(udp_channel&&) = default;
// net::udp_channel& net::udp_channel::operator=(udp_channel&&) = default;
//
// future<net::udp_datagram> net::udp_channel::receive() {
//     return _impl->receive();
// }
//
// future<> net::udp_channel::send(ipv4_addr dst, const char* msg) {
//     return _impl->send(std::move(dst), msg);
// }
//
// future<> net::udp_channel::send(ipv4_addr dst, packet p) {
//     return _impl->send(std::move(dst), std::move(p));
// }
//
// bool net::udp_channel::is_closed() const {
//     return _impl->is_closed();
// }
//
// void net::udp_channel::close() {
//     return _impl->close();
// }
//
// connected_socket::connected_socket()
// {}
//
// connected_socket::connected_socket(
//         std::unique_ptr<net::connected_socket_impl> csi)
//         : _csi(std::move(csi)) {
// }
//
// connected_socket::connected_socket(connected_socket&& cs) noexcept = default;
// connected_socket& connected_socket::operator=(connected_socket&& cs) noexcept = default;
//
// connected_socket::~connected_socket()
// {}
//
// input_stream<char> connected_socket::input() {
//     return input_stream<char>(_csi->source());
// }
//
// output_stream<char> connected_socket::output(size_t buffer_size) {
//     // TODO: allow user to determine buffer size etc
//     return output_stream<char>(_csi->sink(), buffer_size, false, true);
// }
//
// void connected_socket::set_nodelay(bool nodelay) {
//     _csi->set_nodelay(nodelay);
// }
//
// bool connected_socket::get_nodelay() const {
//     return _csi->get_nodelay();
// }
// void connected_socket::set_keepalive(bool keepalive) {
//     _csi->set_keepalive(keepalive);
// }
// bool connected_socket::get_keepalive() const {
//     return _csi->get_keepalive();
// }
// void connected_socket::set_keepalive_parameters(const net::keepalive_params& p) {
//     _csi->set_keepalive_parameters(p);
// }
// net::keepalive_params connected_socket::get_keepalive_parameters() const {
//     return _csi->get_keepalive_parameters();
// }
//
// void connected_socket::shutdown_output() {
//     _csi->shutdown_output();
// }
//
// void connected_socket::shutdown_input() {
//     _csi->shutdown_input();
// }
//
// net::socket::~socket()
// {}
//
// net::socket::socket(
//         std::unique_ptr<::net::socket_impl> si)
//         : _si(std::move(si)) {
// }
//
// net::socket::socket(net::socket&&) noexcept = default;
// net::socket& net::socket::operator=(net::socket&&) noexcept = default;
//
// future<connected_socket> net::socket::connect(socket_address sa, socket_address local, transport proto) {
//     return _si->connect(sa, local, proto);
// }
//
// void net::socket::shutdown() {
//     _si->shutdown();
// }
// server_socket::server_socket() {}
//
// server_socket::server_socket(std::unique_ptr<net::server_socket_impl> ssi)
//         : _ssi(std::move(ssi)) {}
// server_socket::server_socket(server_socket&& ss) noexcept = default;
// server_socket& server_socket::operator=(server_socket&& cs) noexcept = default;
// server_socket::~server_socket() {
// }
// future<connected_socket, socket_address> server_socket::accept() {
//     return _ssi->accept();
// }
//
// future<udp_datagram>
// posix_udp_channel::receive() {
//     _recv.prepare();
//     return _fd->recvmsg(&_recv._hdr).then([this] (size_t size) {
//         auto dst = ipv4_addr(_recv._cmsg.pktinfo.ipi_addr.s_addr, _address.port);
//         return make_ready_future<udp_datagram>(udp_datagram(std::make_unique<posix_datagram>(
//             _recv._src_addr, dst, packet(fragment{_recv._buffer, size}, make_deleter([buf = _recv._buffer] { delete[] buf; })))));
//     }).handle_exception([p = _recv._buffer](auto ep) {
//         delete[] p;
//         return make_exception_future<udp_datagram>(std::move(ep));
//     });
// }
//
//
//
// ipv4_addr::ipv4_addr(const std::string &addr) {
//     std::vector<std::string> items;
//     boost::split(items, addr, boost::is_any_of(":"));
//     if (items.size() == 1) {
//         ip = boost::asio::ip::address_v4::from_string(addr).to_ulong();
//         port = 0;
//     } else if (items.size() == 2) {
//         ip = boost::asio::ip::address_v4::from_string(items[0]).to_ulong();
//         port = std::stoul(items[1]);
//     } else {
//         throw std::invalid_argument("invalid format: " + addr);
//     }
// }
//
// ipv4_addr::ipv4_addr(const std::string &addr, uint16_t port_) : ip(boost::asio::ip::address_v4::from_string(addr).to_ulong()),port(port_) {}
//
// ipv4_addr::ipv4_addr(const seastar::net::inet_address& a, uint16_t port)
//     : ipv4_addr([&a] {
//   ::in_addr in = a;
//   return ::net::ntoh(in.s_addr);
// }(), port)
// {}
//
// }