#pragma once
#include "future.hh"
#include "temp_buffer.hh"
#include <vector>
#include <unordered_map>
#include <functional>
#include "packet.hh"
#include "distributed.hh"

template <typename... T>
class stream;

template <typename... T>
class subscription;

template <typename... T>
class stream {
    subscription<T...>* _sub = nullptr;
    promise<> _done;
    promise<> _ready;
public:
    using next_fn = std::function<future<> (T...)>;
    stream() = default;
    stream(const stream&) = delete;
    stream(stream&&) = delete;
    ~stream();
    void operator=(const stream&) = delete;
    void operator=(stream&&) = delete;

    // Returns a subscription that reads value from this
    // stream.
    subscription<T...> listen();

    // Returns a subscription that reads value from this
    // stream, and also sets up the listen function.
    subscription<T...> listen(next_fn next);

    // Becomes ready when the listener is ready to accept
    // values.  Call only once, when beginning to produce
    // values.
    future<> started();

    // Produce a value.  Call only after started(), and after
    // a previous produce() is ready.
    future<> produce(T... data);

    // End the stream.   Call only after started(), and after
    // a previous produce() is ready.  No functions may be called
    // after this.
    void close();

    // Signal an error.   Call only after started(), and after
    // a previous produce() is ready.  No functions may be called
    // after this.
    template <typename E>
    void set_exception(E ex);
private:
    void pause(future<> can_continue);
    void start();
    friend class subscription<T...>;
};

template <typename... T>
class subscription {
public:
    using next_fn = typename stream<T...>::next_fn;
private:
    stream<T...>* _stream;
    next_fn _next;
private:
    explicit subscription(stream<T...>* s);
public:
    subscription(subscription&& x);
    ~subscription();

    /// \brief Start receiving events from the stream.
    ///
    /// \param next Callback to call for each event
    void start(std::function<future<> (T...)> next);

    // Becomes ready when the stream is empty, or when an error
    // happens (in that case, an exception is held).
    future<> done();

    friend class stream<T...>;
};


template <typename... T>
inline
stream<T...>::~stream() {
    if (_sub) {
        _sub->_stream = nullptr;
    }
}

template <typename... T>
inline
subscription<T...>
stream<T...>::listen() {
    return subscription<T...>(this);
}

template <typename... T>
inline
subscription<T...>
stream<T...>::listen(next_fn next) {
    auto sub = subscription<T...>(this);
    sub.start(std::move(next));
    return sub;
}

template <typename... T>
inline
future<>
stream<T...>::started() {
    return _ready.get_future();
}

template <typename... T>
inline
future<>
stream<T...>::produce(T... data) {
    auto ret = futurize<void>::apply(_sub->_next, std::move(data)...);
    if (ret.available() && !ret.failed()) {
        // Native network stack depends on stream::produce() returning
        // a ready future to push packets along without dropping.  As
        // a temporary workaround, special case a ready, unfailed future
        // and return it immediately, so that then_wrapped(), below,
        // doesn't convert a ready future to an unready one.
        return ret;
    }
    return ret.then_wrapped([this] (auto&& f) {
        try {
            f.get();
        } catch (...) {
            _done.set_exception(std::current_exception());
            // FIXME: tell the producer to stop producing
            throw;
        }
    });
}

template <typename... T>
inline
void
stream<T...>::close() {
    _done.set_value();
}

template <typename... T>
template <typename E>
inline
void
stream<T...>::set_exception(E ex) {
    _done.set_exception(ex);
}

template <typename... T>
inline
subscription<T...>::subscription(stream<T...>* s)
        : _stream(s) {
    assert(!_stream->_sub);
    _stream->_sub = this;
}

template <typename... T>
inline
void
subscription<T...>::start(std::function<future<> (T...)> next) {
    _next = std::move(next);
    _stream->_ready.set_value();
}

template <typename... T>
inline
subscription<T...>::~subscription() {
    if (_stream) {
        _stream->_sub = nullptr;
    }
}

template <typename... T>
inline
subscription<T...>::subscription(subscription&& x)
    : _stream(x._stream), _next(std::move(x._next)) {
    x._stream = nullptr;
    if (_stream) {
        _stream->_sub = this;
    }
}

template <typename... T>
inline future<> subscription<T...>::done() {
    return _stream->_done.get_future();
}


class data_source_impl {
public:
    virtual ~data_source_impl() {}
    virtual future<temporary_buffer<char>> get() = 0;
    virtual future<temporary_buffer<char>> skip(uint64_t n);
    virtual future<> close() { return make_ready_future<>(); }
};

class data_source {
    std::unique_ptr<data_source_impl> _dsi;
protected:
    data_source_impl* impl() const { return _dsi.get(); }
public:
    data_source() = default;
    explicit data_source(std::unique_ptr<data_source_impl> dsi) : _dsi(std::move(dsi)) {}
    data_source(data_source&& x) = default;
    data_source& operator=(data_source&& x) = default;
    future<temporary_buffer<char>> get() { return _dsi->get(); }
    future<temporary_buffer<char>> skip(uint64_t n) { return _dsi->skip(n); }
    future<> close() { return _dsi->close(); }
};

