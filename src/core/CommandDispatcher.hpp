// #pragma once
// #include "../interfaces/IRemoteModule.hpp"
// #include <unordered_map>
// #include <memory>
// #include <string>
// #include <iostream>
// #include "../modules/ProcessManager.hpp"
// #include "../modules/SystemManager.hpp"
// #include "../modules/ScreenManager.hpp"
// #include "../modules/AppManager.hpp"
// #include "../modules/WebcamManager.hpp"
// #include "../modules/FileManager.hpp"
// #include "../modules/InputManager.hpp

// class CommandDispatcher {
// private:
//     std::unordered_map<std::string, std::unique_ptr<IRemoteModule>> modules_;

// public: 
//     CommandDispatcher() {
//         register_module(std::make_unique<ProcessManager>());
//         register_module(std::make_unique<SystemManager>());
//         register_module(std::make_unique<ScreenManager>());
//         register_module(std::make_unique<AppManager>());
//         register_module(std::make_unique<WebcamManager>());
//         register_module(std::make_unique<FileManager>());
//         register_module(std::make_unique<InputManager>())
//     }

//     void register_module(std::unique_ptr<IRemoteModule> module) {
//         if (!module) return;
//         const std::string name = module->get_module_name();
//         auto [it, inserted] = modules_.emplace(name, std::move(module));
//         if (inserted) std::cout << "Module registered: " << name << "\n";
//     }

//     // --- MỚI: Hàm lấy Module pointer để Main ép kiểu ---
//     IRemoteModule* get_module(const std::string& name) {
//         auto it = modules_.find(name);
//         if (it != modules_.end()) return it->second.get();
//         return nullptr;
//     }

//     json dispatch(const json& request) {
//         if (!request.contains("module")) return { {"status","error"},{"message","Missing module"} };
        
//         const std::string module_name = request["module"].get<std::string>();
//         auto it = modules_.find(module_name);
//         if (it == modules_.end()) return { {"status","error"},{"message","Unknown module"} };
        
//         return it->second->handle_command(request);
//     }
// };

#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

// Include các module
#include "../modules/ProcessManager.hpp"
#include "../modules/SystemManager.hpp"
#include "../modules/ScreenManager.hpp"
#include "../modules/AppManager.hpp"
#include "../modules/KeyManager.hpp"
#include "../modules/WebcamManager.hpp"
#include "../modules/FileManager.hpp"
#include "../modules/InputManager.hpp" // Nếu bạn đã làm phần Input Control
#include "../modules/EdgeManager.hpp"

using json = nlohmann::json;

class CommandDispatcher {
private:
    std::unordered_map<std::string, std::unique_ptr<IRemoteModule>> modules_;

public: 
    CommandDispatcher() {
        // Đăng ký các module
        register_module(std::make_unique<ProcessManager>());
        register_module(std::make_unique<SystemManager>());
        register_module(std::make_unique<ScreenManager>());
        register_module(std::make_unique<AppManager>());
        register_module(std::make_unique<KeyManager>());
        register_module(std::make_unique<WebcamManager>());
        register_module(std::make_unique<FileManager>());
        register_module(std::make_unique<InputManager>()); // Nếu chưa có file InputManager thì comment dòng này lại
        register_module(std::make_unique<EdgeManager>());
    }

    void register_module(std::unique_ptr<IRemoteModule> module) {
        if (!module) return;
        const std::string name = module->get_module_name();
        // Sửa lỗi C2001: Đảm bảo chuỗi không bị xuống dòng
        std::cout << "[INIT] Module registered: " << name << "\n";
        modules_.emplace(name, std::move(module));
    }

    // Hàm lấy module pointer (Để main gọi trực tiếp các hàm đặc biệt như Start Stream)
    IRemoteModule* get_module(const std::string& name) {
        auto it = modules_.find(name);
        if (it != modules_.end()) return it->second.get();
        return nullptr;
    }

    json dispatch(const json& request) {
        if (!request.contains("module")) return { {"status","error"},{"message","Missing module"} };
        
        // Sửa lỗi lấy string an toàn
        std::string module_name = request.value("module", "");
        
        auto it = modules_.find(module_name);
        if (it == modules_.end()) return { {"status","error"},{"message","Unknown module"} };
        
        return it->second->handle_command(request);
    }
};