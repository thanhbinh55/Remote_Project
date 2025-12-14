#include "InputManager.hpp"
#include <iostream>

// Cần định nghĩa Display nếu muốn dùng XTest sau này, giờ để tạm NULL
// Display* display = nullptr; 

json InputManager::handle_command(const json& request) {
    // Parser lệnh từ JSON và gọi các hàm private tương ứng
    // Hiện tại trả về thông báo chưa hỗ trợ để test server
    return {
        {"status", "success"}, 
        {"message", "[Linux] Input received but not executed (Dummy Mode)"}
    };
}

void InputManager::move_mouse(double x, double y) {
    // Code di chuyển chuột Linux (XTest) sẽ viết ở đây
}

void InputManager::mouse_btn(const std::string& btn, const std::string& action) {
    // Code click chuột Linux sẽ viết ở đây
}

void InputManager::mouse_scroll(int delta) {
    // Code cuộn chuột Linux sẽ viết ở đây
}

void InputManager::key_event(int key_code, bool is_down) {
    // Code nhấn phím Linux sẽ viết ở đây
}