class data_sink_impl {
public:
    virtual ~data_sink_impl() {}
    virtual temporary_buffer<char> allocate_buffer(size_t size) {
        return temporary_buffer<char>(size);
    }
    virtual future<> put(net::packet data) = 0;
    virtual future<> put(std::vector<temporary_buffer<char>> data) {
        net::packet p;
        p.reserve(data.size());
        for (auto& buf : data) {
            p = net::packet(std::move(p), net::fragment{buf.get_write(), buf.size()}, buf.release());
        }
        return put(std::move(p));
    }
    virtual future<> put(temporary_buffer<char> buf) {
        return put(net::packet(net::fragment{buf.get_write(), buf.size()}, buf.release()));
    }
    virtual future<> flush() {
        return make_ready_future<>();
    }
    virtual future<> close() = 0;
};

class data_sink {
    std::unique_ptr<data_sink_impl> _dsi;
public:
    data_sink() = default;
    explicit data_sink(std::unique_ptr<data_sink_impl> dsi) : _dsi(std::move(dsi)) {}
    data_sink(data_sink&& x) = default;
    data_sink& operator=(data_sink&& x) = default;
    temporary_buffer<char> allocate_buffer(size_t size) {
        return _dsi->allocate_buffer(size);
    }
    future<> put(std::vector<temporary_buffer<char>> data) {
        return _dsi->put(std::move(data));
    }
    future<> put(temporary_buffer<char> data) {
        return _dsi->put(std::move(data));
    }
    future<> put(net::packet p) {
        return _dsi->put(std::move(p));
    }
    future<> flush() {
        return _dsi->flush();
    }
    future<> close() { return _dsi->close(); }
};



template <typename CharType>
class input_stream final {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    data_source _fd;
    temporary_buffer<CharType> _buf;
    bool _eof = false;
private:
    using tmp_buf = temporary_buffer<CharType>;
    size_t available() const { return _buf.size(); }
protected:
    void reset() { _buf = {}; }
    data_source* fd() { return &_fd; }
public:
    // Consumer concept, for consume() method:
    using unconsumed_remainder = std::optional<tmp_buf>;
    struct ConsumerConcept {
        // The consumer should operate on the data given to it, and
        // return a future "unconsumed remainder", which can be undefined
        // if the consumer consumed all the input given to it and is ready
        // for more, or defined when the consumer is done (and in that case
        // the value is the unconsumed part of the last data buffer - this
        // can also happen to be empty).
        future<unconsumed_remainder> operator()(tmp_buf data);
    };
    using char_type = CharType;
    input_stream() = default;
    explicit input_stream(data_source fd) : _fd(std::move(fd)), _buf(0) {}
    input_stream(input_stream&&) = default;
    input_stream& operator=(input_stream&&) = default;
    future<temporary_buffer<CharType>> read_exactly(size_t n);
    template <typename Consumer>
    future<> consume(Consumer& c);
    bool eof() { return _eof; }
    /// Returns some data from the stream, or an empty buffer on end of
    /// stream.
    future<tmp_buf> read();
    /// Returns up to n bytes from the stream, or an empty buffer on end of
    /// stream.
    future<tmp_buf> read_up_to(size_t n);
    /// Detaches the \c input_stream from the underlying data source.
    ///
    /// Waits for any background operations (for example, read-ahead) to
    /// complete, so that the any resources the stream is using can be
    /// safely destroyed.  An example is a \ref file resource used by
    /// the stream returned by make_file_input_stream().
    ///
    /// \return a future that becomes ready when this stream no longer
    ///         needs the data source.
    future<> close() {
        return _fd.close();
    }
    /// Ignores n next bytes from the stream.
    future<> skip(uint64_t n);
private:
    future<temporary_buffer<CharType>> read_exactly_part(size_t n, tmp_buf buf, size_t completed);
};

template <typename char_type, typename Size, Size max_size>
class basic_sstring;

using sstring = basic_sstring<char, uint32_t, 15>;

template <typename string_type = sstring, typename T>
inline string_type to_sstring(T value);

template <typename char_type, typename Size, Size max_size>
class basic_sstring {
    static_assert(
            (std::is_same<char_type, char>::value
             || std::is_same<char_type, signed char>::value
             || std::is_same<char_type, unsigned char>::value),
            "basic_sstring only supports single byte char types");
    union contents {
        struct external_type {
            char_type* str;
            Size size;
            int8_t pad;
        } external;
        struct internal_type {
            char_type str[max_size];
            int8_t size;
        } internal;
        static_assert(sizeof(external_type) <= sizeof(internal_type), "max_size too small");
        static_assert(max_size <= 127, "max_size too large");
    } u;
    bool is_internal() const noexcept {
        return u.internal.size >= 0;
    }
    bool is_external() const noexcept {
        return !is_internal();
    }
    const char_type* str() const {
        return is_internal() ? u.internal.str : u.external.str;
    }
    char_type* str() {
        return is_internal() ? u.internal.str : u.external.str;
    }
    template <typename string_type, typename T>
    static inline string_type to_sstring_sprintf(T value, const char* fmt) {
        char tmp[sizeof(value) * 3 + 2];
        auto len = std::sprintf(tmp, fmt, value);
        using ch_type = typename string_type::value_type;
        return string_type(reinterpret_cast<ch_type*>(tmp), len);
    }
    template <typename string_type>
    static inline string_type to_sstring(int value) {
        return to_sstring_sprintf<string_type>(value, "%d");
    }
    template <typename string_type>
    static inline string_type to_sstring(unsigned value) {
        return to_sstring_sprintf<string_type>(value, "%u");
    }

    template <typename string_type>
    static inline string_type to_sstring(long value) {
        return to_sstring_sprintf<string_type>(value, "%ld");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned long value) {
        return to_sstring_sprintf<string_type>(value, "%lu");
    }

