#ifndef FILE_DESC_HH
#define FILE_DESC_HH
#include <iostream>        // std::cout, std::cerr, std::endl
#include <memory>          // std::unique_ptr, std::shared_ptr 等（如果用到）
#include <stdexcept>       // std::system_error
#include <string>          // std::string
#include <optional>        // std::optional (requires C++17 or higher)
#include <cassert>         // assert
#include <cerrno>          // errno
#include <cstring>         // strerror, memset, strlen
#include <fcntl.h>         // open(), O_* flags
#include <unistd.h>        // close(), read(), write(), dup()
#include <sys/types.h>     // socket(), bind(), listen(), etc.
#include <sys/socket.h>    // socket(), bind(), accept(), etc.
#include <netinet/in.h>    // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>     // inet_ntoa()
#include <sys/ioctl.h>     // ioctl()
#include <sys/stat.h>      // fstat()
#include <sys/mman.h>      // mmap(), MAP_SHARED, etc.
#include <sys/epoll.h>     // epoll_create1()
#include <sys/timerfd.h>   // timerfd_create(), timerfd_settime()
#include <sys/eventfd.h>   // eventfd()
#include <netdb.h>         // getaddrinfo(), getnameinfo()
#include <sys/sendfile.h>  // sendfile() (if used)
#include <sys/uio.h>       // readv(), writev(), iovec
#include "net.hh"
struct mmap_deleter {
    size_t _size;
    void operator()(void* ptr) const;
};


