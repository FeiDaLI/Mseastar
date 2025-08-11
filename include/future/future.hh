#pragma once
#ifndef FUTURE_H
#define FUTURE_H

#include <tuple>  
#include <exception>  
#include <type_traits>   
#include <utility>        
#include <cassert>      
#include <cstdlib>        
#include <memory>      
#include "../task/task.hh"

void schedule_normal(std::unique_ptr<task> t);
void schedule_urgent(std::unique_ptr<task> t);

template<typename T>
struct function_traits;

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
    using return_type = Ret;
    using args_as_tuple = std::tuple<Args...>;
    using signature = Ret (Args...);
    static constexpr std::size_t arity = sizeof...(Args);
    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity, "no such parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)>{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)>{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)>{};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>{};

template<typename T>
struct function_traits<T&> : public function_traits<std::remove_reference_t<T>>{};

template <typename... T> class future;
template <typename... T> class promise;
template <typename... T> struct future_state;


/// \cond internal
template <typename... T>
struct future_state {
    static constexpr bool copy_noexcept = std::is_nothrow_copy_constructible<std::tuple<T...>>::value;
    static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
                  "Types must be no-throw move constructible");
    static_assert(std::is_nothrow_destructible<std::tuple<T...>>::value,
                  "Types must be no-throw destructible");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    enum class state {
         invalid,
         future,
         result,
         exception,
    } _state = state::future;
    union any {
        any() {}
        ~any() {}
        std::tuple<T...> value;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept
            : _state(x._state) {
        switch (_state) {
        case state::future:
            break;
        case state::result:
            new (&_u.value) std::tuple<T...>(std::move(x._u.value));
            x._u.value.~tuple();
            break;
        case state::exception:
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
            break;
        case state::invalid:
            break;
        default:
            abort();
        }
        x._state = state::invalid;
    }
    __attribute__((always_inline))
    ~future_state() noexcept {
        switch (_state) {
        case state::invalid:
            break;
        case state::future:
            break;
        case state::result:
            _u.value.~tuple();
            break;
        case state::exception:
            _u.ex.~exception_ptr();
            break;
        default:
            abort();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        if (this != &x) {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept { return _state == state::result || _state == state::exception; }
    bool failed() const noexcept { return _state == state::exception; }
    void wait();
    void set(const std::tuple<T...>& value) noexcept {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(value);
        _state = state::result;
    }
    void set(std::tuple<T...>&& value) noexcept {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(std::move(value));
        _state = state::result;
    }
    template <typename... A>
    void set(A&&... a) {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(std::forward<A>(a)...);
        _state = state::result;
    }
    void set_exception(std::exception_ptr ex) noexcept {
        assert(_state == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        _state = state::exception;
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_state == state::exception);
        // Move ex out so future::~future() knows we've handled it
        _state = state::invalid;
        auto ex = std::move(_u.ex);
        _u.ex.~exception_ptr();
        return ex;
    }
    std::exception_ptr get_exception() const& noexcept {
        assert(_state == state::exception);
        return _u.ex;
    }
    std::tuple<T...> get_value() && noexcept {
        assert(_state == state::result);
        return std::move(_u.value);
    }
    template<typename U = std::tuple<T...>>
    std::enable_if_t<std::is_copy_constructible<U>::value, U> get_value() const& noexcept(copy_noexcept) {
        assert(_state == state::result);
        return _u.value;
    }
    std::tuple<T...> get() && {
        assert(_state != state::future);
        if (_state == state::exception) {
            _state = state::invalid;
            auto ex = std::move(_u.ex);
            _u.ex.~exception_ptr();
            // Move ex out so future::~future() knows we've handled it
            std::rethrow_exception(std::move(ex));
        }
        return std::move(_u.value);
    }
    std::tuple<T...> get() const& {
        assert(_state != state::future);
        if (_state == state::exception) {
            std::rethrow_exception(_u.ex);
        }
        return _u.value;
    }
    void ignore() noexcept {
        assert(_state != state::future);
        this->~future_state();
        _state = state::invalid;
    }
    using get0_return_type = std::tuple_element_t<0, std::tuple<T...>>;
    static get0_return_type get0(std::tuple<T...>&& x) {
        return std::get<0>(std::move(x));
    }
    void forward_to(promise<T...>& pr) noexcept;
        // assert(_state != state::future);
        // if (_state == state::exception) {
        //     pr.set_urgent_exception(std::move(_u.ex));
        //     _u.ex.~exception_ptr();
        // } else {
        //     pr.set_urgent_value(std::move(_u.value));
        //     _u.value.~tuple();
        // }
        // _state = state::invalid;
    //}
};

template <>
struct future_state<> {
    static_assert(sizeof(std::exception_ptr) == sizeof(void*), "exception_ptr not a pointer");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    static constexpr bool copy_noexcept = true;
    enum class state : uintptr_t {
         invalid = 0,
         future = 1,
         result = 2,
         exception_min = 3,  // or anything greater
    };
    union any {
        any() { 
            st = state::future; 
        }
        ~any() {}
        state st;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept { }
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept {
        if (x._u.st < state::exception_min) {
            _u.st = x._u.st;
        } else {
            // Move ex out so future::~future() knows we've handled it
            // Moving it will reset us to invalid state
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
        }
        x._u.st = state::invalid;
    }
    [[gnu::always_inline]]
    ~future_state() noexcept {
        if (_u.st >= state::exception_min) {
            _u.ex.~exception_ptr();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        if (this != &x) {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept { return _u.st == state::result || _u.st >= state::exception_min; }
    bool failed() const noexcept { return _u.st >= state::exception_min; }
    void set(const std::tuple<>& value) noexcept {
        assert(_u.st == state::future);
        _u.st = state::result;
    }
    void set(std::tuple<>&& value) noexcept {
        assert(_u.st == state::future);
        _u.st = state::result;
    }
    void set() {
        assert(_u.st == state::future);
        _u.st = state::result;//
    }
    void set_exception(std::exception_ptr ex) noexcept {
        assert(_u.st == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        assert(_u.st >= state::exception_min);
    }
    std::tuple<> get() && {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min) {
            // Move ex out so future::~future() knows we've handled it
            // Moving it will reset us to invalid state
            std::rethrow_exception(std::move(_u.ex));
        }
        return {};
    }
    std::tuple<> get() const& {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min) {
            std::rethrow_exception(_u.ex);
        }
        return {};
    }
    void ignore() noexcept {
        assert(_u.st != state::future);
        this->~future_state();
        _u.st = state::invalid;
    }
    using get0_return_type = void;
    static get0_return_type get0(std::tuple<>&&) {
        return;
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_u.st >= state::exception_min);
        // Move ex out so future::~future() knows we've handled it
        // Moving it will reset us to invalid state
        return std::move(_u.ex);
    }
    std::exception_ptr get_exception() const& noexcept {
        assert(_u.st >= state::exception_min);
        return _u.ex;
    }
    std::tuple<> get_value() const noexcept {
        assert(_u.st == state::result);
        return {};
    }
    void forward_to(promise<>& pr) noexcept;
};

template <typename Func, typename... T>
struct continuation final : task {
    continuation(Func&& func,future_state<T...>&& state,std::string name="unamed"):_state(std::move(state)),_func(std::move(func)),_name(std::move(name)){
    }
    continuation(Func&& func,std::string name = "unamed") : _func(std::move(func)),_name(std::move(name)) {
    }
    virtual void run() noexcept override {
        _func(std::move(_state));
    }
    future_state<T...> _state;
    Func _func;
    std::string _name;
};

template <class... T>
class promise;
template <class... T>
class future;
template<>
class promise<void>;
template <typename... T> struct is_future : std::false_type {};
template <typename... T> struct is_future<future<T...>> : std::true_type {};
struct ready_future_marker {};
struct ready_future_from_tuple_marker {};
struct exception_future_marker {};
template <typename T>
struct futurize;

template <typename T>
using futurize_t = typename futurize<T>::type;

template <typename... T, typename... A>
future<T...> make_ready_future(A&&... value);

template <typename... T>
future<T...> make_exception_future(std::exception_ptr value) noexcept;

template <typename T>
struct futurize {
    /// If \c T is a future, \c T; otherwise \c future<T>
    using type = future<T>;
    /// The promise type associated with \c type.
    using promise_type = promise<T>;
    /// The value tuple type associated with \c type
    using value_type = std::tuple<T>;

    /// Apply a function to an argument list (expressed as a tuple)
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    /// Apply a function to an argument list
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;
    /// Convert a value or a future to a future
    static inline type convert(T&& value) {
        return make_ready_future<T>(std::move(value));
    }
    // 如果convert传入的是值, 使用 make_ready_future 转为future类型

    static inline type convert(type&& value){
        return std::move(value);
    }
    // 如果convert传入的是future，直接把future使用std::move变为右值.

    /// Convert the tuple representation into a future
    static type from_tuple(value_type&& value);
    /// Convert the tuple representation into a future
    static type from_tuple(const value_type& value);

    /// Makes an exceptional future of type \ref type.
    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

/// \cond internal
template <>
struct futurize<void> {
    using type = future<>;
    using promise_type = promise<>;
    using value_type = std::tuple<>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

inline bool need_preempt();


template <typename... Args>
struct futurize<future<Args...>> {
    using type = future<Args...>;
    using promise_type = promise<Args...>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
    static inline type convert(type&& value) { return std::move(value); }

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};


#define GCC6_CONCEPT(x...)
#define GCC6_NO_CONCEPT(x...) x
GCC6_CONCEPT(
template <typename T>
concept Future = is_future<T>::value;

template <typename Func, typename... T>
concept CanApply = requires (Func f, T... args) {
    f(std::forward<T>(args)...);
};

template <typename Func, typename Return, typename... T>
concept ApplyReturns = requires (Func f, T... args) {
    { f(std::forward<T>(args)...) } -> std::convertible_to<Return>;
};

template <typename Func, typename... T>
concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
    requires is_future<decltype(f(std::forward<T>(args)...))>::value;
};
)

void engine_exit(std::exception_ptr eptr = {});
void report_failed_future(std::exception_ptr ex);

template <typename... T>
class future {
public:
    promise<T...>* _promise;
    future_state<T...> _local_state;  // valid if !_promise
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    
    future(promise<T...>* pr) noexcept : _promise(pr) {
        _promise->_future = this;
    }

    template <typename... A>
    future(ready_future_marker, A&&... a) : _promise(nullptr) {
        _local_state.set(std::forward<A>(a)...);
    }

    template <typename... A>
    future(ready_future_from_tuple_marker, std::tuple<A...>&& data) : _promise(nullptr) {
        _local_state.set(std::move(data));
    }

    future(exception_future_marker, std::exception_ptr ex) noexcept : _promise(nullptr) {
        _local_state.set_exception(std::move(ex));
    }

    [[gnu::always_inline]]
    explicit future(future_state<T...>&& state) noexcept
            : _promise(nullptr), _local_state(std::move(state)) {
    }

    [[gnu::always_inline]]
    future_state<T...>* state() noexcept {
        return _promise ? _promise->_state : &_local_state;
    }

    template <typename Func>
    void schedule(Func&& func,std::string name="unamed"){
        if (state()->available()) {
            ::schedule_normal(std::make_unique<continuation<Func, T...>>(std::move(func), std::move(*state()),std::move(name)));
        } else {
            //走这一条
            assert(_promise);
            _promise->schedule(std::move(func),name);
            _promise->_future = nullptr;
            _promise = nullptr;
        }
    }
    
    [[gnu::always_inline]]
    future_state<T...> get_available_state() noexcept {
        auto st = state();
        if (_promise) {
            _promise->_future = nullptr;
            _promise = nullptr;
        }
        return std::move(*st);
    }

    [[gnu::noinline]]
    future<T...> rethrow_with_nested() {
        if (!failed()) {
            return make_exception_future<T...>(std::current_exception());
        } else {
            std::nested_exception f_ex;
            try {
                get();
            } catch (...) {
                std::throw_with_nested(f_ex);
            }
        }
        assert(0 && "we should not be here");
    }

    template<typename... U>
    friend class shared_future;
public:
    /// \brief The data type carried by the future.
    using value_type = std::tuple<T...>;
    /// \brief The data type carried by the future.
    using promise_type = promise<T...>;
    /// \brief Moves the future into a new object.
    [[gnu::always_inline]]
    future(future&& x) noexcept : _promise(x._promise) {
        if (!_promise) {
            _local_state = std::move(x._local_state);
        }
        x._promise = nullptr;
        if (_promise) {
            _promise->_future = this;
        }
    }
    future(const future&) = delete;
    future& operator=(future&& x) noexcept {
        if (this != &x) {
            this->~future();
            new (this) future(std::move(x));
        }
        return *this;
    }
    void operator=(const future&) = delete;
    __attribute__((always_inline))
    ~future() {
        if (_promise) {
            _promise->_future = nullptr;
        }
        if (failed()) {
            report_failed_future(state()->get_exception());
        }
    }
    std::tuple<T...> get();

     std::exception_ptr get_exception() {
        return get_available_state().get_exception();
    }
    typename future_state<T...>::get0_return_type get0() {
        return future_state<T...>::get0(get());
    }

    /// \cond internal
    void wait();
    [[gnu::always_inline]]
    bool available() noexcept {
        return state()->available();
    }
    [[gnu::always_inline]]
    bool failed() noexcept {
        return state()->failed();
    }
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    GCC6_CONCEPT( requires CanApply<Func, T...> )
    Result
    then(Func&& func,std::string name = "unnamed") noexcept {
        using futurator = futurize<std::result_of_t<Func(T&&...)>>;
        // 如果当前 future,已经完成且不需要抢占.
        if (available() && !need_preempt()) {
            // 调试的时候这里不会执行到,need_preempt永远为true.
            if (failed()) {
                return futurator::make_exception_future(get_available_state().get_exception());
            } else {
                return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
            }
        }
        // 如果 future 还未完成，创建新的 promise 和 future
        typename futurator::promise_type pr; // 这行代码是什么意思?
        auto fut = pr.get_future();
        try {
            // std::cout<<"开始执行schedule"<<std::endl;
            // schedule接受一个lambda函数,捕捉pr和func,参数为state
            schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable
            {
                //这个地方看不懂.auto &&state和state()有什么区别？为什么state不用引用捕获？
                if (state.failed()) {
                    pr.set_exception(std::move(state).get_exception());
                }
                else{
                    // 执行这个
                    futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
                    // futuator::apply首先执行func(value)，返回类型是T.
                    // 返回一个 future<T>. 然后调用future的forward_to.
                }
            },std::move(name));
        }
        catch (...)
        {
            abort();
        }
        return fut;
    }
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future)>>>
    GCC6_CONCEPT( requires CanApply<Func, future> )
    Result
    then_wrapped(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(future)>>;
        if (available() && !need_preempt()) {
            return futurator::apply(std::forward<Func>(func), future(get_available_state()));
        }
        typename futurator::promise_type pr;
        auto fut = pr.get_future();
        try
        {
            schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
                futurator::apply(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
            });
        }
        catch(...)
        {
            abort();
        }
        return fut;
    }
    void forward_to(promise<T...>&& pr) noexcept {
        if (state()->available()) {
            // std::cout<<"state available future调用forward_to"<<std::endl;
            state()->forward_to(pr);

        }
        else
        {
            // std::cout<<"state unavailable future调用forward_to"<<std::endl;
            _promise->_future = nullptr;
            *_promise = std::move(pr);
            _promise = nullptr;
        }
    }
    template <typename Func>
    GCC6_CONCEPT( requires CanApply<Func> )
    future<T...> finally(Func&& func) noexcept {
        return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
    }


    template <typename Func, bool FuncReturnsFuture>
    struct finally_body;

    template <typename Func>
    struct finally_body<Func, true> {
        Func _func;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        { }

        future<T...> operator()(future<T...>&& result) {
            using futurator = futurize<std::result_of_t<Func()>>;
            return futurator::apply(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
                if (!f_res.failed()) {
                    return std::move(result);
                } else {
                    try {
                        f_res.get();
                    } catch (...) {
                        return result.rethrow_with_nested();
                    }
                    assert(0 && "we should not be here");
                }
            });
        }
    };

    template <typename Func>
    struct finally_body<Func, false> {
        Func _func;
        finally_body(Func&& func) : _func(std::forward<Func>(func))
        {}
        future<T...> operator()(future<T...>&& result) {
            try {
                _func();
                return std::move(result);
            } catch (...) {
                return result.rethrow_with_nested();
            }
        };
    };

    future<> or_terminate() noexcept {
        return then_wrapped([] (auto&& f) {
            try {
                f.get();
            } catch (...) {
                engine_exit(std::current_exception());
            }
        });
    }

    future<> discard_result() noexcept {
        return then([] (T&&...) {});
    }
    template <typename Func>
    future<T...> handle_exception(Func&& func) noexcept {
        using func_ret = std::result_of_t<Func(std::exception_ptr)>;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) -> future<T...> {
            if (!fut.failed()) {
                return make_ready_future<T...>(fut.get());
            } else {
                return futurize<func_ret>::apply(func, fut.get_exception());
            }
        });
    }
    template <typename Func>
    future<T...> handle_exception_type(Func&& func) noexcept {
        using trait = function_traits<Func>;
        static_assert(trait::arity == 1, "func can take only one parameter");
        using ex_type = typename trait::template arg<0>::type;
        using func_ret = typename trait::return_type;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) -> future<T...> {
            try {
                return make_ready_future<T...>(fut.get());
            } catch(ex_type& ex) {
                return futurize<func_ret>::apply(func, ex);
            }
        });
    }
    void ignore_ready_future() noexcept {
        state()->ignore();
    }
    /// \cond internal
    template <typename... U>
    friend class promise;
    template <typename... U, typename... A>
    friend future<U...> make_rFeady_future(A&&... value);
    template <typename... U>
    friend future<U...> make_exception_future(std::exception_ptr ex) noexcept;
    template <typename... U, typename Exception>
    friend future<U...> make_exception_future(Exception&& ex) noexcept;
    /// \endcondF
};