    template <typename string_type>
    static inline string_type to_sstring(long long value) {
        return to_sstring_sprintf<string_type>(value, "%lld");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned long long value) {
        return to_sstring_sprintf<string_type>(value, "%llu");
    }

    template <typename string_type>
    static inline string_type to_sstring(float value) {
        return to_sstring_sprintf<string_type>(value, "%g");
    }

    template <typename string_type>
    static inline string_type to_sstring(double value) {
        return to_sstring_sprintf<string_type>(value, "%g");
    }

    template <typename string_type>
    static inline string_type to_sstring(long double value) {
        return to_sstring_sprintf<string_type>(value, "%Lg");
    }

    template <typename string_type>
    static inline string_type to_sstring(const char* value) {
        return string_type(value);
    }

    template <typename string_type>
    static inline string_type to_sstring(sstring value) {
        return value;
    }

    template <typename string_type>
    static inline string_type to_sstring(const temporary_buffer<char>& buf) {
        return string_type(buf.get(), buf.size());
    }
public:
    using value_type = char_type;
    using traits_type = std::char_traits<char_type>;
    using allocator_type = std::allocator<char_type>;
    using reference = char_type&;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using iterator = char_type*;
    using const_iterator = const char_type*;
    // FIXME: add reverse_iterator and friend
    using difference_type = ssize_t;  // std::make_signed_t<Size> can be too small
    using size_type = Size;
    static constexpr size_type  npos = static_cast<size_type>(-1);
public:
    struct initialized_later {};

