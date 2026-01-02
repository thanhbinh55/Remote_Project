#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

class RegistryClient {
public:
    RegistryClient();
    ~RegistryClient();

    // Hàm duy nhất Main cần gọi: Bắt đầu luồng tự động tìm và giữ kết nối
    void start_monitoring(int ws_port);
    
    // Dừng luồng (khi tắt app)
    void stop_monitoring();

private:
    std::string registry_host_;
    int ws_port_ = 9010;
    
    std::atomic<bool> is_running_{false};      // Cờ chạy luồng
    std::atomic<bool> is_connected_{false};    // Trạng thái kết nối
    std::thread monitor_thread_;

    // Các hàm nội bộ
    bool attempt_discovery();    // Tìm IP (UDP)
    bool attempt_register();     // Đăng ký (HTTP)
    bool send_heartbeat();       // Gửi tim (HTTP) - Trả về false nếu lỗi
};