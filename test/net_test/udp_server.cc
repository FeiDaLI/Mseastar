#include "../../include/app/app-template.hh"
#include "../../include/future/distributed.hh"
using namespace net;
using namespace std::chrono_literals;

class udp_server {
private:
    udp_channel _chan;
    timer<> _stats_timer;
    uint64_t _n_sent {};
public:
    void start(uint16_t port) {
        std::cout<<"run udp server start"<<std::endl;
        ipv4_addr listen_addr{port};
        engine().net().print_stack();
        _chan = engine().net().make_udp_channel(listen_addr);
        if(_chan._impl!=nullptr)
        {
            std::cout<<"chan_._impl: "<<_chan._impl<<std::endl;
            // std::cout<<"test receive"<<std::endl;
            // auto f = chan_._impl->receive();
        }
        else
        {
            std::cout<<"chan_._impl: "<<nullptr<<std::endl;
        }
        // _chan = engine().net().make_udp_channel(listen_addr);
        // std::cout<<"run udp server end"<<std::endl;
        _stats_timer.set_callback([this] {
            // std::cout << "Out: " << _n_sent << " pps" << std::endl;
            _n_sent = 0;
        });
        // std::cout<<"run stat timer end"<<std::endl;
        //为什么定时器不输出?
        _stats_timer.arm_periodic(1s);
        // std::cout<<"before keep doing"<<std::endl;
        keep_doing([this]()mutable {
            std::cout << "Out: " << _n_sent << " pps" << std::endl;
            return _chan.receive().then([this] (udp_datagram dgram) mutable  {
                // std::cout<<dgram.get_data()<<std::endl;
                return _chan.send(dgram.get_src(), std::move(dgram.get_data())).then([this] {
                    _n_sent++;
                });
            });
        });
    }
    // FIXME: we should properly tear down the service here.
    future<> stop() {
        return make_ready_future<>();
    }
};

namespace bpo = boost::program_options;



/*测试代码*/
future<int> f(int x){
    return make_ready_future<int>(x+1);
}
/*测试代码*/

int main(int ac, char ** av) {
    app_template app;
    app.add_options()("port", bpo::value<uint16_t>()->default_value(10000), "UDP server port");

    return app.run_deprecated(ac, av, [&] {
        auto&& config = app.configuration();
        uint16_t port = config["port"].as<uint16_t>();
        auto server = new distributed<udp_server>;
        server->start().then([server = std::move(server), port] () mutable {
            engine().at_exit([server] {
                return server->stop();
            });
            server->invoke_on_all(&udp_server::start, port);
        }).then([port] {
            std::cout << "Seastar UDP server listening on port " << port << " ...\n";
        });
    });
}
