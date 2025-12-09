#include "InputManager.hpp"
#include <iostream>

// Lấy độ phân giải màn hình thật của Server
static int GetScreenWidth() { return GetSystemMetrics(SM_CXSCREEN); }
static int GetScreenHeight() { return GetSystemMetrics(SM_CYSCREEN); }

json InputManager::handle_command(const json& request) {
    std::string cmd = request.value("command", "");
    json p = request.value("payload", json::object());

    if (cmd == "MOUSE_MOVE") {
        move_mouse(p.value("x", 0.0), p.value("y", 0.0));
    }
    else if (cmd == "MOUSE_BTN") {
        mouse_btn(p.value("btn", "left"), p.value("type", "down"));
    }
    else if (cmd == "MOUSE_WHEEL") {
        mouse_scroll(p.value("delta", 0));
    }
    else if (cmd == "KEY_EVENT") {
        bool is_down = (p.value("type", "up") == "down");
        int key = p.value("key", 0);
        key_event(key, is_down);
    }
    return {{"status", "ok"}};
}

// 1. DI CHUYỂN CHUỘT
void InputManager::move_mouse(double x, double y) {
    // Chuyển đổi toạ độ tỉ lệ (0.0 - 1.0) sang toạ độ tuyệt đối (0 - 65535)
    // 65535 là chuẩn toạ độ của SendInput
    int absX = static_cast<int>(x * 65535);
    int absY = static_cast<int>(y * 65535);

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = absX;
    input.mi.dy = absY;
    SendInput(1, &input, sizeof(INPUT));
}

// 2. CLICK CHUỘT (Trái/Phải/Giữa - Nhấn/Nhả)
void InputManager::mouse_btn(const std::string& btn, const std::string& action) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    
    if (btn == "left") {
        input.mi.dwFlags = (action == "down") ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    } else if (btn == "right") {
        input.mi.dwFlags = (action == "down") ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    } else if (btn == "middle") {
        input.mi.dwFlags = (action == "down") ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    }
    SendInput(1, &input, sizeof(INPUT));
}

// 3. LĂN CHUỘT
void InputManager::mouse_scroll(int delta) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = (DWORD)delta;
    SendInput(1, &input, sizeof(INPUT));
}

// 4. BÀN PHÍM (Hỗ trợ giữ phím, tổ hợp phím)
void InputManager::key_event(int vk_code, bool is_down) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = (WORD)vk_code;
    
    if (!is_down) {
        input.ki.dwFlags = KEYEVENTF_KEYUP;
    }
    SendInput(1, &input, sizeof(INPUT));
}