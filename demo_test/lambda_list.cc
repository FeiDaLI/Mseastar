#include<iostream>
#include<functional>
#include<memory>

struct task
{
    std::function<void()> f;
    std::unique_ptr<task> next;

    template<typename F>
    task(F &&fun):f(std::forward<F>(fun)){}

    template<typename F>
    task& then(F &&fun){
        next = std::make_unique<task>(std::forward<F>(fun));
        return *next;
    }
};

int main(){
    task t1([](){std::cout<<"task1"<<std::endl;});
    t1.then([](){std::cout<<"task2"<<std::endl;}).
        then([](){std::cout<<"task3"<<std::endl;}).
            then([](){std::cout<<"task4"<<std::endl;});
    t1.f();
    t1.next->f();
    t1.next->next->f();
    t1.next->next->next->f();
    
}