    basic_sstring() noexcept {
        u.internal.size = 0;
        u.internal.str[0] = '\0';
    }
    basic_sstring(const basic_sstring& x) {
        if (x.is_internal()) {
            u.internal = x.u.internal;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(x.u.external.size + 1));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            std::copy(x.u.external.str, x.u.external.str + x.u.external.size + 1, u.external.str);
            u.external.size = x.u.external.size;
        }
    }
    basic_sstring(basic_sstring&& x) noexcept {
        u = x.u;
        x.u.internal.size = 0;
        x.u.internal.str[0] = '\0';
    }
    basic_sstring(initialized_later, size_t size) {
        if (size_type(size) != size) {
            throw std::overflow_error("sstring overflow");
        }
        if (size + 1 <= sizeof(u.internal.str)) {
            u.internal.str[size] = '\0';
            u.internal.size = size;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(size + 1));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            u.external.size = size;
            u.external.str[size] = '\0';
        }
    }
    basic_sstring(const char_type* x, size_t size) {
        if (size_type(size) != size) {
            throw std::overflow_error("sstring overflow");
        }
        if (size + 1 <= sizeof(u.internal.str)) {
            std::copy(x, x + size, u.internal.str);
            u.internal.str[size] = '\0';
            u.internal.size = size;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(size + 1));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            u.external.size = size;
            std::copy(x, x + size, u.external.str);
            u.external.str[size] = '\0';
        }
    }
    basic_sstring(size_t size, char_type x) : basic_sstring(initialized_later(), size) {
        memset(begin(), x, size);
    }
    basic_sstring(const char* x) : basic_sstring(reinterpret_cast<const char_type*>(x), std::strlen(x)) {}
    basic_sstring(std::basic_string<char_type>& x) : basic_sstring(x.c_str(), x.size()) {}
    basic_sstring(std::initializer_list<char_type> x) : basic_sstring(x.begin(), x.end() - x.begin()) {}
    basic_sstring(const char_type* b, const char_type* e) : basic_sstring(b, e - b) {}
    basic_sstring(const std::basic_string<char_type>& s)
        : basic_sstring(s.data(), s.size()) {}
    template <typename InputIterator>
    basic_sstring(InputIterator first, InputIterator last)
            : basic_sstring(initialized_later(), std::distance(first, last)) {
        std::copy(first, last, begin());
    }
    ~basic_sstring() noexcept {
        if (is_external()) {
            std::free(u.external.str);
        }
    }
    basic_sstring& operator=(const basic_sstring& x) {
        basic_sstring tmp(x);
        swap(tmp);
        return *this;
    }
    basic_sstring& operator=(basic_sstring&& x) noexcept {
        if (this != &x) {
            swap(x);
            x.reset();
        }
        return *this;
    }
    operator std::basic_string<char_type>() const {
        return { str(), size() };
    }
    size_t size() const noexcept {
        return is_internal() ? u.internal.size : u.external.size;
    }

    size_t length() const noexcept {
        return size();
    }

    size_t find(char_type t, size_t pos = 0) const noexcept {
        const char_type* it = str() + pos;
        const char_type* end = str() + size();
        while (it < end) {
            if (*it == t) {
                return it - str();
            }
            it++;
        }
        return npos;
    }

    size_t find(const basic_sstring& s, size_t pos = 0) const noexcept {
        const char_type* it = str() + pos;
        const char_type* end = str() + size();
        const char_type* c_str = s.str();
        const char_type* c_str_end = s.str() + s.size();

        while (it < end) {
            auto i = it;
            auto j = c_str;
            while ( i < end && j < c_str_end && *i == *j) {
                i++;
                j++;
            }
            if (j == c_str_end) {
                return it - str();
            }
            it++;
        }
        return npos;
    }

    /**
     * find_last_of find the last occurrence of c in the string.
     * When pos is specified, the search only includes characters
     * at or before position pos.
     *
     */
    size_t find_last_of (char_type c, size_t pos = npos) const noexcept {
        const char_type* str_start = str();
        if (size()) {
            if (pos >= size()) {
                pos = size() - 1;
            }
            const char_type* p = str_start + pos + 1;
            do {
                p--;
                if (*p == c) {
                    return (p - str_start);
                }
            } while (p != str_start);
        }
        return npos;
    }

    /**
     *  Append a C substring.
     *  @param s  The C string to append.
     *  @param n  The number of characters to append.
     *  @return  Reference to this string.
     */
    basic_sstring& append (const char_type* s, size_t n) {
        basic_sstring ret(initialized_later(), size() + n);
        std::copy(begin(), end(), ret.begin());
        std::copy(s, s + n, ret.begin() + size());
        *this = std::move(ret);
        return *this;
    }

    /**
     *  Resize string.
     *  @param n  new size.
     *  @param c  if n greater than current size character to fill newly allocated space with.
     */
    void resize(size_t n, const char_type c  = '\0') {
        if (n > size()) {
            *this += sstring(n - size(), c);
        } else if (n < size()) {
            if (is_internal()) {
                u.internal.size = n;
            } else if (n + 1 <= sizeof(u.internal.str)) {
                *this = sstring(u.external.str, n);
            } else {
                u.external.size = n;
            }
        }
    }

    /**
     *  Replace characters with a value of a C style substring.
     *
     */
    basic_sstring& replace(size_type pos, size_type n1, const char_type* s,
             size_type n2) {
        if (pos > size()) {
            throw std::out_of_range("sstring::replace out of range");
        }

        if (n1 > size() - pos) {
            n1 = size() - pos;
        }

        if (n1 == n2) {
            if (n2) {
                std::copy(s, s + n2, begin() + pos);
            }
            return *this;
        }
        basic_sstring ret(initialized_later(), size() + n2 - n1);
        char_type* p= ret.begin();
        std::copy(begin(), begin() + pos, p);
        p += pos;
        if (n2) {
            std::copy(s, s + n2, p);
        }
        p += n2;
        std::copy(begin() + pos + n1, end(), p);
        *this = std::move(ret);
        return *this;
    }

    template <class InputIterator>
    basic_sstring& replace (const_iterator i1, const_iterator i2,
            InputIterator first, InputIterator last) {
        if (i1 < begin() || i1 > end() || i2 < begin()) {
            throw std::out_of_range("sstring::replace out of range");
        }
        if (i2 > end()) {
            i2 = end();
        }

        if (i2 - i1 == last - first) {
            //in place replacement
            std::copy(first, last, const_cast<char_type*>(i1));
            return *this;
        }
        basic_sstring ret(initialized_later(), size() + (last - first) - (i2 - i1));
        char_type* p = ret.begin();
        p = std::copy(cbegin(), i1, p);
        p = std::copy(first, last, p);
        std::copy(i2, cend(), p);
        *this = std::move(ret);
        return *this;
    }

    iterator erase(iterator first, iterator last) {
        size_t pos = first - begin();
        replace(pos, last - first, nullptr, 0);
        return begin() + pos;
    }

    /**
     * Inserts additional characters into the string right before
     * the character indicated by p.
     */
    template <class InputIterator>
    void insert(const_iterator p, InputIterator beg, InputIterator end) {
        replace(p, p, beg, end);
    }

    /**
     *  Returns a read/write reference to the data at the last
     *  element of the string.
     *  This function shall not be called on empty strings.
     */
    reference
    back() noexcept {
        return operator[](size() - 1);
    }

    /**
     *  Returns a  read-only (constant) reference to the data at the last
     *  element of the string.
     *  This function shall not be called on empty strings.
     */
    const_reference
    back() const noexcept {
        return operator[](size() - 1);
    }

    basic_sstring substr(size_t from, size_t len = npos)  const {
        if (from > size()) {
            throw std::out_of_range("sstring::substr out of range");
        }
        if (len > size() - from) {
            len = size() - from;
        }
        if (len == 0) {
            return "";
        }
        return { str() + from , len };
    }

    const char_type& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("sstring::at out of range");
        }
        return *(str() + pos);
    }

    char_type& at(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range("sstring::at out of range");
        }
        return *(str() + pos);
    }

    bool empty() const noexcept {
        return u.internal.size == 0;
    }
    void reset() noexcept {
        if (is_external()) {
            std::free(u.external.str);
        }
        u.internal.size = 0;
        u.internal.str[0] = '\0';
    }
    temporary_buffer<char_type> release() && {
        if (is_external()) {
            auto ptr = u.external.str;
            auto size = u.external.size;
            u.external.str = nullptr;
            u.external.size = 0;
            return temporary_buffer<char_type>(ptr, size, make_free_deleter(ptr));
        } else {
            auto buf = temporary_buffer<char_type>(u.internal.size);
            std::copy(u.internal.str, u.internal.str + u.internal.size, buf.get_write());
            u.internal.size = 0;
            u.internal.str[0] = '\0';
            return buf;
        }
    }
    int compare(const basic_sstring& x) const noexcept {
        auto n = traits_type::compare(begin(), x.begin(), std::min(size(), x.size()));
        if (n != 0) {
            return n;
        }
        if (size() < x.size()) {
            return -1;
        } else if (size() > x.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    int compare(size_t pos, size_t sz, const basic_sstring& x) const {
        if (pos > size()) {
            throw std::out_of_range("pos larger than string size");
        }

        sz = std::min(size() - pos, sz);
        auto n = traits_type::compare(begin() + pos, x.begin(), std::min(sz, x.size()));
        if (n != 0) {
            return n;
        }
        if (sz < x.size()) {
            return -1;
        } else if (sz > x.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    void swap(basic_sstring& x) noexcept {
        contents tmp;
        tmp = x.u;
        x.u = u;
        u = tmp;
    }
    char_type* data() {
        return str();
    }
    const char_type* data() const {
        return str();
    }
    const char_type* c_str() const {
        return str();
    }
    const char_type* begin() const { return str(); }
    const char_type* end() const { return str() + size(); }
    const char_type* cbegin() const { return str(); }
    const char_type* cend() const { return str() + size(); }
    char_type* begin() { return str(); }
    char_type* end() { return str() + size(); }
    bool operator==(const basic_sstring& x) const {
        return size() == x.size() && std::equal(begin(), end(), x.begin());
    }
    bool operator!=(const basic_sstring& x) const {
        return !operator==(x);
    }
    bool operator<(const basic_sstring& x) const {
        return compare(x) < 0;
    }
    basic_sstring operator+(const basic_sstring& x) const {
        basic_sstring ret(initialized_later(), size() + x.size());
        std::copy(begin(), end(), ret.begin());
        std::copy(x.begin(), x.end(), ret.begin() + size());
        return ret;
    }
    basic_sstring& operator+=(const basic_sstring& x) {
        return *this = *this + x;
    }
    char_type& operator[](size_type pos) {
        return str()[pos];
    }
    const char_type& operator[](size_type pos) const {
        return str()[pos];
    }

    operator std::basic_string_view<char_type>() const {
        return std::basic_string_view<char_type>(str(), size());
    }

    template <typename string_type, typename T>
    friend inline string_type to_sstring(T value);
};
template <typename char_type, typename Size, Size max_size>
constexpr Size basic_sstring<char_type, Size, max_size>::npos;

template <typename char_type, typename size_type, size_type Max, size_type N>
inline
basic_sstring<char_type, size_type, Max>
operator+(const char(&s)[N], const basic_sstring<char_type, size_type, Max>& t) {
    using sstring = basic_sstring<char_type, size_type, Max>;
    // don't copy the terminating NUL character
    sstring ret(typename sstring::initialized_later(), N-1 + t.size());
    auto p = std::copy(std::begin(s), std::end(s)-1, ret.begin());
    std::copy(t.begin(), t.end(), p);
    return ret;
}

template <size_t N>
static inline
size_t str_len(const char(&s)[N]) { return N - 1; }

template <size_t N>
static inline
const char* str_begin(const char(&s)[N]) { return s; }

template <size_t N>
static inline
const char* str_end(const char(&s)[N]) { return str_begin(s) + str_len(s); }

template <typename char_type, typename size_type, size_type max_size>
static inline
const char_type* str_begin(const basic_sstring<char_type, size_type, max_size>& s) { return s.begin(); }

template <typename char_type, typename size_type, size_type max_size>
static inline
const char_type* str_end(const basic_sstring<char_type, size_type, max_size>& s) { return s.end(); }

template <typename char_type, typename size_type, size_type max_size>
static inline
size_type str_len(const basic_sstring<char_type, size_type, max_size>& s) { return s.size(); }

template <typename First, typename Second, typename... Tail>
static inline
const size_t str_len(const First& first, const Second& second, const Tail&... tail) {
    return str_len(first) + str_len(second, tail...);
}

template <typename char_type, typename size_type, size_type max_size>
inline
void swap(basic_sstring<char_type, size_type, max_size>& x,
          basic_sstring<char_type, size_type, max_size>& y) noexcept
{
    return x.swap(y);
}

template <typename char_type, typename size_type, size_type max_size, typename char_traits>
inline
std::basic_ostream<char_type, char_traits>&
operator<<(std::basic_ostream<char_type, char_traits>& os,
        const basic_sstring<char_type, size_type, max_size>& s) {
    return os.write(s.begin(), s.size());
}

template <typename char_type, typename size_type, size_type max_size, typename char_traits>
inline
std::basic_istream<char_type, char_traits>&
operator>>(std::basic_istream<char_type, char_traits>& is,
        basic_sstring<char_type, size_type, max_size>& s) {
    std::string tmp;
    is >> tmp;
    s = tmp;
    return is;
}

namespace std {

template <typename char_type, typename size_type, size_type max_size>
struct hash<basic_sstring<char_type, size_type, max_size>> {
    size_t operator()(const basic_sstring<char_type, size_type, max_size>& s) const {
        return std::hash<std::basic_string_view<char_type>>()(s);
    }
};

}

static inline
char* copy_str_to(char* dst) {
    return dst;
}

template <typename Head, typename... Tail>
static inline
char* copy_str_to(char* dst, const Head& head, const Tail&... tail) {
    return copy_str_to(std::copy(str_begin(head), str_end(head), dst), tail...);
}

template <typename String = sstring, typename... Args>
static String make_sstring(Args&&... args)
{
    String ret(sstring::initialized_later(), str_len(args...));
    copy_str_to(ret.begin(), args...);
    return ret;
}

template <typename string_type = sstring, typename T>
inline string_type to_sstring(T value) {
    return sstring::to_sstring<string_type>(value);
}

namespace std {
template <typename T>
inline
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    bool first = true;
    os << "{";
    for (auto&& elem : v) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << elem;
    }
    os << "}";
    return os;
}

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<Key, T, Hash, KeyEqual, Allocator>& v) {
    bool first = true;
    os << "{";
    for (auto&& elem : v) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << "{ " << elem.first << " -> " << elem.second << "}";
    }
    os << "}";
    return os;
}
}
//Facilitates data buffering before it's handed over to data_sink.
//When trim_to_size is true it's guaranteed that data sink will not receive
//chunks larger than the configured size, which could be the case when a
//single write call is made with data larger than the configured size.
//The data sink will not receive empty chunks.

