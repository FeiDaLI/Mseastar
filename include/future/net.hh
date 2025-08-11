// #pragma once
// #ifndef NET_HH
// #define NET_HH
// #include <arpa/inet.h>
// #include <string>    
// #include <vector>    
// #include <memory>      
// #include <optional>        
// #include <new>             
// #include <algorithm>  
// #include <ostream>  
// #include <functional>  
// #include <cstdint>         
// #include <cstddef>         
// #include <sys/socket.h>
// #include <optional>
// #include "future.hh"
// #include "../util/unaligned.hh"
// #include "../util/const.hh"
// #include "temp_buffer.hh"
// #include "stream.hh"
// #include <chrono>
// #include <variant>
// namespace net {
//     class connected_socket_impl; // 前向声明
//     class socket_impl;           // 前向声明
//     class server_socket_impl;   // 前向声明
//     class udp_channel_impl;     // 前向声明
//     class get_impl;             // 前向声明
// }


// inline uint64_t ntohq(uint64_t v) {
//     return __builtin_bswap64(v);
// }
// inline uint64_t htonq(uint64_t v) {
//     return __builtin_bswap64(v);
// }
// namespace net {

// inline void ntoh() {}
// inline void hton() {}

// inline uint8_t ntoh(uint8_t x) { return x; }
// inline uint8_t hton(uint8_t x) { return x; }
// inline uint16_t ntoh(uint16_t x) { return ntohs(x); }
// inline uint16_t hton(uint16_t x) { return htons(x); }
// inline uint32_t ntoh(uint32_t x) { return ntohl(x); }
// inline uint32_t hton(uint32_t x) { return htonl(x); }
// inline uint64_t ntoh(uint64_t x) { return ntohq(x); }
// inline uint64_t hton(uint64_t x) { return htonq(x); }

// inline int8_t ntoh(int8_t x) { return x; }
// inline int8_t hton(int8_t x) { return x; }
// inline int16_t ntoh(int16_t x) { return ntohs(x); }
// inline int16_t hton(int16_t x) { return htons(x); }
// inline int32_t ntoh(int32_t x) { return ntohl(x); }
// inline int32_t hton(int32_t x) { return htonl(x); }
// inline int64_t ntoh(int64_t x) { return ntohq(x); }
// inline int64_t hton(int64_t x) { return htonq(x); }

// // Deprecated alias net::packed<> for unaligned<> from unaligned.hh.
// // TODO: get rid of this alias.
// template <typename T> using packed = unaligned<T>;

// template <typename T> inline T ntoh(const packed<T>& x) {
//     T v = x;
//     return ntoh(v);
// }

// template <typename T> inline T hton(const packed<T>& x) {
//     T v = x;
//     return hton(v);
// }

// template <typename T> inline std::ostream& operator<<(std::ostream& os, const packed<T>& v) {
//     auto x = v.raw;
//     return os << x;
// }

// inline void ntoh_inplace() {};
// inline void hton_inplace() {};

// template <typename First, typename... Rest>
// inline void ntoh_inplace(First& first, Rest&... rest) {
//     first = ntoh(first);
//     ntoh_inplace(std::forward<Rest&>(rest)...);
// }

// template <typename First, typename... Rest>
// inline void hton_inplace(First& first, Rest&... rest) {
//     first = hton(first);
//     hton_inplace(std::forward<Rest&>(rest)...);
// }

// template <class T>
// inline
// T ntoh(const T& x) {
//     T tmp = x;
//     tmp.adjust_endianness([] (auto&&... what) { ntoh_inplace(std::forward<decltype(what)&>(what)...); });
//     return tmp;
// }

// template <class T>
// inline
// T hton(const T& x) {
//     T tmp = x;
//     tmp.adjust_endianness([] (auto&&... what) { hton_inplace(std::forward<decltype(what)&>(what)...); });
//     return tmp;
// }

// }


// namespace net{
// class socket {
//     std::unique_ptr<::net::socket_impl> _si;
// public:
//     ~socket();
//     explicit socket(std::unique_ptr<::net::socket_impl> si);
//     socket(socket&&) noexcept;
//     socket& operator=(socket&&) noexcept;
//     future<connected_socket> connect(socket_address sa, socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}),transport proto = transport::TCP);
//     void shutdown();
// };
// }

