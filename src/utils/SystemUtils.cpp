#include "SystemUtils.hpp"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#endif

#include <boost/asio.hpp>

std::string SystemUtils::get_computer_name() {
#ifdef _WIN32
    char buf[256];
    DWORD size = sizeof(buf);
    if (GetComputerNameA(buf, &size)) return std::string(buf);
    return "UNKNOWN-WIN-PC";
#else
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX) == 0) return std::string(hostname);
    return "UNKNOWN-LINUX-PC";
#endif
}

std::string SystemUtils::get_local_ip() {
    try {
        boost::asio::io_context io_context;
        boost::asio::ip::udp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(boost::asio::ip::udp::v4(), "8.8.8.8", "80");
        if (endpoints.begin() != endpoints.end()) {
            boost::asio::ip::udp::socket socket(io_context);
            socket.connect(*endpoints.begin());
            return socket.local_endpoint().address().to_string();
        }
    } catch (...) {}
    return "127.0.0.1";
}

std::string SystemUtils::get_os_name() {
#ifdef _WIN32
    return "Windows";
#elif __linux__
    return "Linux";
#else
    return "Unknown OS";
#endif
}

void SystemUtils::setup_console() {
#ifdef _WIN32
    SetProcessDPIAware();
    SetConsoleOutputCP(CP_UTF8);
#endif
}