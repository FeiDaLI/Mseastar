// #pragma once
// #include<memory>
// #include <cassert>

// template <typename... T>
// class promise;
// // template <>
// // class promise<void>;


// template <typename... T>
// struct future_state {
//     enum class state {
//          invalid,
//          future,
//          result,
//          exception,
//     } _state = state::future;
//     //future可以理解为异步操作正在运行中，表示没有结果
//     union any {
//         any() noexcept {}
//         ~any(){}
//         std::tuple<T...> value;//需要理解这个是什么意思
//         std::exception_ptr ex;
//     } _u;

//     future_state() noexcept {}

//     //移动构造函数,如果是result和exception需要转移所有权(placement new + 析构函数)
//     future_state(future_state&& x) noexcept
//             : _state(x._state) {
//         switch (_state) {
//         case state::future:
//             break;
//         case state::result:
//             new (&_u.value) std::tuple<T...>(std::move(x._u.value));
//             x._u.value.~tuple();
//             break;
//         case state::exception:
//             new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
//             x._u.ex.~exception_ptr();
//             break;
//         case state::invalid:
//             break;
//         default:
//             abort();
//         }
//         x._state = state::invalid;
//     }

//     //析构函数(如果result和exception就调用析构函数)
//     ~future_state() noexcept {
//         switch (_state) {
//         case state::invalid:
//             break;
//         case state::future:
//             break;
//         case state::result:
//             _u.value.~tuple();
//             break;
//         case state::exception:
//             _u.ex.~exception_ptr();
//             break;
//         default:
//             abort();
//         }
//     }

//     //移动赋值运算符，调用移动构造函数
//     future_state& operator=(future_state&& x) noexcept {
//         if (this != &x) {
//             this->~future_state();
//             new (this) future_state(std::move(x));
//         }
//         return *this;
//     }

//     //如果是reuslt或者excepiton返回
//     bool available() const noexcept { return _state == state::result || _state == state::exception; }
//     bool failed() const noexcept { return _state == state::exception; }
//     void wait();

//     //设置值(传入的是左值)
//     void set(const std::tuple<T...>& value) noexcept {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(value);
//         _state = state::result;
//     }
//     //设置值(传入的是右值)，思考为什么不用通用引用？
//     void set(std::tuple<T...>&& value) noexcept {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(std::move(value));
//         _state = state::result;
//     }

//     //下面就是通用引用 (思考：只用这一个够吗？)
//     template <typename... A>
//     void set(A&&... a) {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(std::forward<A>(a)...);
//         _state = state::result;
//     }

//     void set_exception(std::exception_ptr ex) noexcept {
//         assert(_state == state::future);
//         new (&_u.ex) std::exception_ptr(ex);
//         _state = state::exception;
//     }

//     //只能由右值调用的函数(思考:为什么右值调用要移动)，转移所有权
//     std::exception_ptr get_exception() && noexcept {
//         assert(_state == state::exception);
//         // Move ex out so future::~future() knows we've handled it
//         _state = state::invalid;
//         auto ex = std::move(_u.ex);
//         _u.ex.~exception_ptr();
//         return ex;
//     }

    
//     std::exception_ptr get_exception() const& noexcept {
//         assert(_state == state::exception);
//         return _u.ex;
//     }
//     std::tuple<T...> get_value() && noexcept {
//         assert(_state == state::result);
//         return std::move(_u.value);
//     }


//     template<typename U = std::tuple<T...>>
//     U get_value() const& noexcept requires std::is_copy_constructible_v<U> {
//         assert(_state == state::result);
//         return _u.value;
//     }

