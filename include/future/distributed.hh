#pragma once
#include "../util/bool_class.hh"
#include <tuple>
#include <iterator>
#include <vector>
#include <optional>
#include "future.hh"
#include <boost/range/irange.hpp>
#include "future_all12.hh"
#include "smp.hh"

// #include "util/tuple_utils.hh"
extern __thread size_t task_quota;
struct parallel_for_each_state {
    // use optional<> to avoid out-of-line constructor
    std::optional<std::exception_ptr> ex;
    size_t waiting = 0;
    promise<> pr;
    void complete() {
        if (--waiting == 0) {
            if (ex) {
                pr.set_exception(std::move(*ex));
            } else {
                pr.set_value();
            }
        }
    }
};

template <typename Iterator, typename Func>
GCC6_CONCEPT(requires requires (Func f, Iterator i) { { f(*i++) } -> std::same_as<future<>>; })
inline future<>
parallel_for_each(Iterator begin, Iterator end, Func&& func) {
    if (begin == end) {
        return make_ready_future<>();
    }
    return do_with(parallel_for_each_state{}, [&] (parallel_for_each_state& state) -> future<> {
        // increase ref count to ensure all functions run
        ++state.waiting;
        while (begin != end) {
            ++state.waiting;
            try {
                func(*begin++).then_wrapped([&] (auto&& f) {
                    if (f.failed()) {
                        // We can only store one exception.  For more, use when_all().
                        if (!state.ex) {
                            state.ex = f.get_exception();
                        } else {
                            f.ignore_ready_future();
                        }
                    }
                    state.complete();
                });
            } catch (...) {
                if (!state.ex)state.ex = std::move(std::current_exception());
                state.complete();
            }
        }
        // match increment on top
        state.complete();
        return state.pr.get_future();
    });
}


template <typename Range, typename Func>
GCC6_CONCEPT(requires requires (Func f, Range r) { { f(*r.begin()) } -> std::same_as<future<>>; })
inline
future<>
parallel_for_each(Range&& range, Func&& func) {
    return parallel_for_each(std::begin(range), std::end(range),
            std::forward<Func>(func));
}


template<typename AsyncAction, typename StopCondition>
static inline
void do_until_continued(StopCondition&& stop_cond, AsyncAction&& action, promise<> p) {
    while (!stop_cond()) {
        try {
            auto&& f = action();
            if (!f.available() || need_preempt()) {
                f.then_wrapped([action = std::forward<AsyncAction>(action),
                                stop_cond = std::forward<StopCondition>(stop_cond),
                                p = std::move(p)]  // 修复：移动捕获p
                                (std::result_of_t<AsyncAction()> fut) mutable {
                    if (!fut.failed()) {
                        do_until_continued(std::forward<StopCondition>(stop_cond),
                                          std::forward<AsyncAction>(action),
                                          std::move(p));  // 修复：移动p
                    } else {
                        p.set_exception(fut.get_exception());  // 此时p已经被捕获
                    }
                });
                return;
            }
            if (f.failed()) {
                f.forward_to(std::move(p));
                return;
            }
        } catch (...) {
            p.set_exception(std::current_exception());
            return;
        }
    }
    p.set_value();
}

struct stop_iteration_tag { };
using stop_iteration = bool_class<stop_iteration_tag>;

template<typename AsyncAction>
GCC6_CONCEPT( requires ApplyReturns<AsyncAction, stop_iteration> || ApplyReturns<AsyncAction, future<stop_iteration>> )
static inline
future<> repeat(AsyncAction&& action) {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    static_assert(std::is_same<future<stop_iteration>, typename futurator::type>::value, "bad AsyncAction signature");
    try {
        do {
            auto f = futurator::apply(action);
            if (!f.available()) {
                return f.then([action = std::forward<AsyncAction>(action)] (stop_iteration stop) mutable {
                    if (stop == stop_iteration::yes) {
                        return make_ready_future<>();
                    } else {
                        return repeat(std::forward<AsyncAction>(action));
                    }
                });
            }
            if (f.get0() == stop_iteration::yes) {
                return make_ready_future<>();
            }
        } while (!need_preempt());
        promise<> p;
        auto f = p.get_future();
        schedule_normal(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)]() mutable {
            repeat(std::forward<AsyncAction>(action)).forward_to(std::move(p));
        }));
        return f;
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}


