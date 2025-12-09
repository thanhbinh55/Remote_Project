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
using KeyCallback = std::function<void(std::string key_char)>;

class KeyManager : public IRemoteModule {
public:
    static void set_callback(KeyCallback cb);
    void start_hook();
    void stop_hook();
    
    // [MỚI] Hàm set trạng thái khóa
    static void set_locked(bool locked);

    const std::string& get_module_name() const override { 
        static const std::string name = "KEYBOARD"; 
        return name; 
    }

    json handle_command(const json& request) override;

private:
    std::thread hookThread;
};
