// #pragma once
// #ifndef DEBUG_SHARED_PTR
// #include <memory> // for std::unique_ptr
// template<typename T>
// struct is_smart_ptr : std::false_type {};
// template<typename T>
// struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};


// // This header defines functors for comparing and hashing pointers by pointed-to values instead of pointer addresses.
// // Examples:
// //  std::multiset<shared_ptr<sstring>, indirect_less<shared_ptr<sstring>>> _multiset;
// //  std::unordered_map<shared_ptr<sstring>, bool,
// //      indirect_hash<shared_ptr<sstring>>, indirect_equal_to<shared_ptr<sstring>>> _unordered_map;

// template<typename Pointer, typename Equal = std::equal_to<typename std::pointer_traits<Pointer>::element_type>>
// struct indirect_equal_to {
//     Equal _eq;
//     indirect_equal_to(Equal eq = Equal()) : _eq(std::move(eq)) {}
//     bool operator()(const Pointer& i1, const Pointer& i2) const {
//         if (bool(i1) ^ bool(i2)) {
//             return false;
//         }
//         return !i1 || _eq(*i1, *i2);
//     }
// };

// template<typename Pointer, typename Less = std::less<typename std::pointer_traits<Pointer>::element_type>>
// struct indirect_less {
//     Less _cmp;
//     indirect_less(Less cmp = Less()) : _cmp(std::move(cmp)) {}
//     bool operator()(const Pointer& i1, const Pointer& i2) const {
//         if (i1 && i2) {
//             return _cmp(*i1, *i2);
//         }
//         return !i1 && i2;
//     }
// };

// template<typename Pointer, typename Hash = std::hash<typename std::pointer_traits<Pointer>::element_type>>
// struct indirect_hash {
//     Hash _h;
//     indirect_hash(Hash h = Hash()) : _h(std::move(h)) {}
//     size_t operator()(const Pointer& p) const {
//         if (p) {
//             return _h(*p);
//         }
//         return 0;
//     }
// };


// using shared_ptr_counter_type = long;

// template <typename T>
// class lw_shared_ptr;

// template <typename T>
// class shared_ptr;

// template <typename T>
// class enable_lw_shared_from_this;

// template <typename T>
// class enable_shared_from_this;

// template <typename T, typename... A>
// lw_shared_ptr<T> make_lw_shared(A&&... a);

// template <typename T>
// lw_shared_ptr<T> make_lw_shared(T&& a);

// template <typename T>
// lw_shared_ptr<T> make_lw_shared(T& a);

// template <typename T, typename... A>
// shared_ptr<T> make_shared(A&&... a);

// template <typename T>
// shared_ptr<T> make_shared(T&& a);

// template <typename T, typename U>
// shared_ptr<T> static_pointer_cast(const shared_ptr<U>& p);

// template <typename T, typename U>
// shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& p);

// template <typename T, typename U>
// shared_ptr<T> const_pointer_cast(const shared_ptr<U>& p);

// // We want to support two use cases for shared_ptr<T>:
// //
// //   1. T is any type (primitive or class type)
// //
// //   2. T is a class type that inherits from enable_shared_from_this<T>.
// //
// // In the first case, we must wrap T in an object containing the counter,
// // since T may be a primitive type and cannot be a base class.
// //
// // In the second case, we want T to reach the counter through its
// // enable_shared_from_this<> base class, so that we can implement
// // shared_from_this().
// //
// // To implement those two conflicting requirements (T alongside its counter;
// // T inherits from an object containing the counter) we use std::conditional<>
// // and some accessor functions to select between two implementations.


// // CRTP from this to enable shared_from_this:
// template <typename T>
// class enable_lw_shared_from_this {
//     shared_ptr_counter_type _count = 0;
//     using ctor = T;
//     T* to_value() { return static_cast<T*>(this); }
//     T* to_internal_object() { return static_cast<T*>(this); }
// protected:
//     enable_lw_shared_from_this() noexcept {}
//     enable_lw_shared_from_this(enable_lw_shared_from_this&&) noexcept {}
//     enable_lw_shared_from_this(const enable_lw_shared_from_this&) noexcept {}
//     enable_lw_shared_from_this& operator=(const enable_lw_shared_from_this&) noexcept { return *this; }
//     enable_lw_shared_from_this& operator=(enable_lw_shared_from_this&&) noexcept { return *this; }
// public:
//     lw_shared_ptr<T> shared_from_this();
//     lw_shared_ptr<const T> shared_from_this() const;
//     template <typename X>
//     friend class lw_shared_ptr;
// };

