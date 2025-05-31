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

// return fut.then([obj = std::move(obj)](){
//     return;
// });(fut类型不一定是<>，所以只能用then_wrapped)

