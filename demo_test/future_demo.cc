#include<mutex>
#include<condition_variable>
#include<thread>

template <typename T>
struct future_state{
    enum class state{
        invaild,
        future,
        res
    };
    future_state(future_state& x)=delete;
    future_state& operator=(future_state&x)=delete;
    future_state(future_state&& x){
        s=x.s;
        value=std::move(x.value);
        x.value=nullptr;//这个地方有问题
    }
    state s=state::future;
    T value;
    std::mutex m;
    std::condition_variable cv;
    void set_value(T&& v){
        std::unique_lock<std::mutex> lk(m);
        if(s==state::future){
            value=std::move(v);
            s=state::res;
            cv.notify_one();
        }
    }
    T get(){
        std::unique_lock<std::mutex>lk(m);
        while(s==state::future){
            cv.wait(lk);
        }
        return std::move(value);
    }
};

template <typename T>
struct continuation{
    std::function<void()> f;
    future_state<T> res;
};

template <typename T>
struct futurize {
    using future_type = future<T>;
    using promise_type = promise<T>;
    using value_type = T;
    static inline future_type convert(T&& value) {
        return make_ready_future<T>(std::move(value));
    }
    static inline future_type convert(future_type&& value){
        return std::move(value);
    }
};

template <typename T>
struct future{
    future_state<T>* state;
    future_state<T> local_state;
    promise<T>*p;
    future(promise<T>& _p);
    future(future&& x);
    T get();
    template <typename F,typename R = futurize<std::invoke_result_t<F, T>>::type> //这里面的=是什么用法？
    R then(F&& f){
        using ret_type = std::invoke_result_t<F, T>
        if(state->s==state::res){
            return futurize<ret_type>::convert(std::apply(std::forward<F>(f),std::move(state->value)));
        }else{
            p->set_continuation(continuation<T>{
                [f=std::forward<F>(f),this](){
                    state->set_value(std::apply(std::forward<F>(f),std::move(state->value)));
                },
                std::move(local_state)
            });
            state->wait();
            return futurize<ret_type>::convert(std::apply(std::forward<F>(f),std::move(state->value)));
        }
    }
};
    //F(T)必须可以调用(暂时没有实现.)
template <typename T>
struct promise{
    future_state<T>* state;
    std::unique_ptr<continuation<T>> task;

};



template <typename T>
future<T>::future(promise<T>& _p):state(_p.state){
    p=_p;
    if(state==nullptr){
        state=&local_state;
    }else{
        state=p.state;
    }

}

template <typename T>
future<T>::future(future&& x):state(x.state){
    x.state=nullptr;
}

template <typename T>
T future<T>::get(){
    return state->get();
}