template <typename T>
struct repeat_until_value_type_helper;


/// Type helper for repeat_until_value()
template <typename T>
struct repeat_until_value_type_helper<future<std::optional<T>>> {
    using value_type = T;
    using optional_type = std::optional<T>;
    using future_type = future<value_type>;
    using future_optional_type = future<optional_type>;
};

template <typename AsyncAction>
using repeat_until_value_return_type
        = typename repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>::future_type;

template<typename AsyncAction>
GCC6_CONCEPT( requires requires (AsyncAction aa) {
    requires is_future<decltype(aa())>::value;
    bool(aa().get0());
    aa().get0().value();
} )
repeat_until_value_return_type<AsyncAction>
repeat_until_value(AsyncAction&& action) {
    using type_helper = repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>;
    // the "T" in the documentation
    using value_type = typename type_helper::value_type;
    using optional_type = typename type_helper::optional_type;
    using futurator = futurize<typename type_helper::future_optional_type>;
    do {
        auto f = futurator::apply(action);

        if (!f.available()) {
            return f.then([action = std::forward<AsyncAction>(action)] (auto&& optional) mutable {
                if (optional) {
                    return make_ready_future<value_type>(std::move(optional.value()));
                } else {
                    return repeat_until_value(std::forward<AsyncAction>(action));
                }
            });
        }

        if (f.failed()) {
            return make_exception_future<value_type>(f.get_exception());
        }

        optional_type&& optional = std::move(f).get0();
        if (optional) {
            return make_ready_future<value_type>(std::move(optional.value()));
        }
    } while (!need_preempt());

    try {
        promise<value_type> p;
        auto f = p.get_future();
        schedule_normal(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)] () mutable {
            repeat_until_value(std::forward<AsyncAction>(action)).forward_to(std::move(p));
        }));
        return f;
    } catch (...) {
        return make_exception_future<value_type>(std::current_exception());
    }
}

template<typename AsyncAction, typename StopCondition>
GCC6_CONCEPT( requires ApplyReturns<StopCondition, bool> && ApplyReturns<AsyncAction, future<>> )
static inline
future<> do_until(StopCondition&& stop_cond, AsyncAction&& action) {
    promise<> p;
    auto f = p.get_future();
    do_until_continued(std::forward<StopCondition>(stop_cond),
        std::forward<AsyncAction>(action), std::move(p));
    return f;
}

template<typename AsyncAction>
GCC6_CONCEPT( requires ApplyReturns<AsyncAction, future<>> )
static inline
future<> keep_doing(AsyncAction&& action) {
    return repeat([action = std::forward<AsyncAction>(action)] () mutable {
        return action().then([]{ return stop_iteration::no;});
    });
}




template<typename Iterator, typename AsyncAction>
GCC6_CONCEPT( requires requires (Iterator i, AsyncAction aa) { { aa(*i) } -> std::same_as<future<>>; } )
static inline
future<> do_for_each(Iterator begin, Iterator end, AsyncAction&& action) {
    if (begin == end) {
        return make_ready_future<>();
    }
    while (true) {
        auto f = action(*begin++);
        if (begin == end) {
            return f;
        }
        if (!f.available() || need_preempt()) {
            return std::move(f).then([action = std::forward<AsyncAction>(action),
                    begin = std::move(begin), end = std::move(end)] () mutable {
                return do_for_each(std::move(begin), std::move(end), std::forward<AsyncAction>(action));
            });
        }
        if (f.failed()) {
            return std::move(f);
        }
    }
}