using mmap_area = std::unique_ptr<char[], mmap_deleter>;
inline void throw_system_error_on(bool condition, const char* what_arg);
class file_desc {
    int _fd;
public:
    file_desc() = delete;
    file_desc(const file_desc&) = delete;
    file_desc(file_desc&& x) : _fd(x._fd) { x._fd = -1; }
    ~file_desc() { if (_fd != -1) { ::close(_fd); } }
    void operator=(const file_desc&) = delete;
    file_desc& operator=(file_desc&& x) {
        if (this != &x) {
            std::swap(_fd, x._fd);
            if (x._fd != -1) {
                x.close();
            }
        }
        return *this;
    }
    void close() {
        assert(_fd != -1);
        auto r = ::close(_fd);
        throw_system_error_on(r == -1, "close");
        _fd = -1;
    }
    int get() const { return _fd; }
    static file_desc open(std::string name, int flags, mode_t mode = 0) {
        int fd = ::open(name.c_str(), flags, mode);
        throw_system_error_on(fd == -1, "open");
        return file_desc(fd);
    }
    static file_desc socket(int family, int type, int protocol = 0) {
        int fd = ::socket(family, type, protocol);
        throw_system_error_on(fd == -1, "socket");
        return file_desc(fd);
    }
    static file_desc eventfd(unsigned initval, int flags) {
        int fd = ::eventfd(initval, flags);
        throw_system_error_on(fd == -1, "eventfd");
        return file_desc(fd);
    }
    static file_desc epoll_create(int flags = 0) {
        int fd = ::epoll_create1(flags);
        throw_system_error_on(fd == -1, "epoll_create1");
        return file_desc(fd);
    }
    static file_desc timerfd_create(int clockid, int flags) {
        int fd = ::timerfd_create(clockid, flags);
        throw_system_error_on(fd == -1, "timerfd_create");
        return file_desc(fd);
    }
    static file_desc temporary(std::string directory);
    file_desc dup() const {
        int fd = ::dup(get());
        throw_system_error_on(fd == -1, "dup");
        return file_desc(fd);
    }
    file_desc accept(sockaddr& sa, socklen_t& sl, int flags = 0) {
        auto ret = ::accept4(_fd, &sa, &sl, flags);
        throw_system_error_on(ret == -1, "accept4");
        return file_desc(ret);
    }
    void shutdown(int how) {
        auto ret = ::shutdown(_fd, how);
        if (ret == -1 && errno != ENOTCONN) {
            throw_system_error_on(ret == -1, "shutdown");
        }
    }
    void truncate(size_t size) {
        auto ret = ::ftruncate(_fd, size);
        throw_system_error_on(ret, "ftruncate");
    }
    int ioctl(int request) {
        return ioctl(request, 0);
    }
    int ioctl(int request, int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    int ioctl(int request, unsigned int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X&& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int setsockopt(int level, int optname, X&& data) {
        int r = ::setsockopt(_fd, level, optname, &data, sizeof(data));
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    int setsockopt(int level, int optname, const char* data) {
        int r = ::setsockopt(_fd, level, optname, data, strlen(data) + 1);
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    template <typename Data>
    Data getsockopt(int level, int optname) {
        Data data;
        socklen_t len = sizeof(data);
        memset(&data, 0, len);
        int r = ::getsockopt(_fd, level, optname, &data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return data;
    }
    int getsockopt(int level, int optname, char* data, socklen_t len) {
        int r = ::getsockopt(_fd, level, optname, data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return r;
    }
    size_t size() {
        struct stat buf;
        auto r = ::fstat(_fd, &buf);
        throw_system_error_on(r == -1, "fstat");
        return buf.st_size;
    }
    std::optional<size_t> read(void* buffer, size_t len) {
        auto r = ::read(_fd, buffer, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "read");
        return { size_t(r) };
    }
    std::optional<ssize_t> recv(void* buffer, size_t len, int flags) {
        auto r = ::recv(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recv");
        return { ssize_t(r) };
    }
    std::optional<size_t> recvmsg(msghdr* mh, int flags) {
        auto r = ::recvmsg(_fd, mh, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recvmsg");
        return { size_t(r) };
    }
    std::optional<size_t> send(const void* buffer, size_t len, int flags) {
        auto r = ::send(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "send");
        return { size_t(r) };
    }
    std::optional<size_t> sendto(net::socket_address& addr, const void* buf, size_t len, int flags) {
        auto r = ::sendto(_fd, buf, len, flags, &addr.u.sa, sizeof(addr.u.sas));
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendto");
        return { size_t(r) };
    }
    std::optional<size_t> sendmsg(const msghdr* msg, int flags) {
        auto r = ::sendmsg(_fd, msg, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendmsg");
        return { size_t(r) };
    }
    void bind(sockaddr& sa, socklen_t sl) {
        if(sa.sa_family == AF_INET){

            sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&sa);
            std::cout << "Bound to: "
                      << inet_ntoa(sin->sin_addr) << ":"
                      << ntohs(sin->sin_port) << std::endl;
        }
        auto r = ::bind(_fd, &sa, sl);
        if (r == -1) {
            std::cerr << "bind failed: "; // 先输出错误前缀
            throw_system_error_on(true, "bind"); // 这会抛出异常并输出错误信息
        } else {
            std::cout << "bind successful" << std::endl;
            // 如果需要，可以打印绑定的地址和端口信息
            if (sa.sa_family == AF_INET) {
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&sa);
                std::cout << "Bound to: "
                          << inet_ntoa(sin->sin_addr) << ":"
                          << ntohs(sin->sin_port) << std::endl;
            }
            // 对于IPv6的情况可以类似处理
        }
    }
    void connect(sockaddr& sa, socklen_t sl) {
        auto r = ::connect(_fd, &sa, sl);
        if (r == -1 && errno == EINPROGRESS) {
            return;
        }
        throw_system_error_on(r == -1, "connect");
    }
    net::socket_address get_address() {
        net::socket_address addr;
        auto len = (socklen_t) sizeof(addr.u.sas);
        auto r = ::getsockname(_fd, &addr.u.sa, &len);
        throw_system_error_on(r == -1, "getsockname");
        return addr;
    }
    void listen(int backlog) {
        auto fd = ::listen(_fd, backlog);
        throw_system_error_on(fd == -1, "listen");
    }
    std::optional<size_t> write(const void* buf, size_t len) {
        auto r = ::write(_fd, buf, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "write");
        return { size_t(r) };
    }
    std::optional<size_t> writev(const iovec *iov, int iovcnt) {
        auto r = ::writev(_fd, iov, iovcnt);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "writev");
        return { size_t(r) };
    }
    size_t pread(void* buf, size_t len, off_t off) {
        auto r = ::pread(_fd, buf, len, off);
        throw_system_error_on(r == -1, "pread");
        return size_t(r);
    }
    void timerfd_settime(int flags, const itimerspec& its) {
        auto r = ::timerfd_settime(_fd, flags, &its, NULL);
        throw_system_error_on(r == -1, "timerfd_settime");
    }

    mmap_area map(size_t size, unsigned prot, unsigned flags, size_t offset,
            void* addr = nullptr) {
        void *x = mmap(addr, size, prot, flags, _fd, offset);
        throw_system_error_on(x == MAP_FAILED, "mmap");
        return mmap_area(static_cast<char*>(x), mmap_deleter{size});
    }

    mmap_area map_shared_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_SHARED, offset);
    }

    mmap_area map_shared_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_SHARED, offset);
    }

    mmap_area map_private_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_PRIVATE, offset);
    }

    mmap_area map_private_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_PRIVATE, offset);
    }
private:
    file_desc(int fd) : _fd(fd) {}
};



#endif