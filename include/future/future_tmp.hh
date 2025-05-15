// // 添加promise<>的完整实现
// // 这应该放在原来只有声明的位置

// template<>
// class promise<> {
// private:
//     future<>* _future = nullptr;
//     future_state<> _local_state;
//     future_state<>* _state;
//     std::unique_ptr<task> _task;

// public:
//     promise() noexcept : _state(&_local_state) {}

//     promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
//         if (_state == &x._local_state) {
//             _state = &_local_state;
//             _local_state = std::move(x._local_state);
//         }
//         x._future = nullptr;
//         x._state = nullptr;
//         migrated();
//     }

//     promise(const promise&) = delete;
//     ~promise() noexcept {
//         abandoned();
//     }

//     promise& operator=(promise&& x) noexcept {
//         if (this != &x) {
//             this->~promise();
//             new (this) promise(std::move(x));
//         }
//         return *this;
//     }

//     void operator=(const promise&) = delete;
    
//     future<> get_future() noexcept {
//         return future<>(this);
//     }

//     void set_value() noexcept {
//         assert(_state);
//         _state->set();
//         make_ready<urgent::no>();
//     }
    
//     void set_urgent_value(std::tuple<>) noexcept {
//         assert(_state);
//         _state->set();
//         make_ready<urgent::yes>();
//     }
    
//     void set_exception(std::exception_ptr ex) noexcept {
//         assert(_state);
//         _state->set_exception(std::move(ex));
//         make_ready<urgent::no>();
//     }
    
//     void set_urgent_exception(std::exception_ptr ex) noexcept {
//         assert(_state);
//         _state->set_exception(std::move(ex));
//         make_ready<urgent::yes>();
//     }
    
//     template <typename Func>
//     void schedule(Func&& func) {
//         auto tws = std::make_unique<continuation<Func>>(std::forward<Func>(func));
//         _state = &tws->_state;
//         _task = std::move(tws);
//     }
    
//     template<urgent Urgent>
//     void make_ready() noexcept {
//         if (_future) {
//             if (_state == &_local_state) {
//                 _future->_local_state = std::move(*_state);
//                 _future->_promise = nullptr;
//             }
//             _future = nullptr;
//         }
//         if (_task) {
//             schedule_urgent(std::move(_task));
//         }
//     }
    
//     void migrated() noexcept {
//         if (_future) {
//             _future->_promise = this;
//         }
//     }
    
//     void abandoned() noexcept {
//         if (_future) {
//             assert(_state);
//             _future->_local_state = std::move(*_state);
//             _future->_promise = nullptr;
//         }
//     }
// };





// #ifdef __cpp_concepts
// #define GCC6_CONCEPT(x...) x
// #define GCC6_NO_CONCEPT(x...)
// #else
// #define GCC6_CONCEPT(x...)
// #define GCC6_NO_CONCEPT(x...) x
// #endif


// GCC6_CONCEPT(

// template <typename T>
// concept bool Future = is_future<T>::value;

// template <typename Func, typename... T>
// concept bool CanApply = requires (Func f, T... args) {
//     f(std::forward<T>(args)...);
// };
// template <typename Func, typename Return, typename... T>
// concept bool ApplyReturns = requires (Func f, T... args) {
//     { f(std::forward<T>(args)...) } -> Return;
// };
// template <typename Func, typename... T>
// concept bool ApplyReturnsAnyFuture = requires (Func f, T... args) {
//     requires is_future<decltype(f(std::forward<T>(args)...))>::value;
// };

// )



//    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
//     GCC6_CONCEPT( requires CanApply<Func, T...> )
//     Result
//     then(Func&& func) noexcept {
//         using futurator = futurize<std::result_of_t<Func(T&&...)>>;
//         // 如果当前 future,已经完成且不需要抢占.
//         if (available() && !need_preempt()) {
//             //调试的时候这里不会执行到,need_preempt永远为true.
//             if (failed()) {
//                 // 如果失败，传播异常
//                 return futurator::make_exception_future(get_available_state().get_exception());
//             } else {
//                 // 如果成功，执行回调函数
//                 return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
//             }
//         }
//         // 如果 future 还未完成，创建新的 promise 和 future
//         typename futurator::promise_type pr; // 这行代码是什么意思?
//         auto fut = pr.get_future();
//         try {
//             std::cout<<"开始执行schedule"<<std::endl;
//             //schedule接受一个lambda函数,捕捉pr和func,参数为state
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable
//             {
//                 //这个地方看不懂.auto &&state和state()有什么区别？为什么state不用引用捕获？
//                 if (state.failed()) {
//                     pr.set_exception(std::move(state).get_exception());
//                 }
//                 else{
//                     // 执行这个
//                     futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
//                     // futuator::apply首先执行func(value)，返回类型是T.
//                     // 返回一个 future<T>. 然后调用future的forward_to.
//                 }
//             });
//         } catch (...) {
//             abort();
//         }
//         return fut;
//     }




//     template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future)>>>
//     GCC6_CONCEPT( requires CanApply<Func, future> )
//     Result
//     then_wrapped(Func&& func) noexcept {
//         using futurator = futurize<std::result_of_t<Func(future)>>;
//         if (available() && !need_preempt()) {
//             return futurator::apply(std::forward<Func>(func), future(get_available_state()));
//         }
//         typename futurator::promise_type pr;
//         auto fut = pr.get_future();
//         try {
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
//                 futurator::apply(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
//             });
//         } catch (...) {
//             abort();
//         }
//         return fut;
//     }