#include "modules/EdgeManager.hpp"

json EdgeManager::handle_command(const json& request) {
    // Trả về lỗi hoặc thông báo chưa hỗ trợ trên Linux
    return {
        {"status", "error"},
        {"message", "EdgeManager logic is not implemented on Linux yet."}
    };
}