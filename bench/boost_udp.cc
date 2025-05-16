#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

class server {
public:
    server(boost::asio::io_context& io_context, int port)
        : socket_(io_context, udp::endpoint(udp::v4(), port)) {
        std::cout << "服务器正在监听 0.0.0.0:" << port << std::endl;
        start_receive();
    }

private:
    void start_receive() {
        socket_.async_receive_from(
            boost::asio::buffer(data_), sender_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    // 打印接收到的消息（注释掉了）
                    // std::cout << "收到来自 " 
                    //           << sender_endpoint_.address().to_string() << ":" 
                    //           << sender_endpoint_.port() 
                    //           << " 的消息: " 
                    //           << std::string(data_, bytes_recvd) 
                    //           << std::endl;
                    
                    // 回显消息
                    socket_.async_send_to(
                        boost::asio::buffer(data_, bytes_recvd),
                        sender_endpoint_,
                        [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/) {
                            start_receive();
                        });
                } else {
                    start_receive();
                }
            });
    }
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
};

int main() {
    try {
        boost::asio::io_context io_context;
        server s(io_context, 41236);
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "服务器错误: " << e.what() << std::endl;
    }
    return 0;
}