template<typename Container, typename AsyncAction>
GCC6_CONCEPT( requires requires (Container c, AsyncAction aa) { { aa(*c.begin()) } -> std::same_as<future<>>; } )
static inline
future<> do_for_each(Container& c, AsyncAction&& action) {
    return do_for_each(std::begin(c), std::end(c), std::forward<AsyncAction>(action));
}

namespace internal {

template<typename... Futures>
struct identity_futures_tuple {
    using future_type = future<std::tuple<Futures...>>;
    using promise_type = typename future_type::promise_type;
    static void set_promise(promise_type& p, std::tuple<Futures...> futures) {
        p.set_value(std::move(futures));
    }
};


template<typename ResolvedTupleTransform, typename... Futures>
class when_all_state: public std::enable_shared_from_this<when_all_state<ResolvedTupleTransform, Futures...>>{
    using type = std::tuple<Futures...>;
    type tuple;
public:
    typename ResolvedTupleTransform::promise_type p;
    when_all_state(Futures&&... t) : tuple(std::make_tuple(std::move(t)...)) {}
    ~when_all_state() {
        ResolvedTupleTransform::set_promise(p, std::move(tuple));
    }
private:
    template<size_t Idx>
    int wait() {
        auto& f = std::get<Idx>(tuple);
        static_assert(is_future<std::remove_reference_t<decltype(f)>>::value, "when_all parameter must be a future");
        if (!f.available()) {
            f = f.then_wrapped([s = this->shared_from_this()] (auto&& f) {
                return std::move(f);
            });
        }
        return 0;
    }
public:
    template <size_t... Idx>
    typename ResolvedTupleTransform::future_type wait_all(std::index_sequence<Idx...>) {
        [] (...) {} (this->template wait<Idx>()...);
        return p.get_future();
    }
};
}






/// if sharded service inherits from this class sharded::stop() will wait
/// untill all references to a service on each shard will dissapper before
/// returning. It is still service's own responcibility to track its references
/// in asyncronous code by calling shared_from_this() and keeping returned smart
/// pointer as long as object is in use.
template<typename T>
class async_sharded_service{
protected:
    std::function<void()> _delete_cb;
    ~async_sharded_service() {
        if (_delete_cb) {
            _delete_cb();
        }
    }
    template <typename Service> friend class sharded;
};
/// Exception thrown when a \ref sharded object does not exist
class no_sharded_instance_exception : public std::exception {
public:
    virtual const char* what() const noexcept override {
        return "sharded instance does not exists";
    }
};

template <typename... Futs>
GCC6_CONCEPT( requires AllAreFutures<Futs...> )
inline
future<std::tuple<Futs...>>
when_all(Futs&&... futs) {
    namespace si = internal;
    using state = si::when_all_state<si::identity_futures_tuple<Futs...>, Futs...>;
    auto s = std::make_shared<state>(std::forward<Futs>(futs)...);
    return s->wait_all(std::make_index_sequence<sizeof...(Futs)>());
}


/// \defgroup smp-module Multicore
///
/// \brief Support for exploiting multiple cores on a server.
///
/// Seastar supports multicore servers by using \i sharding.  Each logical
/// core (lcore) runs a separate event loop, with its own memory allocator,
/// TCP/IP stack, and other services.  Shards communicate by explicit message
/// passing, rather than using locks and condition variables as with traditional
/// threaded programming.

/// \addtogroup smp-module
/// @{
/// Template helper to distribute a service across all logical cores.
/// The \c sharded template manages a sharded service, by creating
/// a copy of the service on each logical core, providing mechanisms to communicate
/// with each shard's copy, and a way to stop the service.
/// \tparam Service a class to be instantiated on each core.  Must expose
///         a \c stop() method that returns a \c future<>, to be called when
///         the service is stopped.
reactor& engine();
template <typename Service>
class sharded {
public:
    struct entry {
        std::shared_ptr<Service> service;
        promise<> freed;
    };
    std::vector<entry> _instances;
    void service_deleted() {
        _instances[engine().cpu_id()].freed.set_value();
    }
    // template <typename U, bool async>
    // // friend struct std::shared_ptr_make_helper;