template <typename CharType>
class scattered_message {
private:
    using fragment = net::fragment;
    using packet = net::packet;
    using char_type = CharType;
    packet _p;
public:
    scattered_message() {}
    scattered_message(scattered_message&&) = default;
    scattered_message(const scattered_message&) = delete;

    void append_static(const char_type* buf, size_t size) {
        if (size) {
            _p = packet(std::move(_p), fragment{(char_type*)buf, size}, deleter());
        }
    }

    template <size_t N>
    void append_static(const char_type(&s)[N]) {
        append_static(s, N - 1);
    }

    void append_static(const char_type* s) {
        append_static(s, strlen(s));
    }

    template <typename size_type, size_type max_size>
    void append_static(const basic_sstring<char_type, size_type, max_size>& s) {
        append_static(s.begin(), s.size());
    }

    void append_static(const std::string_view& s) {
        append_static(s.data(), s.size());
    }

    template <typename size_type, size_type max_size>
    void append(basic_sstring<char_type, size_type, max_size> s) {
        if (s.size()) {
            _p = packet(std::move(_p), std::move(s).release());
        }
    }

    template <typename size_type, size_type max_size, typename Callback>
    void append(const basic_sstring<char_type, size_type, max_size>& s, Callback callback) {
        if (s.size()) {
            _p = packet(std::move(_p), fragment{s.begin(), s.size()}, make_deleter(std::move(callback)));
        }
    }