// template <typename T>
// struct shared_ptr_no_esft {
//     shared_ptr_counter_type _count = 0;
//     T _value;
//     using ctor = shared_ptr_no_esft;

//     T* to_value() { return &_value; }
//     shared_ptr_no_esft* to_internal_object() { return this; }
//     shared_ptr_no_esft() = default;
//     shared_ptr_no_esft(const T& x) : _value(x) {}
//     shared_ptr_no_esft(T&& x) : _value(std::move(x)) {}
//     template <typename... A>
//     shared_ptr_no_esft(A&&... a) : _value(std::forward<A>(a)...) {}
//     template <typename X>
//     friend class lw_shared_ptr;
// };

// template <typename T>
// using shared_ptr_impl
//     = std::conditional_t<
//         std::is_base_of<enable_lw_shared_from_this<std::remove_const_t<T>>, T>::value,
//         enable_lw_shared_from_this<std::remove_const_t<T>>,
//         shared_ptr_no_esft<std::remove_const_t<T>>
//       >;

// template <typename T>
// class lw_shared_ptr {
//     mutable shared_ptr_impl<T>* _p = nullptr;
// private:
//     lw_shared_ptr(shared_ptr_impl<T>* p) noexcept : _p(p) {
//         if (_p) {
//             ++_p->_count;
//         }
//     }
//     template <typename... A>
//     static lw_shared_ptr make(A&&... a) {
//         return lw_shared_ptr(new typename shared_ptr_impl<T>::ctor(std::forward<A>(a)...));
//     }
// public:
//     using element_type = T;

//     lw_shared_ptr() noexcept = default;
//     lw_shared_ptr(std::nullptr_t) noexcept : lw_shared_ptr() {}
//     lw_shared_ptr(const lw_shared_ptr& x) noexcept : _p(x._p) {
//         if (_p) {
//             ++_p->_count;
//         }
//     }
//     lw_shared_ptr(lw_shared_ptr&& x) noexcept  : _p(x._p) {
//         x._p = nullptr;
//     }
//     [[gnu::always_inline]]
//     ~lw_shared_ptr() {
//         if (_p && !--_p->_count) {
//             delete _p->to_internal_object();
//         }
//     }
//     lw_shared_ptr& operator=(const lw_shared_ptr& x) noexcept {
//         if (_p != x._p) {
//             this->~lw_shared_ptr();
//             new (this) lw_shared_ptr(x);
//         }
//         return *this;
//     }
//     lw_shared_ptr& operator=(lw_shared_ptr&& x) noexcept {
//         if (_p != x._p) {
//             this->~lw_shared_ptr();
//             new (this) lw_shared_ptr(std::move(x));
//         }
//         return *this;
//     }
//     lw_shared_ptr& operator=(std::nullptr_t) noexcept {
//         return *this = lw_shared_ptr();
//     }
//     lw_shared_ptr& operator=(T&& x) noexcept {
//         this->~lw_shared_ptr();
//         new (this) lw_shared_ptr(make_lw_shared<T>(std::move(x)));
//         return *this;
//     }

//     T& operator*() const noexcept { return *_p->to_value(); }
//     T* operator->() const noexcept { return _p->to_value(); }
//     T* get() const noexcept {
//         if (_p) {
//             return _p->to_value();
//         } else {
//             return nullptr;
//         }
//     }

//     long int use_count() const noexcept {
//         if (_p) {
//             return _p->_count;
//         } else {
//             return 0;
//         }
//     }

//     operator lw_shared_ptr<const T>() const noexcept {
//         return lw_shared_ptr<const T>(_p);
//     }

//     explicit operator bool() const noexcept {
//         return _p;
//     }

//     bool owned() const noexcept {
//         return _p->_count == 1;
//     }

//     bool operator==(const lw_shared_ptr<const T>& x) const {
//         return _p == x._p;
//     }

//     bool operator!=(const lw_shared_ptr<const T>& x) const {
//         return !operator==(x);
//     }

//     bool operator==(const lw_shared_ptr<std::remove_const_t<T>>& x) const {
//         return _p == x._p;
//     }

//     bool operator!=(const lw_shared_ptr<std::remove_const_t<T>>& x) const {
//         return !operator==(x);
//     }