    /// Constructs an empty \c sharded object.  No instances of the service are
    /// created.
    sharded() {}
    sharded(const sharded& other) = delete;
    /// Moves a \c sharded object.
    sharded(sharded&& other) = default;
    sharded& operator=(const sharded& other) = delete;
    /// Moves a \c sharded object.
    sharded& operator=(sharded&& other) = default;
    /// Destroyes a \c sharded object.  Must not be in a started state.
    ~sharded();

    /// Starts \c Service by constructing an instance on every logical core
    /// with a copy of \c args passed to the constructor.
    ///
    /// \param args Arguments to be forwarded to \c Service constructor
    /// \return a \ref future<> that becomes ready when all instances have been
    ///         constructed.
    template <typename... Args>
    future<> start(Args&&... args);

    /// Starts \c Service by constructing an instance on a single logical core
    /// with a copy of \c args passed to the constructor.
    ///
    /// \param args Arguments to be forwarded to \c Service constructor
    /// \return a \ref future<> that becomes ready when the instance has been
    ///         constructed.
    template <typename... Args>
    future<> start_single(Args&&... args);

    /// Stops all started instances and destroys them.
    ///
    /// For every started instance, its \c stop() method is called, and then
    /// it is destroyed.
    future<> stop();

    // Invoke a method on all instances of @Service.
    // The return value becomes ready when all instances have processed
    // the message.
    template <typename... Args>
    future<> invoke_on_all(future<> (Service::*func)(Args...), Args... args);

    /// Invoke a method on all \c Service instances in parallel.
    ///
    /// \param func member function to be called.  Must return \c void or
    ///             \c future<>.
    /// \param args arguments to be passed to \c func.
    /// \return future that becomes ready when the method has been invoked
    ///         on all instances.
    template <typename... Args>
    future<> invoke_on_all(void (Service::*func)(Args...), Args... args);

    /// Invoke a callable on all instances of  \c Service.
    ///
    /// \param func a callable with the signature `void (Service&)`
    ///             or `future<> (Service&)`, to be called on each core
    ///             with the local instance as an argument.
    /// \return a `future<>` that becomes ready when all cores have
    ///         processed the message.
    template <typename Func>
    future<> invoke_on_all(Func&& func);

    /// Invoke a method on all instances of `Service` and reduce the results using
    /// `Reducer`.
    ///
    /// \see map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    // template <typename Reducer, typename Ret, typename... FuncArgs, typename... Args>
    // inline
    // auto
    // map_reduce(Reducer&& r, Ret (Service::*func)(FuncArgs...), Args&&... args)
    //     -> typename reducer_traits<Reducer>::future_type
    // {
    //     return ::map_reduce(boost::counting_iterator<unsigned>(0),
    //                         boost::counting_iterator<unsigned>(_instances.size()),
    //         [this, func, args = std::make_tuple(std::forward<Args>(args)...)] (unsigned c) mutable {
    //             return smp::submit_to(c, [this, func, args] () mutable {
    //                 return apply([this, func] (Args&&... args) mutable {
    //                     auto inst = _instances[engine().cpu_id()].service;
    //                     if (inst) {
    //                         return ((*inst).*func)(std::forward<Args>(args)...);
    //                     } else {
    //                         throw no_sharded_instance_exception();
    //                     }
    //                 }, std::move(args));
    //             });
    //         }, std::forward<Reducer>(r));
    // }

