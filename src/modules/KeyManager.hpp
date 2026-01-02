// KeyManager.hpp
#pragma once
#include <string>
#include <functional>
#include <thread>
#include <nlohmann/json.hpp>
#include "../interfaces/IRemoteModule.hpp"

#if _WIN32
#include <windows.h>
#else
#include <map>
#include <fstream>
#include <atomic>
#include <fcntl.h>      // open
#include <unistd.h>     // close, read
#include <linux/input.h>// struct input_event, KEY_*
#include <dirent.h>     // scandir
#include <cstring>
#endif

using json = nlohmann::json;

// Định nghĩa kiểu hàm callback: nhận vào ký tự phím (string)
using KeyCallback = std::function<void(std::string key_char)>;

class KeyManager : public IRemoteModule {
public:
    // --- Bắt buộc implement từ IRemoteModule ---
    const std::string& get_module_name() const override;
    json handle_command(const json& request) override;

    // --- Các hàm quản lý Hook ---
    // Callback cần là static vì Windows Hook là hàm toàn cục (Global C-style function)
    static void set_callback(KeyCallback cb);
    
    void start_hook();
    void stop_hook();
    
    // Hàm khóa phím (static để gọi được từ trong Hook Procedure nếu cần)
    static void set_locked(bool locked);

private:
    std::thread hookThread;
};