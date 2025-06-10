#pragma once
#ifndef NETWORK_STACK_HH
#define NETWORK_STACK_HH

#include "future.hh"
#include "net.hh"

class network_stack
{
public:
    virtual void print_stack(){}

    virtual ~network_stack() {}
    virtual server_socket listen(net::socket_address sa, listen_options opts) = 0;
    // FIXME: local parameter assumes ipv4 for now, fix when adding other AF
    future<connected_socket> connect(net::socket_address sa, net::socket_address local = net::socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}), transport proto = transport::TCP) {
        return socket().connect(sa, local, proto);
    }
    virtual net::socket socket() = 0;
    virtual ::net::udp_channel make_udp_channel(ipv4_addr addr = {}) = 0;
    virtual future<> initialize() {
        return make_ready_future();
    }
    virtual bool has_per_core_namespace() = 0;
};

#endif