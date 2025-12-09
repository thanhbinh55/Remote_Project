#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <windows.h>

class InputManager : public IRemoteModule {
public:
    const std::string& get_module_name() const override {
        static const std::string name = "INPUT";
        return name;
    }

    json handle_command(const json& request) override;

private:
    void move_mouse(double x, double y); // Nhận toạ độ tỉ lệ 0.0 -> 1.0
    void mouse_btn(const std::string& btn, const std::string& action); // left/right, down/up
    void mouse_scroll(int delta);
    void key_event(int key_code, bool is_down);
};