    /// Invoke a callable on all instances of `Service` and reduce the results using
    /// Reducer
    /// \see map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    // template <typename Reducer, typename Func>
    // inline
    // auto map_reduce(Reducer&& r, Func&& func) -> typename reducer_traits<Reducer>::future_type
    // {
    //     return ::map_reduce(boost::counting_iterator<unsigned>(0),
    //                         boost::counting_iterator<unsigned>(_instances.size()),
    //         [this, &func] (unsigned c) mutable {
    //             return smp::submit_to(c, [this, func] () mutable {
    //                 auto inst = get_local_service();
    //                 return func(*inst);
    //             });
    //         },std::forward<Reducer>(r));
    // }

    /// Applies a map function to all shards, then reduces the output by calling a reducer function.
    /// \param map callable with the signature `Value (Service&)` or
    ///               `future<Value> (Service&)` (for some `Value` type).
    ///               used as the second input to \c reduce
    /// \param initial initial value used as the first input to \c reduce.
    /// \param reduce binary function used to left-fold the return values of \c map
    ///               into \c initial .
    /// Each \c map invocation runs on the shard associated with the service.
    /// \tparam  Mapper unary function taking `Service&` and producing some result.
    /// \tparam  Initial any value type
    /// \tparam  Reduce a binary function taking two Initial values and returning an Initial
    /// \return  Result of applying `map` to each instance in parallel, reduced by calling
    ///          `reduce()` on each adjacent pair of results.
    // template <typename Mapper, typename Initial, typename Reduce>
    // inline
    // future<Initial>
    // map_reduce0(Mapper map, Initial initial, Reduce reduce) {
    //     auto wrapped_map = [this, map] (unsigned c) {
    //         return smp::submit_to(c, [this, map] {
    //             auto inst = get_local_service();
    //             return map(*inst);
    //         });
    //     };
    //     return ::map_reduce(smp::all_cpus().begin(), smp::all_cpus().end(),
    //                         std::move(wrapped_map),
    //                         std::move(initial),
    //                         std::move(reduce));
    // }

    /// Applies a map function to all shards, and return a vector of the result.
    /// \param mapper callable with the signature `Value (Service&)` or
    ///               `future<Value> (Service&)` (for some `Value` type).
    /// Each \c map invocation runs on the shard associated with the service.
    ///
    /// \tparam  Mapper unary function taking `Service&` and producing some result.
    /// \return  Result vector of applying `map` to each instance in parallel
    template <typename Mapper, typename return_type = std::result_of_t<Mapper(Service&)>>
    inline future<std::vector<return_type>> map(Mapper mapper) {
        return do_with(std::vector<return_type>(),
                [&mapper, this] (std::vector<return_type>& vec) mutable {
            vec.resize(smp::count);
            return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, &vec, mapper] (unsigned c) {
                return smp::submit_to(c, [this, mapper] {
                    auto inst = get_local_service();
                    return mapper(*inst);
                }).then([&vec, c] (auto res) {
                    vec[c] = res;
                });
            }).then([&vec] {
                return make_ready_future<std::vector<return_type>>(std::move(vec));
            });
        });
    }

    /// Invoke a method on a specific instance of `Service`.
    ///
    /// \param id shard id to call
    /// \param func a method of `Service`
    /// \param args arguments to be passed to `func`
    /// \return result of calling `func(args)` on the designated instance
    template <typename Ret, typename... FuncArgs, typename... Args, typename FutureRet = futurize_t<Ret>>
    FutureRet
    invoke_on(unsigned id, Ret (Service::*func)(FuncArgs...), Args&&... args) {
        using futurator = futurize<Ret>;
        return smp::submit_to(id, [this, func, args = std::make_tuple(std::forward<Args>(args)...)] () mutable {
            auto inst = get_local_service();
            return futurator::apply(std::mem_fn(func), std::tuple_cat(std::make_tuple<>(inst), std::move(args)));
        });
    }

    /// Invoke a callable on a specific instance of `Service`.
    ///
    /// \param id shard id to call
    /// \param func a callable with signature `Value (Service&)` or
    ///        `future<Value> (Service&)` (for some `Value` type)
    /// \return result of calling `func(instance)` on the designated instance
    template <typename Func, typename Ret = futurize_t<std::result_of_t<Func(Service&)>>>
    Ret
    invoke_on(unsigned id, Func&& func) {
        return smp::submit_to(id, [this, func = std::forward<Func>(func)] () mutable {
            auto inst = get_local_service();
            return func(*inst);
        });
    }

    /// Gets a reference to the local instance.
    Service& local();

    /// Gets a shared pointer to the local instance.
    std::shared_ptr<Service> local_shared();

    /// Checks whether the local instance has been initialized.
    bool local_is_initialized();