    void reserve(int n_frags) {
        _p.reserve(n_frags);
    }

    packet release() && {
        return std::move(_p);
    }

    template <typename Callback>
    void on_delete(Callback callback) {
        _p = packet(std::move(_p), make_deleter(std::move(callback)));
    }

    operator bool() const {
        return _p.len();
    }

    size_t size() {
        return _p.len();
    }
};

template <typename CharType>
class output_stream final {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    data_sink _fd;
    temporary_buffer<CharType> _buf;
    net::packet _zc_bufs = net::packet::make_null_packet(); //zero copy buffers
    size_t _size = 0;
    size_t _begin = 0;
    size_t _end = 0;
    bool _trim_to_size = false;
    bool _batch_flushes = false;
    std::optional<promise<>> _in_batch;
    bool _flush = false;
    bool _flushing = false;
    std::exception_ptr _ex;
private:
    size_t available() const { return _end - _begin; }
    size_t possibly_available() const { return _size - _begin; }
    future<> split_and_put(temporary_buffer<CharType> buf);
    future<> put(temporary_buffer<CharType> buf);
    void poll_flush();
    future<> zero_copy_put(net::packet p);
    future<> zero_copy_split_and_put(net::packet p);
public:
    using char_type = CharType;
    output_stream() = default;
    output_stream(data_sink fd, size_t size, bool trim_to_size = false, bool batch_flushes = false)
        : _fd(std::move(fd)), _size(size), _trim_to_size(trim_to_size), _batch_flushes(batch_flushes) {}
    output_stream(output_stream&&) = default;
    output_stream& operator=(output_stream&&) = default;
    ~output_stream() { assert(!_in_batch); }
    future<> write(const char_type* buf, size_t n);
    future<> write(const char_type* buf);
    // template <typename StringChar>
    // future<> write(const basic_sstring<StringChar, SizeType, MaxSize>& s);
    future<> write(const std::basic_string<char_type>& s);
    future<> write(net::packet p);
    future<> write(scattered_message<char_type> msg);
    future<> write(temporary_buffer<char_type>);
    future<> flush();
    future<> close();
private:
    friend class reactor;
};



