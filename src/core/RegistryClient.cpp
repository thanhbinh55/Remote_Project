#include "RegistryClient.hpp"
#include "../utils/SystemUtils.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using udp = net::ip::udp;
using json = nlohmann::json;

RegistryClient::RegistryClient() {}

RegistryClient::~RegistryClient() {
    stop_monitoring();
}

void RegistryClient::stop_monitoring() {
    is_running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void RegistryClient::start_monitoring(int ws_port) {
    ws_port_ = ws_port;
    is_running_ = true;

    // Tạo luồng chạy ngầm
    monitor_thread_ = std::thread([this]() {
        std::cout << "[REGISTRY] Background monitor started...\n";

        while (is_running_) {
            if (!is_connected_) {
                // TRƯỜNG HỢP 1: CHƯA KẾT NỐI -> Tìm và Đăng ký
                
                // B1: Nếu chưa có IP -> Tìm IP
                if (registry_host_.empty()) {
                    if (attempt_discovery()) {
                        std::cout << "[REGISTRY] Found Server at: " << registry_host_ << "\n";
                    } else {
                        // Không tìm thấy -> Đợi 5s rồi thử lại
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                        continue; 
                    }
                }

                // B2: Có IP rồi -> Thử Đăng ký
                if (attempt_register()) {
                    is_connected_ = true;
                    std::cout << "[REGISTRY] Connected & Registered successfully.\n";
                } else {
                    std::cout << "[REGISTRY] Register failed. Retrying in 5s...\n";
                    // Đăng ký lỗi -> Có thể IP sai hoặc Server chết -> Xóa IP để tìm lại từ đầu
                    registry_host_ = ""; 
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }

            } else {
                // TRƯỜNG HỢP 2: ĐÃ KẾT NỐI -> Gửi Heartbeat duy trì
                
                if (!send_heartbeat()) {
                    std::cout << "[REGISTRY] Heartbeat failed! Connection lost.\n";
                    is_connected_ = false;
                    registry_host_ = ""; // Reset IP để tìm server mới (phòng trường hợp server đổi IP)
                }
                
                // Đợi 10s trước lần heartbeat tiếp theo
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }
    });
}

// --- CÁC HÀM XỬ LÝ CHI TIẾT (ĐÃ SỬA ĐỂ TRẢ VỀ BOOL) ---

bool RegistryClient::attempt_discovery() {
    try {
        net::io_context io;
        udp::socket socket(io);
        socket.open(udp::v4());
        socket.set_option(udp::socket::reuse_address(true));
        socket.set_option(net::socket_base::broadcast(true));
        socket.bind(udp::endpoint(udp::v4(), 0));

        udp::endpoint broadcast_ep(net::ip::address_v4::broadcast(), 8888);
        std::string msg = "DISCOVER_REGISTRY";
        socket.send_to(net::buffer(msg), broadcast_ep);

        char data[256] = {};
        udp::endpoint sender;
        socket.non_blocking(true); 

        // Chỉ đợi 2 giây mỗi lần quét để không block luồng quá lâu
        for (int i = 0; i < 20; i++) { 
            boost::system::error_code ec;
            size_t len = socket.receive_from(net::buffer(data), sender, 0, ec);
            if (!ec && len > 0) {
                std::string reply(data, len);
                if (reply.find("REGISTRY_IP:") == 0) {
                    registry_host_ = reply.substr(12);
                    return true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (...) {}
    return false;
}

bool RegistryClient::attempt_register() {
    try {
        net::io_context ctx;
        tcp::resolver resolver(ctx);
        beast::tcp_stream stream(ctx);
        
        // Timeout 3 giây cho kết nối
        stream.expires_after(std::chrono::seconds(3));
        auto const results = resolver.resolve(registry_host_, "3000");
        stream.connect(results);

        json body = {
            {"machineId", SystemUtils::get_computer_name()},
            {"ip", SystemUtils::get_local_ip()},
            {"os", SystemUtils::get_os_name()},
            {"wsPort", ws_port_},
            {"tags", json::array({"lab", "student"})}
        };

        http::request<http::string_body> req{ http::verb::post, "/api/agents/register", 11 };
        req.set(http::field::host, registry_host_);
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();
        http::write(stream, req);
        
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);
        
        stream.socket().shutdown(tcp::socket::shutdown_both);
        
        if (res.result() == http::status::ok || res.result() == http::status::created) {
            return true;
        }
    } catch (...) {}
    return false;
}

bool RegistryClient::send_heartbeat() {
    try {
        net::io_context ctx;
        tcp::resolver resolver(ctx);
        beast::tcp_stream stream(ctx);
        
        stream.expires_after(std::chrono::seconds(3)); // Timeout nhanh
        auto const results = resolver.resolve(registry_host_, "3000");
        stream.connect(results);

        json body = { {"machineId", SystemUtils::get_computer_name()} };
        http::request<http::string_body> req{ http::verb::post, "/api/agents/heartbeat", 11 };
        req.set(http::field::host, registry_host_);
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);
        stream.socket().shutdown(tcp::socket::shutdown_both);

        if (res.result() == http::status::ok) return true;
    } catch (...) {}
    return false; // Lỗi kết nối hoặc server trả về lỗi
}