private:
    void track_deletion(std::shared_ptr<Service>& s, std::false_type) {
        // do not wait for instance to be deleted since it is not going to notify us
        service_deleted();
    }

    void track_deletion(std::shared_ptr<Service>& s, std::true_type) {
        s->_delete_cb = std::bind(std::mem_fn(&sharded<Service>::service_deleted), this);
    }

    template <typename... Args>
    std::shared_ptr<Service> create_local_service(Args&&... args) {
        auto s = std::make_shared<Service>(std::forward<Args>(args)...);
        track_deletion(s, std::is_base_of<async_sharded_service<Service>, Service>());
        return s;
    }

    std::shared_ptr<Service> get_local_service() {
        auto inst = _instances[engine().cpu_id()].service;
        if (!inst) {
            throw no_sharded_instance_exception();
        }
        return inst;
    }
};

template <typename Service>
sharded<Service>::~sharded() {
	assert(_instances.empty());
}

template <typename Service>
template <typename... Args>
future<>
sharded<Service>::start(Args&&... args) {
    _instances.resize(smp::count);
    return parallel_for_each(
        boost::irange<unsigned>(0, _instances.size()),
        [this, args = std::make_tuple(std::forward<Args>(args)...)] (unsigned c) mutable {
            return smp::submit_to(c, [this, args] () mutable {
                _instances[engine().cpu_id()].service = apply([this] (Args... args) {
                    return create_local_service(std::forward<Args>(args)...);
                }, args);
            });
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
            return make_ready_future<>();
        } catch (...) {
            return this->stop().then([e = std::current_exception()] () mutable {
                std::rethrow_exception(e);
            });
        }
    });
}

template <typename Service>
template <typename... Args>
future<>
sharded<Service>::start_single(Args&&... args) {
    assert(_instances.empty());
    _instances.resize(1);
    return smp::submit_to(0, [this, args = std::make_tuple(std::forward<Args>(args)...)] () mutable {
        _instances[0].service = apply([this] (Args... args) {
            return create_local_service(std::forward<Args>(args)...);
        }, args);
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
            return make_ready_future<>();
        } catch (...) {
            return this->stop().then([e = std::current_exception()] () mutable {
                std::rethrow_exception(e);
            });
        }
    });
}

template <typename Service>
future<>
sharded<Service>::stop() {
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this] (unsigned c) mutable {
        return smp::submit_to(c, [this] () mutable {
            auto inst = _instances[engine().cpu_id()].service;
            if (!inst) {
                return make_ready_future<>();
            }
            _instances[engine().cpu_id()].service = nullptr;
            return inst->stop().then([this, inst] {
                return _instances[engine().cpu_id()].freed.get_future();
            });
        });
    }).then([this] {
        _instances.clear();
        _instances = std::vector<sharded<Service>::entry>();
    });
}

template <typename Service>
template <typename... Args>
inline
future<>
sharded<Service>::invoke_on_all(future<> (Service::*func)(Args...), Args... args) {
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, func, args...] (unsigned c) {
        return smp::submit_to(c, [this, func, args...] {
            auto inst = get_local_service();
            return ((*inst).*func)(args...);
        });
    });
}