// namespace net {
// class inet_address {
// public:
//     enum class family {
//         INET = AF_INET, INET6 = AF_INET6
//     };
// private:
//     family _in_family;
//     union {
//         ::in_addr _in;
//         ::in6_addr _in6;
//     };
// public:
//     inet_address();
//     inet_address(::in_addr i);
//     inet_address(::in6_addr i);
//     // NOTE: does _not_ resolve the address. Only parses
//     // ipv4/ipv6 numerical address
//     inet_address(const std::string&);
//     inet_address(inet_address&&) = default;
//     inet_address(const inet_address&) = default;
//     inet_address& operator=(const inet_address&) = default;
//     bool operator==(const inet_address&) const;
//     family in_family() const {
//         return _in_family;
//     }
//     size_t size() const;
//     const void * data() const;
//     operator const ::in_addr&() const;
//     operator const ::in6_addr&() const;
//     future<std::string> hostname() const;
//     future<std::vector<std::string>> aliases() const;
//     static future<inet_address> find(const std::string&);
//     static future<inet_address> find(const std::string&, family);
//     static future<std::vector<inet_address>> find_all(const std::string&);
//     static future<std::vector<inet_address>> find_all(const std::string&, family);
// };
//     std::ostream& operator<<(std::ostream&, const inet_address&);
//     std::ostream& operator<<(std::ostream&, const inet_address::family&);
// }

// // struct ipv4_addr {
// //     uint32_t ip;
// //     uint16_t port;
// //     ipv4_addr() : ip(0), port(0) {}
// //     ipv4_addr(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
// //     ipv4_addr(uint16_t port) : ip(0), port(port) {}
// //     ipv4_addr(const std::string &addr);
// //     ipv4_addr(const std::string &addr, uint16_t port);
// //     ipv4_addr(const net::inet_address&, uint16_t);
// //     ipv4_addr(const net::socket_address &sa) {
// //         ip = net::ntoh(sa.u.in.sin_addr.s_addr);
// //         port = net::ntoh(sa.u.in.sin_port);
// //     }
// //     ipv4_addr(net::socket_address &&sa) : ipv4_addr(sa) {}
// // };


// struct ipv4_addr {
//     uint32_t ip;
//     uint16_t port;
//     ipv4_addr() : ip(0), port(0) {}
//     ipv4_addr(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
//     ipv4_addr(uint16_t port) : ip(0), port(port) {}
//     ipv4_addr(const std::string &addr);
//     ipv4_addr(const std::string &addr, uint16_t port);
//     ipv4_addr(const net::inet_address&, uint16_t);
    
//     // 修改构造函数，使用 ::sockaddr_in 而不是 net::socket_address
//     ipv4_addr(const ::sockaddr_in &sa) {
//         ip = net::ntoh(sa.sin_addr.s_addr);
//         port = net::ntoh(sa.sin_port);
//     }
//     ipv4_addr(::sockaddr_in &&sa) : ipv4_addr(sa) {}
// };

// namespace net{

// class udp_datagram_impl {
// public:
//     virtual ~udp_datagram_impl() {};
//     virtual ipv4_addr get_src() = 0;
//     virtual ipv4_addr get_dst() = 0;
//     virtual uint16_t get_dst_port() = 0;
//     virtual packet& get_data() = 0;
// };

// class udp_datagram final {
// private:
//     std::unique_ptr<udp_datagram_impl> _impl;
// public:
//     udp_datagram(std::unique_ptr<udp_datagram_impl>&& impl) : _impl(std::move(impl)) {};
//     ipv4_addr get_src() { return _impl->get_src(); }
//     ipv4_addr get_dst() { return _impl->get_dst(); }
//     uint16_t get_dst_port() { return _impl->get_dst_port(); }
//     packet& get_data() { return _impl->get_data(); }
// };
// class udp_channel_impl {
// public:
//         virtual ~udp_channel_impl() {};
//         virtual future<udp_datagram> receive() = 0;
//         virtual future<> send(ipv4_addr dst, const char* msg) = 0;
//         virtual future<> send(ipv4_addr dst, packet p) = 0;
//         virtual bool is_closed() const = 0;
//         virtual void close() = 0;
// };
// class posix_udp_channel;
// class udp_channel {
// public:
//     std::unique_ptr<udp_channel_impl> _impl;
//     udp_channel();
//     udp_channel(std::unique_ptr<udp_channel_impl>);
//     ~udp_channel();
//     udp_channel(udp_channel&&);
//     udp_channel& operator=(udp_channel&&);
//     future<udp_datagram> receive();
//     future<> send(ipv4_addr dst, const char* msg);
//     future<> send(ipv4_addr dst, packet p);
//     bool is_closed() const;
//     void close();
// };
// }

