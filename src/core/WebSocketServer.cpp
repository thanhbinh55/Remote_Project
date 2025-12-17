#include "WebSocketServer.hpp"
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>

// Include các Module để xử lý logic đặc thù
#include "../modules/WebcamManager.hpp"
#include "../modules/ScreenManager.hpp"
#include "../modules/FileManager.hpp"
#include "../modules/EdgeManager.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using json = nlohmann::json;

WebSocketServer::WebSocketServer(net::io_context& ioc, unsigned short port, CommandDispatcher& dispatcher)
    : ioc_(ioc), acceptor_(ioc, {tcp::v4(), port}), dispatcher_(dispatcher) {}

void WebSocketServer::run() {
    do_accept();
    std::cout << "[SERVER] Listening on port " << acceptor_.local_endpoint().port() << "...\n";
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(sessions_mtx_);
    for (auto* ws : sessions_) {
        try {
            if (ws->is_open()) {
                ws->text(true);
                ws->write(net::buffer(message));
            }
        } catch (...) {}
    }
}

void WebSocketServer::do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::thread(&WebSocketServer::handle_session, this, std::move(socket)).detach();
        }
        do_accept();
    });
}

void WebSocketServer::handle_session(tcp::socket socket) {
    auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
    auto ws_mutex = std::make_shared<std::mutex>();

    try {
        std::string client_ip = ws->next_layer().remote_endpoint().address().to_string();
        std::cout << "[SESSION] CONNECTED: " << client_ip << "\n";

        ws->accept();
        {
            std::lock_guard<std::mutex> lock(sessions_mtx_);
            sessions_.push_back(ws.get());
        }

        for (;;) {
            beast::flat_buffer buffer;
            ws->read(buffer);
            std::string req_str = beast::buffers_to_string(buffer.data());
            json request = json::parse(req_str);
            json response;

            std::string module = request.value("module", "");
            std::string cmd = request.value("command", "");

            // --- XỬ LÝ LOGIC ---
            if (module == "WEBCAM") {
                auto* cam = dynamic_cast<WebcamManager*>(dispatcher_.get_module("WEBCAM"));
                if (cam) {
                    if (cmd == "START_STREAM") {
                        std::cout << "[MAIN] Webcam Stream Started\n";
                        cam->start_stream([ws, ws_mutex](const std::vector<uint8_t>& data) {
                            std::lock_guard<std::mutex> lock(*ws_mutex);
                            try { if(ws->is_open()) { ws->binary(true); ws->write(net::buffer(data.data(), data.size())); } } catch (...) {}
                        });
                        response = {{"status", "success"}, {"message", "Stream Started"}};
                    } 
                    else if (cmd == "STOP_STREAM") {
                        cam->stop_stream();
                        response = {{"status", "success"}, {"message", "Stream Stopped"}};
                    }
                }
            }
            else if (module == "SCREEN" && cmd == "CAPTURE_BINARY") {
                std::vector<uint8_t> jpg_data;
                std::string err;
                bool should_save = true;
                if (request.contains("payload") && request["payload"].contains("save")) {
                    should_save = request["payload"]["save"].get<bool>();
                }
                if (ScreenManager::capture_screen_data(jpg_data, err, should_save)) {
                    {
                        std::lock_guard<std::mutex> lock(*ws_mutex);
                        ws->binary(true);
                        ws->write(net::buffer(jpg_data.data(), jpg_data.size()));
                    }
                    if (should_save) response = {{"module", "SCREEN"}, {"command", "CAPTURE_COMPLETE"}, {"status", "success"}};
                    else continue;
                } else {
                    response = {{"status", "error"}, {"message", err}};
                }
            }
            else if (module == "FILE" && cmd == "GET") {
                std::string filename = "";
                if (request.contains("payload") && request["payload"].contains("name")) filename = request["payload"]["name"];
                std::vector<uint8_t> file_data;
                if (!filename.empty() && FileManager::read_file_binary(filename, file_data)) {
                    {
                        std::lock_guard<std::mutex> lock(*ws_mutex);
                        ws->binary(true);
                        ws->write(net::buffer(file_data.data(), file_data.size()));
                    }
                    response = {{"status", "success"}, {"command", "GET_COMPLETE"}, {"file", filename}};
                } else {
                    response = {{"status", "error"}, {"message", "File not found"}};
                }
            }
            else if (module == "EDGE") {
                response = dispatcher_.dispatch(request);
                // Logging logic cho Edge
                if (cmd.find("GET") != std::string::npos) std::cout << "[EDGE] Extracted data\n";
            }
            else {
                // Mặc định cho Process, App, System...
                response = dispatcher_.dispatch(request);
            }

            // Gửi phản hồi JSON
            {
                std::lock_guard<std::mutex> lock(*ws_mutex);
                ws->text(true);
                ws->write(net::buffer(response.dump()));
            }
        }
    } catch (...) {}

    // Clean up session
    {
        std::lock_guard<std::mutex> lock(sessions_mtx_);
        sessions_.erase(std::remove(sessions_.begin(), sessions_.end(), ws.get()), sessions_.end());
    }
}