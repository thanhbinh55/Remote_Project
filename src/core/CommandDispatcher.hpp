#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../interfaces/IRemoteModule.hpp"

// Include tất cả các module implementation
#include "../modules/ProcessManager.hpp"
#include "../modules/SystemManager.hpp"
#include "../modules/ScreenManager.hpp"
#include "../modules/AppManager.hpp"
#include "../modules/WebcamManager.hpp"
#include "../modules/KeyManager.hpp"
#include "../modules/FileManager.hpp"
#include "../modules/InputManager.hpp"
#include "../modules/EdgeManager.hpp" // <-- Đã thêm module này

using json = nlohmann::json;

class CommandDispatcher {
private:
    std::unordered_map<std::string, std::unique_ptr<IRemoteModule>> modules_;

public: 
    CommandDispatcher() {
        // Tự động đăng ký khi khởi tạo class
        register_module(std::make_unique<ProcessManager>());
        register_module(std::make_unique<SystemManager>());
        register_module(std::make_unique<ScreenManager>());
        register_module(std::make_unique<AppManager>());
        register_module(std::make_unique<WebcamManager>());
        register_module(std::make_unique<KeyManager>());
        register_module(std::make_unique<FileManager>());
        register_module(std::make_unique<InputManager>());
        register_module(std::make_unique<EdgeManager>()); 
    }

    void register_module(std::unique_ptr<IRemoteModule> module) {
        if (!module) return;
        const std::string name = module->get_module_name();
        // Sử dụng emplace để đưa vào map
        auto [it, inserted] = modules_.emplace(name, std::move(module));
        if (inserted) std::cout << "[DISPATCHER] Module registered: " << name << "\n";
    }

    // Hàm lấy Module pointer (quan trọng cho WebSocketServer dùng dynamic_cast)
    IRemoteModule* get_module(const std::string& name) {
        auto it = modules_.find(name);
        if (it != modules_.end()) return it->second.get();
        return nullptr;
    }

    json dispatch(const json& request) {
        if (!request.contains("module")) return { {"status","error"},{"message","Missing module"} };
        
        const std::string module_name = request["module"].get<std::string>();
        auto it = modules_.find(module_name);
        if (it == modules_.end()) return { {"status","error"},{"message","Unknown module"} };
        
        return it->second->handle_command(request);
    }
};