// using keepalive_params = std::variant<tcp_keepalive_params, sctp_keepalive_params>;
// class connected_socket_impl;
// class socket_impl;
// class server_socket_impl;
// class udp_channel_impl;
// class get_impl;
// class connected_socket {
//     friend class net::get_impl;
//     std::unique_ptr<net::connected_socket_impl> _csi;
// public:
//     connected_socket();
//     ~connected_socket();
//     explicit connected_socket(std::unique_ptr<net::connected_socket_impl> csi);
//     connected_socket(connected_socket&& cs) noexcept;
//     connected_socket& operator=(connected_socket&& cs) noexcept;
//     input_stream<char> input();
//     output_stream<char> output(size_t buffer_size = 8192);
//     void set_nodelay(bool nodelay);
//     bool get_nodelay() const;
//     void set_keepalive(bool keepalive);
//     bool get_keepalive() const;
//     void set_keepalive_parameters(const net::keepalive_params& p);
//     net::keepalive_params get_keepalive_parameters() const;
//     void shutdown_output();
//     void shutdown_input();
// };


// // std::ostream& operator<<(std::ostream&, const net::socket_address&);
// enum class transport {
//     TCP = IPPROTO_TCP,
//     SCTP = IPPROTO_SCTP
// };

// namespace net {
//     class inet_address;
// }

// struct listen_options {
//     transport proto = transport::TCP;
//     bool reuse_address = false;
//     listen_options(bool rua = false)
//         : reuse_address(rua){}
// };






// namespace net {
// /// \cond internal
// class connected_socket_impl {
// public:
//     virtual ~connected_socket_impl() {}
//     virtual data_source source() = 0;
//     virtual data_sink sink() = 0;
//     virtual void shutdown_input() = 0;
//     virtual void shutdown_output() = 0;
//     virtual void set_nodelay(bool nodelay) = 0;
//     virtual bool get_nodelay() const = 0;
//     virtual void set_keepalive(bool keepalive) = 0;
//     virtual bool get_keepalive() const = 0;
//     virtual void set_keepalive_parameters(const keepalive_params&) = 0;
//     virtual keepalive_params get_keepalive_parameters() const = 0;
// };

// class socket_impl {
// public:
//     virtual ~socket_impl() {}
//     virtual future<connected_socket> connect(socket_address sa, socket_address local, transport proto = transport::TCP) = 0;
//     virtual void shutdown() = 0;
// };

// class server_socket_impl {
// public:
//     virtual ~server_socket_impl() {}
//     virtual future<connected_socket, socket_address> accept() = 0;
//     virtual void abort_accept() = 0;
// };
// /// \endcond
// }

// class server_socket {
//     std::unique_ptr<net::server_socket_impl> _ssi;
// public:
//     server_socket();
//     explicit server_socket(std::unique_ptr<net::server_socket_impl> ssi);
//     server_socket(server_socket&& ss) noexcept;
//     ~server_socket();
//     /// Move-assigns a \c server_socket object.
//     server_socket& operator=(server_socket&& cs) noexcept;
//     future<connected_socket, net::socket_address> accept();
//     void abort_accept();
// };

// #include <chrono>
// #include <variant>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <cstring>

// struct ipv4_addr;
// namespace net {
// // see linux tcp(7) for parameter explanation
// struct tcp_keepalive_params {
//     std::chrono::seconds idle; // TCP_KEEPIDLE
//     std::chrono::seconds interval; // TCP_KEEPINTVL
//     unsigned count; // TCP_KEEPCNT
// };
// // see linux sctp(7) for parameter explanation
// struct sctp_keepalive_params {
//     std::chrono::seconds interval; // spp_hbinterval
//     unsigned count; // spp_pathmaxrt
// };

