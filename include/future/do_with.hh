#pragma once
#include <utility>
#include <memory>
#include <tuple>

template<typename T, typename F>
inline
auto do_with(T&& rvalue, F&& f) {
    auto obj = std::make_unique<T>(std::forward<T>(rvalue));
    auto fut = f(*obj);
    return fut.then_wrapped([obj = std::move(obj)] (auto&& fut) {
        return std::move(fut);
    });
}

/// \cond internal
template <typename Tuple, size_t... Idx>
inline
auto
cherry_pick_tuple(std::index_sequence<Idx...>, Tuple&& tuple) {
    return std::make_tuple(std::get<Idx>(std::forward<Tuple>(tuple))...);
}
/// \endcond

/// Executes the function \c func making sure the lock \c lock is taken,
/// and later on properly released.
///
/// \param lock the lock, which is any object having providing a lock() / unlock() semantics.
///        Caller must make sure that it outlives \ref func.
/// \param func function to be executed
/// \returns whatever \c func returns
template<typename Lock, typename Func>
inline
auto with_lock(Lock& lock, Func&& func) {
    return lock.lock().then([func = std::forward<Func>(func)] () mutable {
        return func();
    }).then_wrapped([&lock] (auto&& fut) {
        lock.unlock();
        return std::move(fut);
    });
}


template <typename T1, typename T2, typename T3_or_F, typename... More>
inline
auto
do_with(T1&& rv1, T2&& rv2, T3_or_F&& rv3, More&&... more) {
    auto all = std::forward_as_tuple(
            std::forward<T1>(rv1),
            std::forward<T2>(rv2),
            std::forward<T3_or_F>(rv3),
            std::forward<More>(more)...);
    constexpr size_t nr = std::tuple_size<decltype(all)>::value - 1;
    using idx = std::make_index_sequence<nr>;
    auto&& just_values = cherry_pick_tuple(idx(), std::move(all));
    auto&& just_func = std::move(std::get<nr>(std::move(all)));
    auto obj = std::make_unique<std::remove_reference_t<decltype(just_values)>>(std::move(just_values));
    auto fut = std::apply(just_func, *obj);
    return fut.then_wrapped([obj = std::move(obj)] (auto&& fut) {
        return std::move(fut);
    });
}

