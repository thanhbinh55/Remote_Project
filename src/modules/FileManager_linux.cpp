#include "FileManager.hpp"
#include <iostream>
#include <fstream>

// Hàm xử lý lệnh từ Client (bắt buộc do kế thừa IRemoteModule)
json FileManager::handle_command(const json& request) {
    return {
        {"status", "error"}, 
        {"message", "[Linux] FileManager chưa được hỗ trợ đầy đủ."}
    };
}

// Hàm đọc file nhị phân (Main đang gọi hàm này)
// Tôi viết tạm logic đọc file cơ bản để bạn dùng được luôn nếu cần
bool FileManager::read_file_binary(const std::string& filepath, std::vector<uint8_t>& out_buffer) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Lấy kích thước file
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Đọc dữ liệu
    out_buffer.resize(fileSize);
    file.read(reinterpret_cast<char*>(out_buffer.data()), fileSize);
    
    return true;
}