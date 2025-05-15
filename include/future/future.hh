// #pragma once
// #include "future_state.hh"
// #include "promise.hh"
// // Include task header but not continuation.hh yet
// #include "../task/task.hh"

// // Forward declarations
// template <typename T>
// struct futurize;

// template <typename T>
// using futurize_t = typename futurize<T>::type;

// /*下面的技巧很常见*/
// struct ready_future_marker {};
// struct ready_future_from_tuple_marker {};
// struct exception_future_marker {};

// // Forward declaration of the future class itself
// template <typename... T>
// class future;

// // Forward declaration of promise
// template <typename... T>
// class promise;

// // Now we can define is_future
// template <typename T> struct is_future : std::false_type {};
// template <typename... T> struct is_future<future<T...>> : std::true_type {};
// template <typename T>
// inline constexpr bool is_future_v = is_future<T>::value;

// void report_failed_future(std::exception_ptr eptr);

// // Now include continuation.hh after the forward declarations
// #include "../task/continuation.hh"

// template <typename... T>
// class future {
// public:
//     using value_type = std::tuple<T...>;
//     using promise_type = promise<T...>;
//     using future_state_type = future_state<T...>;
//     promise_type* _promise;
//     future_state_type _local_state;
// /*----------------------------------构造函数和析构函数-----------------------------------------------------*/
//     future(promise_type* pr) noexcept : _promise(pr) {
//         _promise->_future = this;
//     }

//     /*设置值*/
//     template <typename... A>
//     future(ready_future_marker, A&&... a) : _promise(nullptr) {
//         _local_state.set(std::forward<A>(a)...);
//     }
    
//     template <typename... A>
//     future(ready_future_from_tuple_marker, std::tuple<A...>&& data) : _promise(nullptr) {
//         _local_state.set(std::move(data));
//     }

//     future(exception_future_marker, std::exception_ptr ex) noexcept : _promise(nullptr) {
//         _local_state.set_exception(std::move(ex));
//     }

//     explicit future(future_state_type&& state) noexcept
//             : _promise(nullptr), _local_state(std::move(state)) {
//     }



//     future(const future&) = delete;
//     void operator=(const future&) = delete;

//     //这个地方没有看懂
//     future(future&& x) noexcept : _promise(x._promise) {
//         if (!_promise) {
//             _local_state = std::move(x._local_state);
//         }
//         x._promise = nullptr;
//         if (_promise) {
//             _promise->_future = this;
//         }
//     }


//     future& operator=(future&& x) noexcept {
//         if (this != &x) {
//             this->~future();
//             new (this) future(std::move(x));
//         }
//         return *this;
//     }

//     ~future() {
//         if (_promise) {
//             _promise->_future = nullptr;
//         }
//         if (failed()) {
//             report_failed_future(state()->get_exception());
//         }
//     }
//  /*------------------------------------------------------------------------------------------*/
 
//     future_state_type* state() noexcept {
//         return _promise ? _promise->_state : &_local_state;
//     }
//     bool available() noexcept {
//         return state()->available();
//     }

//     bool failed() noexcept {
//         return state()->failed();
//     }

//     future_state_type get_available_state() noexcept {
//         auto st = state();
//         if (_promise) {
//             _promise->_future = nullptr;
//             _promise = nullptr;
//         }
//         return std::move(*st);
//     }

//     //这个Func是从哪来的？
//     template <typename Func>
//     void schedule(Func&& func){
//         if (state()->available()){
//             auto ptr = std::make_unique<continuation<Func, T...>>(std::move(func), std::move(*state()));
//             // Explicitly cast to task* for proper conversion
//             schedule_urgent(std::unique_ptr<task>(static_cast<task*>(ptr.release())));
//         } else {
//             assert(_promise);
//             _promise->schedule(std::move(func));
//             _promise->_future = nullptr;
//             _promise = nullptr; //清空两个双向指针
//         }
//     }
//     // Helper method to determine if preemption is needed
//     bool need_preempt() const {
//         return false; // Simplified implementation for now
//     }

