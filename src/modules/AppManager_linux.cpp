#include "AppManager.hpp"

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

json AppManager::list_apps() {
    json app_list = json::array();

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return {
            {"status", "error"},
            {"module", get_module_name()},
            {"command", "LIST"},
            {"error", "Could not open /proc directory"}
        };
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        
        if (entry->d_type != DT_DIR) {
            continue;
        }

        const char* d_name = entry->d_name;
        bool is_pid = true;
        if (*d_name == '\0') is_pid = false; // Tên rỗng

        while (*d_name) {
            if (!std::isdigit(*d_name)) {
                is_pid = false;
                break;
            }
            d_name++;
        }

        if (is_pid) {
            std::string pid_str(entry->d_name);
            unsigned long pid = 0;
            unsigned long threads = 0;
            std::string name;

            try {
                pid = std::stoul(pid_str);
            } catch (...) {
                continue; // Bỏ qua nếu không phải số hợp lệ
            }

            std::ifstream comm_file("/proc/" + pid_str + "/comm");
            if (comm_file.is_open()) {
                std::getline(comm_file, name);
            } else {
                continue; 
            }

            std::ifstream status_file("/proc/" + pid_str + "/status");
            std::string line;
            if (status_file.is_open()) {
                while (std::getline(status_file, line)) {
                    // Tìm dòng bắt đầu bằng "Threads:"
                    if (line.rfind("Threads:", 0) == 0) {
                        std::stringstream ss(line);
                        std::string key; // Đọc chữ "Threads:"
                        ss >> key >> threads; // Đọc số lượng luồng
                        break;
                    }
                }
            }

            app_list.push_back({
                {"pid",     pid},
                {"name",    name} 
                // {"threads", threads}
            });
        }
    }

    closedir(proc_dir);

    return {
        {"status", "success"},
        {"module", get_module_name()},
        {"command", "LIST"},
        {"data", app_list}
    }; 
}

json AppManager::start_app(const std::string& app_name) { // Y như tạo một tiến  trình
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        return {
            {"status", "error"}, 
            {"module", get_module_name()}, 
            {"action", "START"},
            {"error", "Failed to create pipe"} 
        };
    }

    pid_t pid = fork();

    if (pid == -1) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return {
            {"status", "error"}, 
            {"module", get_module_name()}, 
            {"action", "START"},
            {"error", "Fork failed"} 
        };
    }

    if (pid == 0) {
        close(pipe_fd[0]); // Con không đọc, đóng đầu đọc
        dup2(pipe_fd[1], STDOUT_FILENO); // Chuyển hướng stdout
        close(pipe_fd[1]); // Đóng fd gốc

        std::string command = app_name + " & echo $!";
        
        execl("/bin/sh", "sh", "-c", command.c_str(), (char*) NULL);
        
        _exit(errno); 
    }

    close(pipe_fd[1]); // Cha không viết, đóng đầu viết

    int status;
    waitpid(pid, &status, 0); 

    std::array<char, 32> buffer;
    std::string real_pid_str;
    ssize_t bytes_read = read(pipe_fd[0], buffer.data(), buffer.size() - 1);
    
    close(pipe_fd[0]); // Đọc xong, đóng đầu đọc

    unsigned long real_pid = 0;

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Đảm bảo kết thúc chuỗi
        real_pid_str = buffer.data();
        
        real_pid_str.erase(
            std::remove(real_pid_str.begin(), real_pid_str.end(), '\n'), 
            real_pid_str.end()
        );

        try {
            real_pid = std::stoul(real_pid_str);
        } catch (...) {
             return {
                {"status", "error"}, 
                {"module", get_module_name()}, 
                {"action", "START"},
                {"error", "Failed to parse PID from child"}
            };
        }
    } else {
         return {
            {"status", "error"}, 
            {"module", get_module_name()}, 
            {"action", "START"},
            {"error", "Child process returned no PID"}
        };
    }

    return {
        {"status", "success"},
        {"module", get_module_name()},
        {"action", "START"},
        {"data", app_name}
    };

}