//     bool operator<(const lw_shared_ptr<const T>& x) const {
//         return _p < x._p;
//     }

//     bool operator<(const lw_shared_ptr<std::remove_const_t<T>>& x) const {
//         return _p < x._p;
//     }

//     template <typename U>
//     friend class lw_shared_ptr;

//     template <typename X, typename... A>
//     friend lw_shared_ptr<X> make_lw_shared(A&&...);

//     template <typename U>
//     friend lw_shared_ptr<U> make_lw_shared(U&&);

//     template <typename U>
//     friend lw_shared_ptr<U> make_lw_shared(U&);

//     template <typename U>
//     friend class enable_lw_shared_from_this;
// };

// template <typename T, typename... A>
// inline
// lw_shared_ptr<T> make_lw_shared(A&&... a) {
//     return lw_shared_ptr<T>::make(std::forward<A>(a)...);
// }

// template <typename T>
// inline
// lw_shared_ptr<T> make_lw_shared(T&& a) {
//     return lw_shared_ptr<T>::make(std::move(a));
// }

// template <typename T>
// inline
// lw_shared_ptr<T> make_lw_shared(T& a) {
//     return lw_shared_ptr<T>::make(a);
// }

// template <typename T>
// inline
// lw_shared_ptr<T> enable_lw_shared_from_this<T>::shared_from_this() {
//     return lw_shared_ptr<T>(this);
// }

// template <typename T>
// inline lw_shared_ptr<const T>
// enable_lw_shared_from_this<T>::shared_from_this() const {
//     return lw_shared_ptr<const T>(const_cast<enable_lw_shared_from_this*>(this));
// }

// template <typename T>
// static inline std::ostream& operator<<(std::ostream& out, const lw_shared_ptr<T>& p) {
//     if (!p) {
//         return out << "null";
//     }
//     return out << *p;
// }

// // Polymorphic shared pointer class
// struct shared_ptr_count_base {
//     // destructor is responsible for fully-typed deletion
//     virtual ~shared_ptr_count_base() {}
//     shared_ptr_counter_type count = 0;
// };

// template <typename T>
// struct shared_ptr_count_for : shared_ptr_count_base {
//     T data;
//     template <typename... A>
//     shared_ptr_count_for(A&&... a) : data(std::forward<A>(a)...) {}
// };

// template <typename T>
// class enable_shared_from_this : private shared_ptr_count_base {
// public:
//     shared_ptr<T> shared_from_this();
//     shared_ptr<const T> shared_from_this() const;

//     template <typename U>
//     friend class shared_ptr;

//     template <typename U, bool esft>
//     friend struct shared_ptr_make_helper;
// };

// template <typename T>
// class shared_ptr {
//     mutable shared_ptr_count_base* _b = nullptr;
//     mutable T* _p = nullptr;
// private:
//     explicit shared_ptr(shared_ptr_count_for<T>* b) noexcept : _b(b), _p(&b->data) {
//         ++_b->count;
//     }
//     shared_ptr(shared_ptr_count_base* b, T* p) noexcept : _b(b), _p(p) {
//         if (_b) {
//             ++_b->count;
//         }
//     }
//     explicit shared_ptr(enable_shared_from_this<std::remove_const_t<T>>* p) noexcept : _b(p), _p(static_cast<T*>(p)) {
//         if (_b) {
//             ++_b->count;
//         }
//     }
// public:
//     using element_type = T;