// class socket_address {
// public:
//     union {
//         ::sockaddr_storage sas;
//         ::sockaddr sa;
//         ::sockaddr_in in;
//     } u;
//     socket_address(sockaddr_in sa) {
//         u.in = sa;
//     }
//     socket_address(ipv4_addr);
//     socket_address() = default;
//     ::sockaddr& as_posix_sockaddr() { return u.sa; }
//     ::sockaddr_in& as_posix_sockaddr_in() { return u.in; }
//     const ::sockaddr& as_posix_sockaddr() const { return u.sa; }
//     const ::sockaddr_in& as_posix_sockaddr_in() const { return u.in; }
//     bool operator==(const socket_address&) const;
// };

// }



// static inline net::socket_address make_ipv4_address(ipv4_addr addr) {
//     net::socket_address sa;
//     sa.u.in.sin_family = AF_INET;
//     sa.u.in.sin_port = htons(addr.port);
//     sa.u.in.sin_addr.s_addr = htonl(addr.ip);
//     return sa;
// }

// inline net::socket_address make_ipv4_address(uint32_t ip, uint16_t port) {
//     net::socket_address sa;
//     sa.u.in.sin_family = AF_INET;
//     sa.u.in.sin_port = htons(port);
//     sa.u.in.sin_addr.s_addr = htonl(ip);
//     return sa;
// }

// #endif


#pragma once
#ifndef NET_HH
#define NET_HH
#include <arpa/inet.h>
#include <string>    
#include <vector>    
#include <memory>      
#include <optional>        
#include <new>             
#include <algorithm>  
#include <ostream>  
#include <functional>  
#include <cstdint>         
#include <cstddef>         
#include <sys/socket.h>
#include <optional>
#include "future.hh"
#include "../util/unaligned.hh"
#include "../util/const.hh"
#include "temp_buffer.hh"
#include "stream.hh"
#include <chrono>
#include <variant>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
// Forward declarations
namespace net {
    class connected_socket_impl;
    class socket_impl;
    class server_socket_impl;
    class udp_channel_impl;
    class get_impl;
    class inet_address;
    class socket_address;
    // TCP/SCTP keepalive parameters
    struct tcp_keepalive_params {
        std::chrono::seconds idle; // TCP_KEEPIDLE
        std::chrono::seconds interval; // TCP_KEEPINTVL
        unsigned count; // TCP_KEEPCNT
    };    
    struct sctp_keepalive_params {
        std::chrono::seconds interval; // spp_hbinterval
        unsigned count; // spp_pathmaxrt
    };
}

struct ipv4_addr;
class connected_socket;
class server_socket;

enum class transport {
    TCP = IPPROTO_TCP,
    SCTP = IPPROTO_SCTP
};

using keepalive_params = net::tcp_keepalive_params;

struct listen_options {
    transport proto = transport::TCP;
    bool reuse_address = false;
    listen_options(bool rua = false)
        : reuse_address(rua){}
};

// Network byte order conversion functions
inline uint64_t ntohq(uint64_t v) {
    return __builtin_bswap64(v);
}
inline uint64_t htonq(uint64_t v) {
    return __builtin_bswap64(v);
}