// json AppManager::kill_app_by_name(const std::string& app_name) {
// // Ngăn chặn các cuộc tấn công "command injection" cơ bản
//     // Chỉ cho phép tên ứng dụng an toàn (chữ, số, dấu gạch ngang)
//     std::string search_term = to_lower(app_name);
//     if (search_term.empty() || search_term.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-") != std::string::npos) {
//         return {
//             {"status", "error"},
//             {"module", get_module_name()},
//             {"action", "KILL"},
//             {"error", "Invalid application name"}
//         };
//     }
//
//     // Xây dựng lệnh: pkill -9 ten_ung_dung
//     // -9 là SIGKILL, tương đương với kill_pid_impl của chúng ta
//     std::string command = "pkill -9 " + app_name;
//
//     // Gọi lệnh hệ thống
//     int result = system(command.c_str());
//
//     if (result == 0) {
//         // pkill tìm thấy và đã kill thành công
//         return {
//             {"status", "success"},
//             {"module", get_module_name()},
//             {"action", "KILL"},
//             {"data", app_name}
//         };
//     }
//     // pkill thất bại (thường là do không tìm thấy tiến trình)
//     return {
//         {"status", "error"},
//         {"module", get_module_name()},
//         {"message", "Application not found or pkill failed"},
//         {"data", app_name}
//     };
// } 

json AppManager::kill_app_by_name(const std::string& keyword) {
    int killed = 0;
    std::vector<std::string> killed_details;
    
    std::string search_term = to_lower(keyword);
    if (search_term.empty()) {
         return {
            {"status", "error"},
            {"module", get_module_name()},
            {"command", "KILL"},
            {"error", "Keyword cannot be empty"}
        };
    }

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return {
            {"status", "error"}, 
            {"module", get_module_name()},
            {"command", "KILL"},
            {"error", "Cannot open /proc directory"}
        };
    }

    pid_t my_pid = getpid();

    struct dirent* entry;
    
    while ((entry = readdir(proc_dir)) != NULL) {
        
        if (entry->d_type != DT_DIR) continue;
        if (!std::isdigit(entry->d_name[0])) continue;

        std::string pid_str = entry->d_name;
        int pid = std::stoi(pid_str);

        if (pid == my_pid) continue;

        std::string comm_path = "/proc/" + pid_str + "/comm";
        std::ifstream cmd_file(comm_path);
        std::string exe_name;

        if (cmd_file.is_open()) {
            std::getline(cmd_file, exe_name);
            
            if (!exe_name.empty() && exe_name.back() == '\n') {
                exe_name.pop_back();
            }

            std::string exe_lower = to_lower(exe_name);
            
            if (exe_lower.find(search_term) != std::string::npos) {
                
                if (kill(pid, SIGKILL) == 0) {
                    killed++;
                    killed_details.push_back(exe_name + " (" + pid_str + ")");
                } else {
                }
            }
        }
        cmd_file.close();
    }

    closedir(proc_dir);

    return {
        {"status", "success"},
        {"module", get_module_name()},
        {"action", "KILL"},
        {"data", {
            {"keyword", keyword},
            {"killed_count", killed},
            {"details", killed_details}
        }}
    };
}

json AppManager::handle_command(const json& request) {
    const std::string command = request.value("command", "");
    json payload;
    if (request.contains("payload")) payload = request["payload"];

    if (command == "LIST") return list_apps();
    
    if (command == "KILL") {
        std::string name = payload.value("name", "");
        if (name.empty()) return {
            {"status", "error"}, 
            {"module", get_module_name()},
            {"error", "Missing name"}
        };
        return kill_app_by_name(name);
    }

    if (command == "START") {
        std::string path = payload.value("path", "");
        if (path.empty()) return {
            {"status", "error"}, 
            {"module", get_module_name()},
            {"error", "Missing path"}};
        return start_app(path);
    }

    return {
        {"status", "error"}, 
        {"module", get_module_name()},
        {"error", "Unknown APP command"}};
}
