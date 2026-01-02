#pragma once
#include <string>

class SystemUtils {
public:
    static std::string get_computer_name();
    static std::string get_local_ip();
    static std::string get_os_name();
    static void setup_console(); // Cấu hình DPI và UTF8
};