namespace net {

inline void ntoh() {}
inline void hton() {}

inline uint8_t ntoh(uint8_t x) { return x; }
inline uint8_t hton(uint8_t x) { return x; }
inline uint16_t ntoh(uint16_t x) { return ntohs(x); }
inline uint16_t hton(uint16_t x) { return htons(x); }
inline uint32_t ntoh(uint32_t x) { return ntohl(x); }
inline uint32_t hton(uint32_t x) { return htonl(x); }
inline uint64_t ntoh(uint64_t x) { return ntohq(x); }
inline uint64_t hton(uint64_t x) { return htonq(x); }

inline int8_t ntoh(int8_t x) { return x; }
inline int8_t hton(int8_t x) { return x; }
inline int16_t ntoh(int16_t x) { return ntohs(x); }
inline int16_t hton(int16_t x) { return htons(x); }
inline int32_t ntoh(int32_t x) { return ntohl(x); }
inline int32_t hton(int32_t x) { return htonl(x); }
inline int64_t ntoh(int64_t x) { return ntohq(x); }
inline int64_t hton(int64_t x) { return htonq(x); }

// Deprecated alias net::packed<> for unaligned<> from unaligned.hh.
// TODO: get rid of this alias.
template <typename T> using packed = unaligned<T>;

template <typename T> inline T ntoh(const packed<T>& x) {
    T v = x;
    return ntoh(v);
}

template <typename T> inline T hton(const packed<T>& x) {
    T v = x;
    return hton(v);
}

template <typename T> inline std::ostream& operator<<(std::ostream& os, const packed<T>& v) {
    auto x = v.raw;
    return os << x;
}

inline void ntoh_inplace() {};
inline void hton_inplace() {};

template <typename First, typename... Rest>
inline void ntoh_inplace(First& first, Rest&... rest) {
    first = ntoh(first);
    ntoh_inplace(std::forward<Rest&>(rest)...);
}

template <typename First, typename... Rest>
inline void hton_inplace(First& first, Rest&... rest) {
    first = hton(first);
    hton_inplace(std::forward<Rest&>(rest)...);
}

template <class T>
inline
T ntoh(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { ntoh_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}

template <class T>
inline
T hton(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { hton_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}

// socket_address class definition
class socket_address {
public:
    union {
        ::sockaddr_storage sas;
        ::sockaddr sa;
        ::sockaddr_in in;
    } u;
    socket_address(sockaddr_in sa) {
        u.in = sa;
    }
    socket_address(ipv4_addr);
    socket_address() = default;
    ::sockaddr& as_posix_sockaddr() { return u.sa; }
    ::sockaddr_in& as_posix_sockaddr_in() { return u.in; }
    const ::sockaddr& as_posix_sockaddr() const { return u.sa; }
    const ::sockaddr_in& as_posix_sockaddr_in() const { return u.in; }
    bool operator==(const socket_address&) const;
};

// inet_address class definition
class inet_address {
public:
    enum class family {
        INET = AF_INET, INET6 = AF_INET6
    };
private:
    family _in_family;
    union {
        ::in_addr _in;
        ::in6_addr _in6;
    };
public:
    inet_address();
    inet_address(::in_addr i);
    inet_address(::in6_addr i);
    // NOTE: does _not_ resolve the address. Only parses
    // ipv4/ipv6 numerical address
    inet_address(const std::string&);
    inet_address(inet_address&&) = default;
    inet_address(const inet_address&) = default;
    inet_address& operator=(const inet_address&) = default;
    bool operator==(const inet_address&) const;
    family in_family() const {
        return _in_family;
    }
    size_t size() const;
    const void * data() const;
    operator const ::in_addr&() const;
    operator const ::in6_addr&() const;
    future<std::string> hostname() const;
    future<std::vector<std::string>> aliases() const;
    static future<inet_address> find(const std::string&);
    static future<inet_address> find(const std::string&, family);
    static future<std::vector<inet_address>> find_all(const std::string&);
    static future<std::vector<inet_address>> find_all(const std::string&, family);
};

std::ostream& operator<<(std::ostream&, const inet_address&);
std::ostream& operator<<(std::ostream&, const inet_address::family&);

// socket class definition
class socket {
    std::unique_ptr<::net::socket_impl> _si;
public:
    ~socket();
    explicit socket(std::unique_ptr<::net::socket_impl> si);
    socket(socket&&) noexcept;
    socket& operator=(socket&&) noexcept;
    future<connected_socket> connect(socket_address sa, socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}),transport proto = transport::TCP);
    void shutdown();
};

}