template <typename... T>
class promise {
public:
    enum class urgent { no, yes };
    future<T...>* _future = nullptr;
    future_state<T...> _local_state;
    future_state<T...>* _state;
    std::unique_ptr<task> _task;
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    /// \brief Constructs an empty \c promise.
    /// Creates promise with no associated future yet (see get_future()).
    promise() noexcept : _state(&_local_state) {}
    /// \brief Moves a \c promise object.
    promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
        if (_state == &x._local_state) {
            _state = &_local_state;
            _local_state = std::move(x._local_state);
        }//这段代码有什么用?
        x._future = nullptr;
        x._state = nullptr;
        if (_future){
            _future->_promise = this;
        }
    }
    promise(const promise&) = delete;
    __attribute__((always_inline))
    ~promise() noexcept {
        abandoned();
    }
    promise& operator=(promise&& x) noexcept {
        if (this != &x) {
            this->~promise();
            new (this) promise(std::move(x));
        }
        return *this;
    }
    void operator=(const promise&) = delete;

    future<T...> get_future() noexcept;

    void set_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
        do_set_value<urgent::no>(result);
    }

    void set_value(std::tuple<T...>&& result) noexcept {
        do_set_value<urgent::no>(std::move(result));
    }

    template <typename... A>
    void set_value(A&&... a) noexcept {
        assert(_state);
        _state->set(std::forward<A>(a)...);
        make_ready<urgent::no>();
    }

    void set_exception(std::exception_ptr ex) noexcept {
        do_set_exception<urgent::no>(std::move(ex));
    }

    template<typename Exception>
    void set_exception(Exception&& e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }

    template<urgent Urgent>
    void do_set_value(std::tuple<T...> result) noexcept {
        assert(_state);
        _state->set(std::move(result));
        make_ready<Urgent>();
    }

    void set_urgent_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
        do_set_value<urgent::yes>(result);
    }

    void set_urgent_value(std::tuple<T...>&& result) noexcept {
        do_set_value<urgent::yes>(std::move(result));
    }

    template<urgent Urgent>
    void do_set_exception(std::exception_ptr ex) noexcept {
        assert(_state);
        _state->set_exception(std::move(ex));
        make_ready<Urgent>();
    }

    void set_urgent_exception(std::exception_ptr ex) noexcept {
        do_set_exception<urgent::yes>(std::move(ex));
    }

    template <typename Func>
    void schedule(Func&& func,std::string name) {
        auto tws = std::make_unique<continuation<Func, T...>>(std::move(func),std::move(name));
        _state = &tws->_state;
        _task = std::move(tws);
    }

    template<urgent Urgent>
    void make_ready() noexcept;
    void abandoned() noexcept;
    template <typename... U>
    friend class future;
    friend class future_state<T...>;
};

template<>
class promise<void> : public promise<> {};
template <>
class future<void> : public future<> {};

#endif