//     future<T...> rethrow_with_nested() {
//         if (!failed()) {
//             return make_exception_future<T...>(std::current_exception());
//         } else {
//             std::nested_exception f_ex;
//             try {
//                 get();
//             } catch (...) {
//                 std::throw_with_nested(f_ex);
//             }
//         }
//         assert(0 && "we should not be here");
//     }

//     std::tuple<T...> get() {
//         if (!state()->available()) {
//             wait();
//         } else if (thread_impl::get() && thread_impl::should_yield()) {
//             thread_impl::yield();
//         }
//         return get_available_state().get();
//     }

//      std::exception_ptr get_exception() {
//         return get_available_state().get_exception();
//     }

//     void wait() {
//         auto thread = thread_impl::get();
//         assert(thread);
//         schedule([this, thread] (future_state_type<T...>&& new_state) {
//              *state() = std::move(new_state);
//              thread_impl::switch_in(thread);
//         });
//         thread_impl::switch_out(thread);
//     }

// // Helper function to handle exceptions
// void engine_exit(std::exception_ptr ep) {
//     try {
//         if (ep) {
//             std::rethrow_exception(ep);
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "Exiting on exception: " << e.what() << std::endl;
//     } catch (...) {
//         std::cerr << "Exiting on unknown exception" << std::endl;
//     }
//     std::abort();
// }

// template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
// Result
// then(Func&& func) noexcept {
//         using futurator = futurize<std::result_of_t<Func(T&&...)>>;
//         if (available() && !need_preempt()) {
//             if (failed()) {
//                 return futurator::make_exception_future(get_available_state().get_exception());
//             } else {
//                 return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
//             }
//         }
//         typename futurator::promise_type pr;
//         auto fut = pr.get_future();
//         try {
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
//                 if (state.failed()) {
//                     pr.set_exception(std::move(state).get_exception());
//                 } else {
//                     futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
//                     //这段代码有问题.
//                 }
//             });
//         } catch (...) {
//             // catch possible std::bad_alloc in schedule() above
//             // nothing can be done about it, we cannot break future chain by returning
//             // ready future while 'this' future is not ready
//             abort();
//         }
//         return fut;
//     }

//     template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future<T...>)>>>
//     Result
//     then_wrapped(Func&& func) noexcept {
//         // using futurator = futurize<std::result_of_t<Func(future<T...>)>>;
//         using futurator = futurize<std::result_of_t<Func(future<T...>)>>;
//         if (available() && !need_preempt()) {
//             return futurator::apply(std::forward<Func>(func), future<T...>(get_available_state()));
//         }
//         typename futurator::promise_type pr;
//         auto fut = pr.get_future();
//         try {
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
//                 futurator::apply(std::forward<Func>(func), future<T...>(std::move(state))).forward_to(std::move(pr));
//             });
//         } catch (...) {
//             // catch possible std::bad_alloc in schedule() above
//             // nothing can be done about it, we cannot break future chain by returning
//             // ready future while 'this' future is not ready
//             abort();
//         }
//         return fut;
//     }

    
//     template <typename Func>
//     future<T...> finally(Func&& func) noexcept {
//         return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
//     }

//     template <typename Func, bool FuncReturnsFuture>
//     struct finally_body;

//     template <typename Func>
//     struct finally_body<Func, true> {
//         Func _func;

//         finally_body(Func&& func) : _func(std::forward<Func>(func))
//         { }

//         future<T...> operator()(future<T...>&& result) {
//             using futurator = futurize<std::invoke_result_t<Func>>;
//             return futurator::apply(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
//                 if (!f_res.failed()) {
//                     return std::move(result);
//                 } else {
//                     try {
//                         f_res.get();
//                     } catch (...) {
//                         return result.rethrow_with_nested();
//                     }
//                     assert(0 && "we should not be here");
//                 }
//             });
//         }
//     };

