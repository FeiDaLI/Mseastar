// #pragma once
// #include <tuple>
// #include <type_traits> // For std::invoke_result_t and other type traits

// // Forward declarations instead of includes to prevent circular dependencies
// template <typename... T>
// class promise;

// template <typename... T>
// class future;

// // Forward declare functions from future.hh that we need
// template <typename... T, typename... A>
// future<T...> make_ready_future(A&&... value);

// template <typename... T>
// future<T...> make_exception_future(std::exception_ptr ex) noexcept;

// template <typename T>
// struct futurize; // 前向声明

// template <typename T>
// using futurize_t = typename futurize<T>::type;

// template <typename T>
// struct futurize {
//     using type = future<T>;
//     using promise_type = promise<T>;
//     using value_type = std::tuple<T>;
//     // 把func转为future<T>
//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept{
//         try {
//             return convert(std::apply(std::forward<Func>(func), std::move(args)));
//         } catch (...) {
//             return make_exception_future(std::current_exception());
//         }
//     }

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept{
//         try {
//             return convert(func(std::forward<FuncArgs>(args)...));
//         }catch (...){
//             return make_exception_future(std::current_exception());
//         }
//     }
//     //把T转为future<T>.
//     static inline type convert(T&& value) { return make_ready_future<T>(std::move(value)); }
//     static inline type convert(type&& value) { return std::move(value); }
//     //把tuple<T>转为future<T>.
//     static type from_tuple(value_type&& value){
//         return make_ready_future<T>(std::move(value));
//     }
//     static type from_tuple(const value_type& value){
//         return make_ready_future<T>(value);
//     }
//     //返回一个表示一个exception的错误.
//     template <typename Arg>
//     static type make_exception_future(Arg&& arg){
//         return ::make_exception_future<T>(std::forward<Arg>(arg));
//     }
// };

// template <>
// struct futurize<void> {
//     using type = future<>;
//     using promise_type = promise<>;
//     using value_type = std::tuple<>;
//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept{
//         return do_void_futurize_apply_tuple(std::forward<Func>(func), std::move(args));
//     }

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept{
//         return do_void_futurize_apply(std::forward<Func>(func), std::forward<FuncArgs>(args)...);
//     }
//     static inline type from_tuple(value_type&& value){ return make_ready_future<>();}
//     static inline type from_tuple(const value_type& value){ return make_ready_future<>();}

//     template <typename Arg>
//     static type make_exception_future(Arg&& arg){
//         return ::make_exception_future<>(std::forward<Arg>(arg));
//     }
// };

// /*
//     为什么没有tuple?
// */
// template <typename... Args>
// struct futurize<future<Args...>> {
//     using type = future<Args...>;
//     using promise_type = promise<Args...>;

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept{
//         try {
//             return std::apply(std::forward<Func>(func), std::move(args));
//         } catch (...) {
//             return make_exception_future(std::current_exception());
//         }
//     }

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept{
//         try {
//             return func(std::forward<FuncArgs>(args)...);
//         } catch (...) {
//             return make_exception_future(std::current_exception());
//         }
//     }
//     static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
//     static inline type convert(type&& value) { return std::move(value); }

//     template <typename Arg>
//     static type make_exception_future(Arg&& arg){
//         return ::make_exception_future<Args...>(std::forward<Arg>(arg));
//     }
// };




// // template<typename Func, typename... FuncArgs>
// // inline
// // std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// // do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
// //     try {
// //         func(std::forward<FuncArgs>(args)...);
// //         return make_ready_future<>();
// //     } catch (...) {
// //         return make_exception_future(std::current_exception());
// //     }
// // }


// // template<typename Func, typename... FuncArgs>
// // inline
// // std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// // do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
// //     try {
// //         func(std::forward<FuncArgs>(args)...);
// //         return make_ready_future<>();
// //     } catch (...) {
// //         return make_exception_future(std::current_exception());
// //     }
// // }

// // template<typename Func, typename... FuncArgs>
// // inline
// // std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// // do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
// //     try {
// //         return func(std::forward<FuncArgs>(args)...);
// //     } catch (...) {
// //         return make_exception_future(std::current_exception());
// //     }
// // }

// // template<typename Func, typename... FuncArgs>
// // inline
// // std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// // do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
// //     try {
// //         std::apply(std::forward<Func>(func), std::move(args));
// //         return make_ready_future<>();
// //     } catch (...) {
// //         return make_exception_future(std::current_exception());
// //     }
// // }

// // template<typename Func, typename... FuncArgs>
// // inline
// // std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// // do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
// //     try {
// //         return std::apply(std::forward<Func>(func), std::move(args));
// //     } catch (...) {
// //         return make_exception_future(std::current_exception());
// //     }
// // }








// template<typename Func, typename... FuncArgs>
// requires std::is_invocable_v<Func, FuncArgs...> && (is_future_v<std::result_of_t<Func(FuncArgs...)>>)
// inline
// auto do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
//     try {
//         return func(std::forward<FuncArgs>(args)...);
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }



// template<typename Func, typename... FuncArgs>
// requires std::is_invocable_v<Func, FuncArgs...> && (!is_future_v<std::result_of_t<Func(FuncArgs...)>>)
// inline
// auto do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     try {
//         std::apply(std::forward<Func>(func), std::move(args));
//         return make_ready_future<>();
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... FuncArgs>
// requires std::is_invocable_v<Func, FuncArgs...> && (is_future_v<std::result_of_t<Func(FuncArgs...)>>)
// inline
// auto do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     try {
//         return std::apply(std::forward<Func>(func), std::move(args));
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }    







// /*
// do_void_futurize_apply_tuple函数的主要功能是把一个接受元组参数的函数包装成返回 future<> 类型的函数.
// 此函数存在两种重载形式，分别处理函数返回值为 future<> 类型和非 future<> 类型的情况.
// 功能概述:
//     非 future<> 返回值情况：尝试调用传入的函数，若成功则返回一个就绪的 future<>；若抛出异常，则返回一个包含当前异常的 future<>.
//     future<> 返回值情况：尝试调用传入的函数并返回其结果；若抛出异常，则返回一个包含当前异常的 future<>.
// */

// template<typename Func, typename... Args>
// auto futurize_apply(Func&& func, Args&&... args) {
//     using futurator = futurize<std::result_of_t<Func(Args&&...)>>;
//     return futurator::apply(std::forward<Func>(func), std::forward<Args>(args)...);
// }


