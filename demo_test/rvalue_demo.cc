#include <iostream>
#include <memory>

struct MyType {
    MyType() { std::cout << "Constructed\n"; }
    MyType(const MyType&) { std::cout << "Copy\n"; }
    MyType(MyType&&) noexcept { std::cout << "Move\n"; }
    ~MyType() { std::cout << "Destroyed\n"; }
};

int main() {
    auto ptr = std::make_unique<MyType>(MyType{}); // 右值 MyType{}
    // 输出:
    // Constructed (创建临时对象 MyType{})
    // Move (移动到 std::make_unique 分配的对象)
    // Destroyed (临时对象销毁)
    // ... (ptr 离开作用域时销毁堆上对象，输出 Destroyed)
    {
        MyType&& ref = MyType{}; // 右值引用绑定到临时对象
        std::cout << &ref <<std::endl; // 合法，输出临时对象的地址
    }
    // Constructed
    // 0x7ffd74027867

    return 0;
}