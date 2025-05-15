// #pragma once
// #include <tuple>
// #include <exception>
// #include <memory>
// #include <utility>
// #include <type_traits>
// #include <cassert>
// #include <boost/optional.hpp>
// #include <boost/variant.hpp>
// #include <boost/any.hpp>
// #include <functional>
// #include <boost/range/adaptor/transformed.hpp>
// #include <boost/range/algorithm/copy.hpp>
// #include <boost/range/algorithm/for_each.hpp>
// // Include only what's needed directly
// #include "future_state.hh"
// #include "../task/task.hh"
// // #include "../task/schedule_.hh"

// // Forward declarations
// template <typename... T> class future;
// class task;

// template <typename Func, typename... T>
// struct continuation;

// void schedule_urgent(std::unique_ptr<task> t);
// void report_failed_future(std::exception_ptr eptr);

// template <typename... T>
// class promise {
// public:
//     enum class urgent { no, yes };
//     future<T...>* _future = nullptr;
//     future_state<T...> _local_state;
//     future_state<T...>* _state;
//     // 注意模板中指针的写法
//     std::unique_ptr<task> _task;
//     // promise和一个task关联,这个task是什么?为什么要和task关联?这个task什么时候执行?什么时候初始化?

// public:
//     /// \brief Constructs an empty \c promise.
//     /// Creates promise with no associated future yet(see get_future()).
//     promise() noexcept : _state(&_local_state) {}

// //右值构造 
//     promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
//         if (_state == &x._local_state) {
//             _state = &_local_state;
//             _local_state = std::move(x._local_state);
//         }
//         x._future = nullptr;
//         x._state = nullptr;
//         migrated();
//     }//这个地方看不懂
//     //不能拷贝构造
//     promise(const promise&) = delete;
//     ~promise() noexcept {
//         abandoned();
//     }
//     promise& operator=(promise&& x) noexcept {
//         if (this != &x) {
//             this->~promise();
//             new (this) promise(std::move(x));//为什么要调用placement new?为什么我在其他地方的构造函数就没有看到？
//         }
//         return *this;
//     }
//     void operator=(const promise&) = delete;

//     /*
//         get_future()通过构造函数构造一个future并返回,
//         构造函数就是把promise的future指针指向future,把future的promise指针指向promise。
//         这样future和promise就关联起来了。
//     */
//     future<T...> get_future() noexcept{
//         //如果future是空指针,task是空指针，
//         assert(!_future && _state && !_task);
//         return future<T...>(this);
//     }

//     void set_value(const std::tuple<T...>& result) noexcept {
//         do_set_value<urgent::no>(result);
//     }

//     void set_value(std::tuple<T...>&& result) noexcept {
//         do_set_value<urgent::no>(std::move(result));
//     }

//     template <typename... A>
//     void set_value(A&&... a) noexcept {
//         assert(_state);
//         _state->set(std::forward<A>(a)...);
//         make_ready<urgent::no>();
//     }

//     /// \brief Marks the promise as failed
//     /// Forwards the exception argument to the future and makes it
//     /// available. May be called either before or after \c get_future().
//     void set_exception(std::exception_ptr ex) noexcept {
//         do_set_exception<urgent::no>(std::move(ex));
//     }

//     /// \brief Marks the promise as failed
//     ///
//     /// Forwards the exception argument to the future and makes it
//     /// available.  May be called either before or after \c get_future().
//     template<typename Exception>
//     void set_exception(Exception&& e) noexcept {
//         set_exception(make_exception_ptr(std::forward<Exception>(e)));
//     }

//     // Move these to public since they need to be accessed by future_state
//     void set_urgent_value(const std::tuple<T...>& result) noexcept {
//         do_set_value<urgent::yes>(result);
//     }

//     void set_urgent_value(std::tuple<T...>&& result) noexcept {
//         do_set_value<urgent::yes>(std::move(result));
//     }

//     void set_urgent_exception(std::exception_ptr ex) noexcept {
//         do_set_exception<urgent::yes>(std::move(ex));
//     }

//     template<urgent Urgent>
//     void do_set_value(std::tuple<T...> result) noexcept {
//         assert(_state);
//         _state->set(std::move(result));
//         make_ready<Urgent>();
//     }

//     template<urgent Urgent>
//     void do_set_exception(std::exception_ptr ex) noexcept {
//         assert(_state);
//         _state->set_exception(std::move(ex));
//         make_ready<Urgent>();
//     }

//     template <typename Func>
//     void schedule(Func&& func) {
//         auto tws = std::make_unique<continuation<Func, T...>>(std::move(func));
//         _state = &tws->_state;
//         // Explicitly cast to std::unique_ptr<task> to ensure proper conversion
//         _task = std::unique_ptr<task>(static_cast<task*>(tws.release()));
//     }
//     template<urgent Urgent>
//     void make_ready() noexcept;
//     void migrated() noexcept{
//         if(_future){
//             _future->_promise = this;
//         }
//     }
//     void abandoned() noexcept{
//         if(_future) {
//             assert(_state);
//             assert(_state->available() || !_task);
//             _future->_local_state = std::move(*_state);
//             _future->_promise = nullptr;
//         }else if(_state && _state->failed()) {
//             report_failed_future(_state->get_exception());
//         }
//     }
// };


// template<>
// class promise<void> : public promise<>{};

// void report_failed_future(std::exception_ptr eptr) {
//     throw("Exceptional future");
// }

// /*
//     promise中 make_ready 如果 task 不为空,就把 task 加到调度独立里面. 
// */

// template <typename... T>
// template<typename promise<T...>::urgent Urgent>
// inline void promise<T...>::make_ready() noexcept {
//     if (_task) {
//         _state = nullptr;
//         if (Urgent == urgent::yes){
//             schedule_urgent(std::move(_task));
//         } else {
//             schedule(std::move(_task));
//         }
//     }
// }

// template <typename... T>
// void future_state<T...>::forward_to(promise<T...>& pr) noexcept {
//     assert(_state != state::future);
//     if (_state == state::exception) {
//         pr.set_urgent_exception(std::move(_u.ex));
//         _u.ex.~exception_ptr();
//     } else {
//         pr.set_urgent_value(std::move(_u.value));
//         _u.value.~tuple();
//     }
//     _state = state::invalid;
// }

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
