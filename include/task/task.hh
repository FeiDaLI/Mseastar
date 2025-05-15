
#pragma once

#include <memory>

class task {
public:
    virtual ~task() noexcept {}
    virtual void run() noexcept = 0;
};

//为什么要把func封装为lambda_task?多一层引用有什么用？
template <typename Func>
class lambda_task final : public task {
    Func _func;
public:
    template<typename F>
    lambda_task(F&& func) : _func(std::forward<F>(func)) {}
    virtual void run() noexcept override { _func(); }
};

template <typename Func>
inline
std::unique_ptr<task>
make_task(Func&& func) {
    return std::make_unique<lambda_task<Func>>(std::forward<Func>(func));
}
//这里是通用引用，因为发生了类型推导。

/*  注意这里再模板中使用make_unique的方法. */




