#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <vector>
#include <mutex>
#include <memory>
#include "CommandDispatcher.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketServer {
public:
    WebSocketServer(net::io_context& ioc, unsigned short port, CommandDispatcher& dispatcher);
    void run();
    void broadcast(const std::string& message);

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    CommandDispatcher& dispatcher_;

    // Quản lý session
    std::vector<boost::beast::websocket::stream<tcp::socket>*> sessions_;
    std::mutex sessions_mtx_;

    void do_accept();
    void handle_session(tcp::socket socket);
};