// ipv4_addr struct definition
struct ipv4_addr {
    uint32_t ip;
    uint16_t port;
    ipv4_addr() : ip(0), port(0) {}
    ipv4_addr(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
    ipv4_addr(uint16_t port) : ip(0), port(port) {}
    ipv4_addr(const std::string &addr);
    ipv4_addr(const std::string &addr, uint16_t port);
    ipv4_addr(const net::inet_address&, uint16_t);
    
    // 修改构造函数，使用 ::sockaddr_in 而不是 net::socket_address
    ipv4_addr(const ::sockaddr_in &sa) {
        ip = net::ntoh(sa.sin_addr.s_addr);
        port = net::ntoh(sa.sin_port);
    }
    ipv4_addr(::sockaddr_in &&sa) : ipv4_addr(sa) {}
    
    // 添加接受 net::socket_address 的构造函数
    ipv4_addr(const net::socket_address &sa) {
        ip = net::ntoh(sa.u.in.sin_addr.s_addr);
        port = net::ntoh(sa.u.in.sin_port);
    }
    ipv4_addr(net::socket_address &&sa) : ipv4_addr(sa) {}
};

// Helper functions for socket_address creation
static inline net::socket_address make_ipv4_address(ipv4_addr addr) {
    net::socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(addr.port);
    sa.u.in.sin_addr.s_addr = htonl(addr.ip);
    return sa;
}

inline net::socket_address make_ipv4_address(uint32_t ip, uint16_t port) {
    net::socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(port);
    sa.u.in.sin_addr.s_addr = htonl(ip);
    return sa;
}

namespace net{

class udp_datagram_impl {
public:
    virtual ~udp_datagram_impl() {};
    virtual ipv4_addr get_src() = 0;
    virtual ipv4_addr get_dst() = 0;
    virtual uint16_t get_dst_port() = 0;
    virtual packet& get_data() = 0;
};

class udp_datagram final {
private:
    std::unique_ptr<udp_datagram_impl> _impl;
public:
    udp_datagram(std::unique_ptr<udp_datagram_impl>&& impl) : _impl(std::move(impl)) {};
    ipv4_addr get_src() { return _impl->get_src(); }
    ipv4_addr get_dst() { return _impl->get_dst(); }
    uint16_t get_dst_port() { return _impl->get_dst_port(); }
    packet& get_data() { return _impl->get_data(); }
};

class udp_channel_impl {
public:
        virtual ~udp_channel_impl() {};
        virtual future<udp_datagram> receive() = 0;
        virtual future<> send(ipv4_addr dst, const char* msg) = 0;
        virtual future<> send(ipv4_addr dst, packet p) = 0;
        virtual bool is_closed() const = 0;
        virtual void close() = 0;
};

class posix_udp_channel;

class udp_channel {
public:
    std::unique_ptr<udp_channel_impl> _impl;
    udp_channel();
    udp_channel(std::unique_ptr<udp_channel_impl>);
    ~udp_channel();
    udp_channel(udp_channel&&);
    udp_channel& operator=(udp_channel&&);
    future<udp_datagram> receive();
    future<> send(ipv4_addr dst, const char* msg);
    future<> send(ipv4_addr dst, packet p);
    bool is_closed() const;
    void close();
};

}

// connected_socket class definition
class connected_socket {
    friend class net::get_impl;
    std::unique_ptr<net::connected_socket_impl> _csi;
public:
    connected_socket();
    ~connected_socket();
    explicit connected_socket(std::unique_ptr<net::connected_socket_impl> csi);
    connected_socket(connected_socket&& cs) noexcept;
    connected_socket& operator=(connected_socket&& cs) noexcept;
    input_stream<char> input();
    output_stream<char> output(size_t buffer_size = 8192);
    void set_nodelay(bool nodelay);
    bool get_nodelay() const;
    void set_keepalive(bool keepalive);
    bool get_keepalive() const;
    void set_keepalive_parameters(const keepalive_params& p);
    keepalive_params get_keepalive_parameters() const;
    void shutdown_output();
    void shutdown_input();
};

// server_socket class definition
class server_socket {
    std::unique_ptr<net::server_socket_impl> _ssi;
public:
    server_socket();
    explicit server_socket(std::unique_ptr<net::server_socket_impl> ssi);
    server_socket(server_socket&& ss) noexcept;
    ~server_socket();
    /// Move-assigns a \c server_socket object.
    server_socket& operator=(server_socket&& cs) noexcept;
    future<connected_socket, net::socket_address> accept();
    void abort_accept();
};

namespace net {
/// \cond internal
class connected_socket_impl {
public:
    virtual ~connected_socket_impl() {}
    virtual data_source source() = 0;
    virtual data_sink sink() = 0;
    virtual void shutdown_input() = 0;
    virtual void shutdown_output() = 0;
    virtual void set_nodelay(bool nodelay) = 0;
    virtual bool get_nodelay() const = 0;
    virtual void set_keepalive(bool keepalive) = 0;
    virtual bool get_keepalive() const = 0;
    virtual void set_keepalive_parameters(const keepalive_params&) = 0;
    virtual keepalive_params get_keepalive_parameters() const = 0;
};

class socket_impl {
public:
    virtual ~socket_impl() {}
    virtual future<connected_socket> connect(socket_address sa, socket_address local, transport proto = transport::TCP) = 0;
    virtual void shutdown() = 0;
};

class server_socket_impl {
public:
    virtual ~server_socket_impl() {}
    virtual future<connected_socket, socket_address> accept() = 0;
    virtual void abort_accept() = 0;
};
/// \endcond
}

#endif