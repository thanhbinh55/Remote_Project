#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ProcessManager : public IRemoteModule {
public:
    const std::string& get_module_name() const override;
    json handle_command(const json& request) override;

private:
    json list_processes();
    bool kill_process(int pid);
    
    // [MỚI] Hàm khởi tạo tiến trình trả về PID
    bool start_process(const std::string& path, int& out_pid);
};