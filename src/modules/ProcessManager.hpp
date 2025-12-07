#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>

#if _WIN32
#include <tlhelp32.h>
#include <windows.h>
#else
#include <signal.h>
#include <wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <dirent.h>
#endif

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