//     std::tuple<T...> get() && {
//         //assert(_state != state::future);这行不应该去掉
//         if (_state == state::exception) {
//             _state = state::invalid;
//             auto ex = std::move(_u.ex);
//             _u.ex.~exception_ptr();
//             // Move ex out so future::~future() knows we've handled it
//             std::rethrow_exception(std::move(ex));
//         }
//         return std::move(_u.value);
//     }
//     std::tuple<T...> get() const& {
//         assert(_state != state::future);
//         if (_state == state::exception) {
//             std::rethrow_exception(_u.ex);
//         }
//         return _u.value;
//     }
//     //ignore有什么用
//     void ignore() noexcept {
//         assert(_state != state::future);
//         this->~future_state();
//         _state = state::invalid;
//     }

//     void forward_to(promise<T...>& pr) noexcept;
// };



// template <>
// struct future_state<> {
//     enum class state : uintptr_t {
//          invalid = 0,
//          future = 1,
//          result = 2,
//          exception_min = 3,  // or anything greater
//     };
//     union any {
//         any() { st = state::future; }
//         ~any() {}
//         state st;
//         std::exception_ptr ex;
//     } _u;
//     future_state() noexcept {}
//     future_state(future_state&& x) noexcept {
//         if (x._u.st < state::exception_min) {
//             _u.st = x._u.st;
//         } else {
//             // Move ex out so future::~future() knows we've handled it
//             // Moving it will reset us to invalid state
//             new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
//             x._u.ex.~exception_ptr();
//         }
//         x._u.st = state::invalid;
//     }
//     ~future_state() noexcept {
//         if (_u.st >= state::exception_min) {
//             _u.ex.~exception_ptr();
//         }
//     }
//     future_state& operator=(future_state&& x) noexcept {
//         if (this != &x) {
//             this->~future_state();
//             new (this) future_state(std::move(x));
//         }
//         return *this;
//     }
//     bool available() const noexcept { return _u.st == state::result || _u.st >= state::exception_min; }
//     bool failed() const noexcept { return _u.st >= state::exception_min; }
//     void set(const std::tuple<>& value) noexcept {
//         assert(_u.st == state::future);
//         _u.st = state::result;
//     }
//     void set(std::tuple<>&& value) noexcept {
//         assert(_u.st == state::future);
//         _u.st = state::result;
//     }
//     void set() {
//         assert(_u.st == state::future);
//         _u.st = state::result;
//     }
//     void set_exception(std::exception_ptr ex) noexcept {
//         assert(_u.st == state::future);
//         new (&_u.ex) std::exception_ptr(ex);
//         assert(_u.st >= state::exception_min);
//     }
//     std::tuple<> get() && {
//         assert(_u.st != state::future);
//         if (_u.st >= state::exception_min) {
//             std::rethrow_exception(std::move(_u.ex));
//         }
//         return {};
//     }
//     std::tuple<> get() const& {
//         assert(_u.st != state::future);
//         if (_u.st >= state::exception_min) {
//             std::rethrow_exception(_u.ex);
//         }
//         return {};
//     }
//     void ignore() noexcept {
//         assert(_u.st != state::future);
//         this->~future_state();
//         _u.st = state::invalid;
//     }
//     std::exception_ptr get_exception() && noexcept {
//         assert(_u.st >= state::exception_min);
//         // Move ex out so future::~future() knows we've handled it
//         // Moving it will reset us to invalid state
//         return std::move(_u.ex);
//     }
//     std::exception_ptr get_exception() const& noexcept {
//         assert(_u.st >= state::exception_min);
//         return _u.ex;
//     }
//     std::tuple<> get_value() const noexcept {
//         assert(_u.st == state::result);
//         return {};
//     }
//     void forward_to(promise<>& pr) noexcept;
// };

// // Explicit specialization for void
// template<>
// struct future_state<void> : public future_state<> {};

// // Add the forward_to implementation for void futures
// // template <typename... U>
// // void forward_to(promise<U...>& pr) noexcept {
// //     if (state._u.st >= future_state<>::state::exception_min) {
// //         pr.set_exception(std::move(state._u.ex));
// //     } else {
// //         pr.set();
// //     }
// //     state._u.st = future_state<>::state::invalid;
// // }