//     shared_ptr() noexcept = default;
//     shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}
//     shared_ptr(const shared_ptr& x) noexcept
//             : _b(x._b)
//             , _p(x._p) {
//         if (_b) {
//             ++_b->count;
//         }
//     }
//     shared_ptr(shared_ptr&& x) noexcept
//             : _b(x._b)
//             , _p(x._p) {
//         x._b = nullptr;
//         x._p = nullptr;
//     }
//     template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
//     shared_ptr(const shared_ptr<U>& x) noexcept
//             : _b(x._b)
//             , _p(x._p) {
//         if (_b) {
//             ++_b->count;
//         }
//     }
//     template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
//     shared_ptr(shared_ptr<U>&& x) noexcept
//             : _b(x._b)
//             , _p(x._p) {
//         x._b = nullptr;
//         x._p = nullptr;
//     }
//     ~shared_ptr() {
//         if (_b && !--_b->count) {
//             delete _b;
//         }
//     }
//     shared_ptr& operator=(const shared_ptr& x) noexcept {
//         if (this != &x) {
//             this->~shared_ptr();
//             new (this) shared_ptr(x);
//         }
//         return *this;
//     }
//     shared_ptr& operator=(shared_ptr&& x) noexcept {
//         if (this != &x) {
//             this->~shared_ptr();
//             new (this) shared_ptr(std::move(x));
//         }
//         return *this;
//     }
//     shared_ptr& operator=(std::nullptr_t) noexcept {
//         return *this = shared_ptr();
//     }
//     template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
//     shared_ptr& operator=(const shared_ptr<U>& x) noexcept {
//         if (*this != x) {
//             this->~shared_ptr();
//             new (this) shared_ptr(x);
//         }
//         return *this;
//     }
//     template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
//     shared_ptr& operator=(shared_ptr<U>&& x) noexcept {
//         if (*this != x) {
//             this->~shared_ptr();
//             new (this) shared_ptr(std::move(x));
//         }
//         return *this;
//     }
//     explicit operator bool() const noexcept {
//         return _p;
//     }
//     T& operator*() const noexcept {
//         return *_p;
//     }
//     T* operator->() const noexcept {
//         return _p;
//     }
//     T* get() const noexcept {
//         return _p;
//     }
//     long use_count() const noexcept {
//         if (_b) {
//             return _b->count;
//         } else {
//             return 0;
//         }
//     }

//     template <bool esft>
//     struct make_helper;

//     template <typename U, typename... A>
//     friend shared_ptr<U> make_shared(A&&... a);

//     template <typename U>
//     friend shared_ptr<U> make_shared(U&& a);

//     template <typename V, typename U>
//     friend shared_ptr<V> static_pointer_cast(const shared_ptr<U>& p);

//     template <typename V, typename U>
//     friend shared_ptr<V> dynamic_pointer_cast(const shared_ptr<U>& p);

//     template <typename V, typename U>
//     friend shared_ptr<V> const_pointer_cast(const shared_ptr<U>& p);

//     template <bool esft, typename... A>
//     static shared_ptr make(A&&... a);

//     template <typename U>
//     friend class enable_shared_from_this;

//     template <typename U, bool esft>
//     friend struct shared_ptr_make_helper;

//     template <typename U>
//     friend class shared_ptr;
// };

// template <typename U, bool esft>
// struct shared_ptr_make_helper;

// template <typename T>
// struct shared_ptr_make_helper<T, false> {
//     template <typename... A>
//     static shared_ptr<T> make(A&&... a) {
//         return shared_ptr<T>(new shared_ptr_count_for<T>(std::forward<A>(a)...));
//     }
// };

// template <typename T>
// struct shared_ptr_make_helper<T, true> {
//     template <typename... A>
//     static shared_ptr<T> make(A&&... a) {
//         auto p = new T(std::forward<A>(a)...);
//         return shared_ptr<T>(p, p);
//     }
// };

// template <typename T, typename... A>
// inline
// shared_ptr<T>
// make_shared(A&&... a) {
//     using helper = shared_ptr_make_helper<T, std::is_base_of<shared_ptr_count_base, T>::value>;
//     return helper::make(std::forward<A>(a)...);
// }

// template <typename T>
// inline
// shared_ptr<T>
// make_shared(T&& a) {
//     using helper = shared_ptr_make_helper<T, std::is_base_of<shared_ptr_count_base, T>::value>;
//     return helper::make(std::forward<T>(a));
// }

// template <typename T, typename U>
// inline
// shared_ptr<T>
// static_pointer_cast(const shared_ptr<U>& p) {
//     return shared_ptr<T>(p._b, static_cast<T*>(p._p));
// }

// template <typename T, typename U>
// inline
// shared_ptr<T>
// dynamic_pointer_cast(const shared_ptr<U>& p) {
//     auto q = dynamic_cast<T*>(p._p);
//     return shared_ptr<T>(q ? p._b : nullptr, q);
// }

// template <typename T, typename U>
// inline
// shared_ptr<T>
// const_pointer_cast(const shared_ptr<U>& p) {
//     return shared_ptr<T>(p._b, const_cast<T*>(p._p));
// }

// template <typename T>
// inline
// shared_ptr<T>
// enable_shared_from_this<T>::shared_from_this() {
//     auto unconst = reinterpret_cast<enable_shared_from_this<std::remove_const_t<T>>*>(this);
//     return shared_ptr<T>(unconst);
// }

