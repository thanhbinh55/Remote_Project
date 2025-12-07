#include "modules/AppManager.hpp"
#include "modules/ProcessManager.hpp"
#include "modules/SystemManager.hpp"
#include "modules/KeyManager.hpp"
#include "modules/ScreenManager.hpp"
#include "modules/WebcamManager.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <fstream>

using namespace std;

ofstream out("OUTPUT.txt");

void output_key_pressed(std::string s) {
    out << s;
}

int main() {
    // KeyManager km;
    // km.set_callback(output_key_pressed);
    //
    // json request = {{"command", "START"}};
    // km.handle_command(request);
    // std::cout << "START\n";
    //
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    // std::cout << "\n";
    //
    // km.handle_command(request);
    // request["command"] = "STOP";

    WebcamManager webcam;

    std::cout << "Starting Webcam...\n";

    // Start stream và lưu thử 1 frame vào file để kiểm tra
    int count = 0;
    webcam.start_stream([&count](const std::vector<uint8_t>& data) {
        // Đây là nơi bạn gửi data qua WebSocket
        // server.send(data);
        
        // Test: Lưu frame đầu tiên ra file
        if (count <= 10) {
            std::string file_name = "test_capture" + to_string(count) + ".jpg";
            std::ofstream file(file_name, std::ios::binary);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            std::cout << "Captured frame size: " << data.size() << " bytes\n";
            count++;
        }
    });

    // Giữ chương trình chạy 10s
    std::this_thread::sleep_for(std::chrono::seconds(10));

    webcam.stop_stream();
    return 0;
}