template <typename Service>
template <typename... Args>
inline
future<>
sharded<Service>::invoke_on_all(void (Service::*func)(Args...), Args... args) {
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, func, args...] (unsigned c) {
        return smp::submit_to(c, [this, func, args...] {
            auto inst = get_local_service();
            ((*inst).*func)(args...);
        });
    });
}

template <typename Service>
template <typename Func>
inline
future<>
sharded<Service>::invoke_on_all(Func&& func) {
    static_assert(std::is_same<futurize_t<std::result_of_t<Func(Service&)>>, future<>>::value,
                  "invoke_on_all()'s func must return void or future<>");
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, &func] (unsigned c) {
        return smp::submit_to(c, [this, func] {
            auto inst = get_local_service();
            return func(*inst);
        });
    });
}

template <typename Service>
Service& sharded<Service>::local() {
    assert(local_is_initialized());
    return *_instances[engine().cpu_id()].service;
}

template <typename Service>
std::shared_ptr<Service> sharded<Service>::local_shared() {
    assert(local_is_initialized());
    return _instances[engine().cpu_id()].service;
}

template <typename Service>
inline bool sharded<Service>::local_is_initialized() {
    return _instances.size() > engine().cpu_id() &&
           _instances[engine().cpu_id()].service;
}


template <typename Service>
using distributed = sharded<Service>;





// template <typename... Futs>
// GCC6_CONCEPT( requires AllAreFutures<Futs...> )
// inline
// future<std::tuple<Futs...>>
// when_all(Futs&&... futs) {
//     namespace si = internal;
//     using state = si::when_all_state<si::identity_futures_tuple<Futs...>, Futs...>;
//     auto s = std::make_shared<state>(std::forward<Futs>(futs)...);
//     return s->wait_all(std::make_index_sequence<sizeof...(Futs)>());
// }

/// \cond internal
namespace internal {

template <typename Iterator, typename IteratorCategory>
inline
size_t
when_all_estimate_vector_capacity(Iterator begin, Iterator end, IteratorCategory category) {
    // For InputIterators we can't estimate needed capacity
    return 0;
}

template <typename Iterator>
inline
size_t
when_all_estimate_vector_capacity(Iterator begin, Iterator end, std::forward_iterator_tag category) {
    // May be linear time below random_access_iterator_tag, but still better than reallocation
    return std::distance(begin, end);
}

template<typename Future>
struct identity_futures_vector {
    using future_type = future<std::vector<Future>>;
    static future_type run(std::vector<Future> futures) {
        return make_ready_future<std::vector<Future>>(std::move(futures));
    }
};

// Internal function for when_all().
template <typename ResolvedVectorTransform, typename Future>
inline
typename ResolvedVectorTransform::future_type
complete_when_all(std::vector<Future>&& futures, typename std::vector<Future>::iterator pos) {
    // If any futures are already ready, skip them.
    while (pos != futures.end() && pos->available()) {
        ++pos;
    }
    // Done?
    if (pos == futures.end()) {
        return ResolvedVectorTransform::run(std::move(futures));
    }
    // Wait for unready future, store, and continue.
    return pos->then_wrapped([futures = std::move(futures), pos] (auto fut) mutable {
        *pos++ = std::move(fut);
        return complete_when_all<ResolvedVectorTransform>(std::move(futures), pos);
    });
}

template<typename ResolvedVectorTransform, typename FutureIterator>
inline auto
do_when_all(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    std::vector<typename itraits::value_type> ret;
    ret.reserve(when_all_estimate_vector_capacity(begin, end, typename itraits::iterator_category()));
    // Important to invoke the *begin here, in case it's a function iterator,
    // so we launch all computation in parallel.
    std::move(begin, end, std::back_inserter(ret));
    return complete_when_all<ResolvedVectorTransform>(std::move(ret), ret.begin());
}

}

