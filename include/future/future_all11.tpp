// /* future/promise相关实现 */
// /// \brief Waits for the future to become available
// /// This method blocks the current thread until the future becomes available.
// template <typename... T>
// void future<T...>::wait() {
//     std::cout<<"future wait"<<std::endl;
//     auto thread = thread_impl::get();
//     assert(thread);//这里报错.
//
//     schedule([this, thread] (future_state<T...>&& new_state) {
//         *state() = std::move(new_state);
//         thread_impl::switch_in(thread);
//     });
//     thread_impl::switch_out(thread);
// }
//
// template <typename... T>
// [[gnu::always_inline]]
// std::tuple<T...> future<T...>::get() {
//     if (!state()->available()) {
//         std::cout<<"future.get  调用这里的wait"<<std::endl;
//         wait();
//     } else if (thread_impl::get() && thread_impl::should_yield()) {
//         std::cout<<"future.get  should_yield"<<std::endl;
//         thread_impl::yield();
//     }
//     return get_available_state().get();
// }
//
// template <typename T>
// template <typename Arg>
// inline
// future<T>
// futurize<T>::make_exception_future(Arg&& arg) {
//     return ::make_exception_future<T>(std::forward<Arg>(arg));
// }
//
// template <typename... T>
// template <typename Arg>
// inline
// future<T...>
// futurize<future<T...>>::make_exception_future(Arg&& arg) {
//     return ::make_exception_future<T...>(std::forward<Arg>(arg));
// }
//
// template <typename Arg>
// inline
// future<>
// futurize<void>::make_exception_future(Arg&& arg) {
//     return ::make_exception_future<>(std::forward<Arg>(arg));
// }
//
// template <typename T>
// inline
// future<T>
// futurize<T>::from_tuple(std::tuple<T>&& value) {
//     return make_ready_future<T>(std::move(value));
// }
//
// template <typename T>
// inline
// future<T>
// futurize<T>::from_tuple(const std::tuple<T>& value) {
//     return make_ready_future<T>(value);
// }
// inline future<> futurize<void>::from_tuple(std::tuple<>&& value) {
//     return make_ready_future<>();
// }
//
// inline future<> futurize<void>::from_tuple(const std::tuple<>& value) {
//     return make_ready_future<>();
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
// /*
//  *reactor实现
//  */
// template <typename T, typename E, typename EnableFunc>
// void reactor::complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
//     expired_timers = timers.expire(timers.now()); // 获取过期的定时器
//     std::cout << "Expired " << expired_timers.size() << " timers" << std::endl;
//     // 处理所有过期定时器
//     for (auto* timer_ptr : expired_timers) {
//         if (timer_ptr) {
//             // std::cout << "Marking timer " << " as expired" << std::endl;
//             timer_ptr->_expired = true;
//         }
//     }
//     // arm表示定时器是否有一个过期时间.
//     while (!expired_timers.empty()) {
//         auto* timer_ptr = expired_timers.front();
//         expired_timers.pop_front();
//         if (timer_ptr) {
//             // std::cout << "Processing timer "<<std::endl;
//             timer_ptr->_queued = false;
//             if (timer_ptr->_armed) {
//                 timer_ptr->_armed = false;
//                 if (timer_ptr->_period) {
//                     // std::cout << "Re-adding periodic timer " << timer_ptr->_timerid << std::endl;
//                     timer_ptr->readd_periodic();//周期定时器
//
//                 }
//                 try {
//                     // std::cout << "Executing timer callback for " << std::endl;
//                     timer_ptr->_callback();
//                 } catch (const std::exception& e) {
//                     // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed: " << e.what() << std::endl;
//                 } catch (...) {
//                     // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed with unknown error" << std::endl;
//                 }
//             }
//         }
//     }
//
//     enable_fn();
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
//
//
//
//
//
// namespace net{
//
//     future<> posix_data_source_impl::close() {
//         _fd->shutdown(SHUT_RD);
//         return make_ready_future<>();
//     }
// }