// template <typename T>
// inline
// shared_ptr<const T>
// enable_shared_from_this<T>::shared_from_this() const {
//     auto esft = const_cast<enable_shared_from_this*>(this);
//     auto unconst = reinterpret_cast<enable_shared_from_this<std::remove_const_t<T>>*>(esft);
//     return shared_ptr<const T>(unconst);
// }

// template <typename T, typename U>
// inline
// bool
// operator==(const shared_ptr<T>& x, const shared_ptr<U>& y) {
//     return x.get() == y.get();
// }

// template <typename T>
// inline
// bool
// operator==(const shared_ptr<T>& x, std::nullptr_t) {
//     return x.get() == nullptr;
// }

// template <typename T>
// inline
// bool
// operator==(std::nullptr_t, const shared_ptr<T>& y) {
//     return nullptr == y.get();
// }

// template <typename T, typename U>
// inline
// bool
// operator!=(const shared_ptr<T>& x, const shared_ptr<U>& y) {
//     return x.get() != y.get();
// }

// template <typename T>
// inline
// bool
// operator!=(const shared_ptr<T>& x, std::nullptr_t) {
//     return x.get() != nullptr;
// }

// template <typename T>
// inline
// bool
// operator!=(std::nullptr_t, const shared_ptr<T>& y) {
//     return nullptr != y.get();
// }

// template <typename T, typename U>
// inline
// bool
// operator<(const shared_ptr<T>& x, const shared_ptr<U>& y) {
//     return x.get() < y.get();
// }

// template <typename T>
// inline
// bool
// operator<(const shared_ptr<T>& x, std::nullptr_t) {
//     return x.get() < nullptr;
// }

// template <typename T>
// inline
// bool
// operator<(std::nullptr_t, const shared_ptr<T>& y) {
//     return nullptr < y.get();
// }

// template <typename T, typename U>
// inline
// bool
// operator<=(const shared_ptr<T>& x, const shared_ptr<U>& y) {
//     return x.get() <= y.get();
// }

// template <typename T>
// inline
// bool
// operator<=(const shared_ptr<T>& x, std::nullptr_t) {
//     return x.get() <= nullptr;
// }

// template <typename T>
// inline
// bool
// operator<=(std::nullptr_t, const shared_ptr<T>& y) {
//     return nullptr <= y.get();
// }

// template <typename T, typename U>
// inline
// bool
// operator>(const shared_ptr<T>& x, const shared_ptr<U>& y) {
//     return x.get() > y.get();
// }

// template <typename T>
// inline
// bool
// operator>(const shared_ptr<T>& x, std::nullptr_t) {
//     return x.get() > nullptr;
// }

// template <typename T>
// inline
// bool
// operator>(std::nullptr_t, const shared_ptr<T>& y) {
//     return nullptr > y.get();
// }

// template <typename T, typename U>
// inline
// bool
// operator>=(const shared_ptr<T>& x, const shared_ptr<U>& y) {
//     return x.get() >= y.get();
// }

// template <typename T>
// inline
// bool
// operator>=(const shared_ptr<T>& x, std::nullptr_t) {
//     return x.get() >= nullptr;
// }

// template <typename T>
// inline
// bool
// operator>=(std::nullptr_t, const shared_ptr<T>& y) {
//     return nullptr >= y.get();
// }

// template <typename T>
// static inline
// std::ostream& operator<<(std::ostream& out, const shared_ptr<T>& p) {
//     if (!p) {
//         return out << "null";
//     }
//     return out << *p;
// }

// template<typename T>
// using shared_ptr_equal_by_value = indirect_equal_to<shared_ptr<T>>;

// template<typename T>
// using shared_ptr_value_hash = indirect_hash<shared_ptr<T>>;


// namespace std {
// template <typename T>
//     struct hash<lw_shared_ptr<T>> : private hash<T*> {
//     size_t operator()(const lw_shared_ptr<T>& p) const {
//         return hash<T*>::operator()(p.get());
//     }
// };

// template <typename T>
// struct hash<::shared_ptr<T>> : private hash<T*> {
//     size_t operator()(const ::shared_ptr<T>& p) const {
//         return hash<T*>::operator()(p.get());
//     }
// };

// }

// #endif // DEBUG_SHARED_PTR
