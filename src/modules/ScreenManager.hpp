#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <vector>
#include <string>

class ScreenManager : public IRemoteModule {
public:
    const std::string& get_module_name() const override { 
        static const std::string name = "SCREEN"; return name; 
    }
    
    json handle_command(const json& request) override;

    // Hàm public để Main gọi trực tiếp
    static bool capture_screen_data(std::vector<uint8_t>& out_buffer, std::string& error_msg);
};