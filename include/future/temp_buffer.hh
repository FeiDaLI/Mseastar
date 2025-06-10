#ifndef TEMP_BUFFER_HH
#define TEMP_BUFFER_HH
#include "../util/deleter.hh"





template <typename CharType>
class temporary_buffer {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    CharType* _buffer;
    size_t _size;
    deleter _deleter;
public:
    explicit temporary_buffer(size_t size)
        : _buffer(static_cast<CharType*>(malloc(size * sizeof(CharType)))), _size(size)
        , _deleter(make_free_deleter(_buffer)) {
        if (size && !_buffer) {
            throw std::bad_alloc();
        }
    }
    //explicit temporary_buffer(CharType* borrow, size_t size) : _buffer(borrow), _size(size) {}
    /// Creates an empty \c temporary_buffer that does not point at anything.
    temporary_buffer()
        : _buffer(nullptr)
        , _size(0) {}
    temporary_buffer(const temporary_buffer&) = delete;
    /// Moves a \c temporary_buffer.
    temporary_buffer(temporary_buffer&& x) noexcept : _buffer(x._buffer), _size(x._size), _deleter(std::move(x._deleter)) {
        x._buffer = nullptr;
        x._size = 0;
    }
    temporary_buffer(CharType* buf, size_t size, deleter d)
        : _buffer(buf), _size(size), _deleter(std::move(d)) {}
    temporary_buffer(const CharType* src, size_t size) : temporary_buffer(size) {
        std::copy_n(src, size, _buffer);
    }
    void operator=(const temporary_buffer&) = delete;
    /// Moves a \c temporary_buffer.
    temporary_buffer& operator=(temporary_buffer&& x) {
        if (this != &x) {
            _buffer = x._buffer;
            _size = x._size;
            _deleter = std::move(x._deleter);
            x._buffer = nullptr;
            x._size = 0;
        }
        return *this;
    }
    /// Gets a pointer to the beginning of the buffer.
    const CharType* get() const { return _buffer; }
    /// Gets a writable pointer to the beginning of the buffer.  Use only
    /// when you are certain no user expects the buffer data not to change.
    CharType* get_write() { return _buffer; }
    /// Gets the buffer size.
    size_t size() const { return _size; }
    /// Gets a pointer to the beginning of the buffer.
    const CharType* begin() const { return _buffer; }
    /// Gets a pointer to the end of the buffer.
    const CharType* end() const { return _buffer + _size; }
    temporary_buffer prefix(size_t size) && {
        auto ret = std::move(*this);
        ret._size = size;
        return ret;
    }
    CharType operator[](size_t pos) const {
        return _buffer[pos];
    }
    bool empty() const { return !size(); }
    explicit operator bool() const { return size(); }
    temporary_buffer share() {
        return temporary_buffer(_buffer, _size, _deleter.share());
    }
    temporary_buffer share(size_t pos, size_t len) {
        auto ret = share();
        ret._buffer += pos;
        ret._size = len;
        return ret;
    }
    void trim_front(size_t pos) {
        _buffer += pos;
        _size -= pos;
    }
    void trim(size_t pos) {
        _size = pos;
    }
    deleter release() {
        return std::move(_deleter);
    }
    static temporary_buffer aligned(size_t alignment, size_t size) {
        void *ptr = nullptr;
        auto ret = ::posix_memalign(&ptr, alignment, size * sizeof(CharType));
        auto buf = static_cast<CharType*>(ptr);
        if (ret) {
            throw std::bad_alloc();
        }
        return temporary_buffer(buf, size, make_free_deleter(buf));
    }

    /// Compare contents of this buffer with another buffer for equality
    ///
    /// \param o buffer to compare with
    /// \return true if and only if contents are the same
    bool operator==(const temporary_buffer<char>& o) const {
        return size() == o.size() && std::equal(begin(), end(), o.begin());
    }

    /// Compare contents of this buffer with another buffer for inequality
    ///
    /// \param o buffer to compare with
    /// \return true if and only if contents are not the same
    bool operator!=(const temporary_buffer<char>& o) const {
        return !(*this == o);
    }
};

#endif