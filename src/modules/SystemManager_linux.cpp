#include "SystemManager.hpp"

void wrapper_shutdown() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    int t = system("nohup systemctl poweroff");
}

void wrapper_restart() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    int t = system("nohup systemctl reboot");
}

json SystemManager::shutdown_system() const {
    int can_shutdown = system("systemctl --dry-run poweroff > /dev/null 2>&1");
    json res = {
        {"status", "success"},
        {"module", get_module_name()},
        {"command", "SHUTDOWN"}
    };

    if (can_shutdown != 0) { // can't shutdown
        res["status"] = "error";
        res["error"] = "Can't shutdown";

        return res;
    }

    res["data"] = "Shutdown successfully";

    std::thread t(wrapper_shutdown);
    t.detach();

    return res;
}

json SystemManager::restart_system() const {
    int can_restart = system("systemctl --dry-run reboot > /dev/null 2>&1");
    json res = {
        {"status", "success"},
        {"module", get_module_name()},
        {"command", "RESTART"}
    };

    if (can_restart != 0) { // can't shutdown
        res["status"] = "error";
        res["error"] = "Can't restart";

        return res;
    }

    res["data"] = "Restart successfully";

    std::thread t(wrapper_restart);
    t.detach();

    return res;

}

json SystemManager::handle_command(const json& request) {
    const std::string command = request.value("command", "");

    if (command == "SHUTDOWN") {
        return shutdown_system();
    }

    else if (command == "RESTART")
        return restart_system();
    
    return {
        {"status", "error"},
        {"module", get_module_name()},
        {"error", "Unknown system command"}
    };
}