//     template <typename Func>
//     struct finally_body<Func, false> {
//         Func _func;

//         finally_body(Func&& func) : _func(std::forward<Func>(func))
//         { }

//         future<T...> operator()(future<T...>&& result) {
//             try {
//                 _func();
//                 return std::move(result);
//             } catch (...) {
//                 return result.rethrow_with_nested();
//             }
//         };
//     };


//     future<> or_terminate() noexcept {
//         return then_wrapped([] (auto&& f) {
//             try {
//                 f.get();
//             } catch (...) {
//                 engine_exit(std::current_exception());
//             }
//         });
//     }

//     future<> discard_result() noexcept {
//         return then([] (T&&...) {});
//     }

//     // Helper struct to extract function traits
//     template <typename F>
//     struct function_traits : public function_traits<decltype(&F::operator())> {};

//     // For regular functions
//     template <typename R, typename... Args>
//     struct function_traits<R(Args...)> {
//         using return_type = R;
//         static constexpr size_t arity = sizeof...(Args);
        
//         template <size_t N>
//         struct arg {
//             using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
//         };
//     };

//     // For function pointers
//     template <typename R, typename... Args>
//     struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

//     // For member function pointers
//     template <typename C, typename R, typename... Args>
//     struct function_traits<R(C::*)(Args...)> : public function_traits<R(Args...)> {};

//     // For const member function pointers
//     template <typename C, typename R, typename... Args>
//     struct function_traits<R(C::*)(Args...) const> : public function_traits<R(Args...)> {};

//     // For lambdas
//     template <typename C, typename R, typename... Args>
//     struct function_traits<R(C::*)(Args...) const noexcept> : public function_traits<R(Args...)> {};

//     template <typename Func>
//     future<T...> handle_exception(Func&& func) noexcept {
//         using func_ret = std::result_of_t<Func(std::exception_ptr)>;
//         return then_wrapped([func = std::forward<Func>(func)]
//                              (auto&& fut) -> future<T...> {
//             if (!fut.failed()) {
//                 return make_ready_future<T...>(fut.get());
//             } else {
//                 return futurize<func_ret>::apply(func, fut.get_exception());
//             }
//         });
//     }

//     template <typename Func>
//     future<T...> handle_exception_type(Func&& func) noexcept {
//         using trait = function_traits<Func>;
//         static_assert(trait::arity == 1, "func can take only one parameter");
//         using ex_type = typename trait::template arg<0>::type;
//         using func_ret = typename trait::return_type;
//         return then_wrapped([func = std::forward<Func>(func)]
//                              (auto&& fut) -> future<T...> {
//             try {
//                 return make_ready_future<T...>(fut.get());
//             } catch(ex_type& ex) {
//                 return futurize<func_ret>::apply(func, ex);
//             }
//         });
//     }


//     void ignore_ready_future() noexcept {
//         state()->ignore();
//     }
// };

// // Explicit specialization for future<void>
// template<>
// class future<void> : public future<> {
// public:
//     // Inherit constructors from base class
//     using future<>::future;
    
//     // Add explicit conversion constructor from future<>
//     future(future<>&& f) noexcept : future<>(std::move(f)) {}
// };

// /*
// 辅助函数，根据标签类直接设置future的state.
// */

// template <typename... T, typename... A>
// inline
// future<T...> make_ready_future(A&&... value) {
//     return future<T...>(ready_future_marker(), std::forward<A>(value)...);
// }


// template <typename... T>
// inline
// future<T...> make_exception_future(std::exception_ptr ex) noexcept {
//     return future<T...>(exception_future_marker(), std::move(ex));
// }

// template <typename... T, typename Exception>
// inline
// future<T...> make_exception_future(Exception&& ex) noexcept {
//     return make_exception_future<T...>(std::make_exception_ptr(std::forward<Exception>(ex)));
// }
