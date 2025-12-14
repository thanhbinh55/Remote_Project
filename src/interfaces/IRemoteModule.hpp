// src/interfaces/IRemoteModule.hpp
#pragma once
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class IRemoteModule {
public:
    virtual ~IRemoteModule() = default;
    
    // Phương thức xử lý lệnh: nhận JSON, xử lý, trả về JSON kết quả
    virtual json handle_command(const json& request) = 0;
    
    // Tên Module để Dispatcher phân loại (VD: "PROCESS")
    virtual const std::string& get_module_name() const = 0;
};