template <typename CharType> 
template <typename Consumer>
future<> input_stream<CharType>::consume(Consumer& consumer) {
    return repeat([&consumer, this] {
        if(_buf.empty() && !_eof) {
            return _fd.get().then([this] (tmp_buf buf) {
                _buf = std::move(buf);
                _eof = _buf.empty();
                return make_ready_future<stop_iteration>(stop_iteration::no);
            });
        }
        future<unconsumed_remainder> unconsumed = consumer(std::move(_buf));
        if (unconsumed.available()) {
            unconsumed_remainder u = std::get<0>(unconsumed.get());
            if (u) {
                // consumer is done
                _buf = std::move(u.value());
                return make_ready_future<stop_iteration>(stop_iteration::yes);
            }
            if (_eof) {
                return make_ready_future<stop_iteration>(stop_iteration::yes);
            }
            // If we're here, consumer consumed entire buffer and is ready for
            // more now. So we do not return, and rather continue the loop.
            // TODO: if we did too many iterations, schedule a call to
            // consume() instead of continuing the loop.
            return make_ready_future<stop_iteration>(stop_iteration::no);
        } else {
            // TODO: here we wait for the consumer to finish the previous
            // buffer (fulfilling "unconsumed") before starting to read the
            // next one. Consider reading ahead.
            return unconsumed.then([this] (unconsumed_remainder u) {
                if (u) {
                    // consumer is done
                    _buf = std::move(u.value());
                    return make_ready_future<stop_iteration>(stop_iteration::yes);
                } else {
                    // consumer consumed entire buffer, and is ready for more
                    return make_ready_future<stop_iteration>(stop_iteration::no);
                }
            });
        }
    });
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_up_to(size_t n) {
    using tmp_buf = temporary_buffer<CharType>;
    if (_buf.empty()) {
        if (_eof) {
            return make_ready_future<tmp_buf>();
        } else {
            return _fd.get().then([this, n] (tmp_buf buf) {
                _eof = buf.empty();
                _buf = std::move(buf);
                return read_up_to(n);
            });
        }
    } else if (_buf.size() <= n) {
        // easy case: steal buffer, return to caller
        return make_ready_future<tmp_buf>(std::move(_buf));
    } else {
        // buffer is larger than n, so share its head with a caller
        auto front = _buf.share(0, n);
        _buf.trim_front(n);
        return make_ready_future<tmp_buf>(std::move(front));
    }
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read() {
    using tmp_buf = temporary_buffer<CharType>;
    if (_eof) {
        return make_ready_future<tmp_buf>();
    }
    if (_buf.empty()) {
        return _fd.get().then([this] (tmp_buf buf) {
            _eof = buf.empty();
            return make_ready_future<tmp_buf>(std::move(buf));
        });
    } else {
        return make_ready_future<tmp_buf>(std::move(_buf));
    }
}

template <typename CharType>
future<>
input_stream<CharType>::skip(uint64_t n) {
    auto skip_buf = std::min(n, _buf.size());
    _buf.trim_front(skip_buf);
    n -= skip_buf;
    if (!n) {
        return make_ready_future<>();
    }
    return _fd.skip(n).then([this] (temporary_buffer<CharType> buffer) {
        _buf = std::move(buffer);
    });
}

// Writes @buf in chunks of _size length. The last chunk is buffered if smaller.
template <typename CharType>
future<>
output_stream<CharType>::split_and_put(temporary_buffer<CharType> buf) {
    assert(_end == 0);

    return repeat([this, buf = std::move(buf)] () mutable {
        if (buf.size() < _size) {
            if (!_buf) {
                _buf = _fd.allocate_buffer(_size);
            }
            std::copy(buf.get(), buf.get() + buf.size(), _buf.get_write());
            _end = buf.size();
            return make_ready_future<stop_iteration>(stop_iteration::yes);
        }
        auto chunk = buf.share(0, _size);
        buf.trim_front(_size);
        return put(std::move(chunk)).then([] {
            return stop_iteration::no;
        });
    });
}

template <typename CharType>
future<>
output_stream<CharType>::write(const char_type* buf, size_t n) {
    assert(!_zc_bufs && "Mixing buffered writes and zero-copy writes not supported yet");
    auto bulk_threshold = _end ? (2 * _size - _end) : _size;
    if (n >= bulk_threshold) {
        if (_end) {
            auto now = _size - _end;
            std::copy(buf, buf + now, _buf.get_write() + _end);
            _end = _size;
            temporary_buffer<char> tmp = _fd.allocate_buffer(n - now);
            std::copy(buf + now, buf + n, tmp.get_write());
            _buf.trim(_end);
            _end = 0;
            return put(std::move(_buf)).then([this, tmp = std::move(tmp)]() mutable {
                if (_trim_to_size) {
                    return split_and_put(std::move(tmp));
                } else {
                    return put(std::move(tmp));
                }
            });
        } else {
            temporary_buffer<char> tmp = _fd.allocate_buffer(n);
            std::copy(buf, buf + n, tmp.get_write());
            if (_trim_to_size) {
                return split_and_put(std::move(tmp));
            } else {
                return put(std::move(tmp));
            }
        }
    }

    if (!_buf) {
        _buf = _fd.allocate_buffer(_size);
    }

    auto now = std::min(n, _size - _end);
    std::copy(buf, buf + now, _buf.get_write() + _end);
    _end += now;
    if (now == n) {
        return make_ready_future<>();
    } else {
        temporary_buffer<char> next = _fd.allocate_buffer(_size);
        std::copy(buf + now, buf + n, next.get_write());
        _end = n - now;
        std::swap(next, _buf);
        return put(std::move(next));
    }
}


template <typename CharType>
future<>
output_stream<CharType>::flush() {
    if (!_batch_flushes) {
        if (_end) {
            _buf.trim(_end);
            _end = 0;
            return put(std::move(_buf)).then([this] {
                return _fd.flush();
            });
        } else if (_zc_bufs) {
            return zero_copy_put(std::move(_zc_bufs)).then([this] {
                return _fd.flush();
            });
        }
    } else {
        if (_ex) {
            // flush is a good time to deliver outstanding errors
            return make_exception_future<>(std::move(_ex));
        } else {
            _flush = true;
            if (!_in_batch) {
                add_to_flush_poller(this);
                _in_batch = promise<>();
            }
        }
    }
    return make_ready_future<>();
}



template <typename CharType>
future<>
output_stream<CharType>::put(temporary_buffer<CharType> buf) {
    // if flush is scheduled, disable it, so it will not try to write in parallel
    _flush = false;
    if (_flushing) {
        // flush in progress, wait for it to end before continuing
        return _in_batch.value().get_future().then([this, buf = std::move(buf)] () mutable {
            return _fd.put(std::move(buf));
        });
    } else {
        return _fd.put(std::move(buf));
    }
}

template <typename CharType>
void output_stream<CharType>::poll_flush() {
    if (!_flush) {
        // flush was canceled, do nothing
        _flushing = false;
        _in_batch.value().set_value();
        _in_batch = std::nullopt;
        return;
    }
    auto f = make_ready_future();
    _flush = false;
    _flushing = true; // make whoever wants to write into the fd to wait for flush to complete

    if (_end) {
        // send whatever is in the buffer right now
        _buf.trim(_end);
        _end = 0;
        f = _fd.put(std::move(_buf));
    } else if(_zc_bufs) {
        f = _fd.put(std::move(_zc_bufs));
    }

    f.then([this] {
        return _fd.flush();
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
        } catch (...) {
            _ex = std::current_exception();
        }
        // if flush() was called while flushing flush once more
        poll_flush();
    });
}

template <typename CharType>
future<>
output_stream<CharType>::close() {
    return flush().finally([this] {
        if (_in_batch) {
            return _in_batch.value().get_future();
        } else {
            return make_ready_future();
        }
    }).then([this] {
        // report final exception as close error
        if (_ex) {
            std::rethrow_exception(_ex);
        }
    }).finally([this] {
        return _fd.close();
    });
}


template<typename CharType>
inline
future<> output_stream<CharType>::write(const char_type* buf) {
    return write(buf, strlen(buf));
}

// template<typename CharType>
// template<typename StringChar, typename SizeType, SizeType MaxSize>
// inline
// future<> output_stream<CharType>::write(const std::basic_string<CharType>& s) {
//     return write(reinterpret_cast<const CharType *>(s.data()), s.size());
// }

template<typename CharType>
inline
future<> output_stream<CharType>::write(const std::basic_string<CharType>& s) {
     return write(reinterpret_cast<const CharType *>(s.data()), s.size());
}

template<typename CharType>
future<> output_stream<CharType>::write(scattered_message<CharType> msg) {
    return write(std::move(msg).release());
}

template<typename CharType>
future<>
output_stream<CharType>::zero_copy_put(net::packet p) {
    // if flush is scheduled, disable it, so it will not try to write in parallel
    _flush = false;
    if (_flushing) {
        // flush in progress, wait for it to end before continuing
        return _in_batch.value().get_future().then([this, p = std::move(p)] () mutable {
            return _fd.put(std::move(p));
        });
    } else {
        return _fd.put(std::move(p));
    }
}

// Writes @p in chunks of _size length. The last chunk is buffered if smaller.
template <typename CharType>
future<>
output_stream<CharType>::zero_copy_split_and_put(net::packet p) {
    return repeat([this, p = std::move(p)] () mutable {
        if (p.len() < _size) {
            if (p.len()) {
                _zc_bufs = std::move(p);
            } else {
                _zc_bufs = net::packet::make_null_packet();
            }
            return make_ready_future<stop_iteration>(stop_iteration::yes);
        }
        auto chunk = p.share(0, _size);
        p.trim_front(_size);
        return zero_copy_put(std::move(chunk)).then([] {
            return stop_iteration::no;
        });
    });
}

template<typename CharType>
future<> output_stream<CharType>::write(net::packet p) {
    static_assert(std::is_same<CharType, char>::value, "packet works on char");

    if (p.len() != 0) {
        assert(!_end && "Mixing buffered writes and zero-copy writes not supported yet");

        if (_zc_bufs) {
            _zc_bufs.append(std::move(p));
        } else {
            _zc_bufs = std::move(p);
        }

        if (_zc_bufs.len() >= _size) {
            if (_trim_to_size) {
                return zero_copy_split_and_put(std::move(_zc_bufs));
            } else {
                return zero_copy_put(std::move(_zc_bufs));
            }
        }
    }
    return make_ready_future<>();
}

template<typename CharType>
future<> output_stream<CharType>::write(temporary_buffer<CharType> p) {
    if (p.empty()) {
        return make_ready_future<>();
    }
    assert(!_end && "Mixing buffered writes and zero-copy writes not supported yet");

    return write(net::packet(std::move(p)));
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_exactly_part(size_t n, tmp_buf out, size_t completed) {
    if (available()) {
        auto now = std::min(n - completed, available());
        std::copy(_buf.get(), _buf.get() + now, out.get_write() + completed);
        _buf.trim_front(now);
        completed += now;
    }
    if (completed == n) {
        return make_ready_future<tmp_buf>(std::move(out));
    }

    // _buf is now empty
    return _fd.get().then([this, n, out = std::move(out), completed] (auto buf) mutable {
        if (buf.size() == 0) {
            _eof = true;
            return make_ready_future<tmp_buf>(std::move(buf));
        }
        _buf = std::move(buf);
        return this->read_exactly_part(n, std::move(out), completed);
    });
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_exactly(size_t n) {
    if (_buf.size() == n) {
        // easy case: steal buffer, return to caller
        return make_ready_future<tmp_buf>(std::move(_buf));
    } else if (_buf.size() > n) {
        // buffer large enough, share it with caller
        auto front = _buf.share(0, n);
        _buf.trim_front(n);
        return make_ready_future<tmp_buf>(std::move(front));
    } else if (_buf.size() == 0) {
        // buffer is empty: grab one and retry
        return _fd.get().then([this, n] (auto buf) mutable {
            if (buf.size() == 0) {
                _eof = true;
                return make_ready_future<tmp_buf>(std::move(buf));
            }
            _buf = std::move(buf);
            return this->read_exactly(n);
        });
    } else {
        // buffer too small: start copy/read loop
        tmp_buf b(n);
        return read_exactly_part(n, std::move(b), 0);
    }
}


