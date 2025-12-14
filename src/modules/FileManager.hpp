#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class FileManager : public IRemoteModule {
public:
    const std::string& get_module_name() const override {
        static const std::string name = "FILE";
        return name;
    }

    json handle_command(const json& request) override;

    // Hàm hỗ trợ đọc file nhị phân (Static để Main gọi dễ dàng)
    static bool read_file_binary(const std::string& filename, std::vector<uint8_t>& out_data);
};