#include <fxl/backend_win32.hpp>
#include <iostream>

using namespace fxl::runtime::backend;

int main() {
    std::cerr << "[Test] Creating Win32Window\n";
    Win32Window window;
    
    WindowParams params;
    params.title = "Test Window";
    params.width = 800;
    params.height = 600;
    params.x = 100;
    params.y = 100;
    params.visible = true;
    params.resizable = true;
    
    WindowCallbacks callbacks;
    callbacks.on_create = []() {
        std::cerr << "[Test] on_create callback invoked\n";
    };
    callbacks.on_destroy = []() {
        std::cerr << "[Test] on_destroy callback invoked\n";
    };
    callbacks.on_resize = [](int w, int h) {
        std::cerr << "[Test] on_resize: " << w << "x" << h << "\n";
    };
    callbacks.on_paint = []() {
        std::cerr << "[Test] on_paint callback invoked\n";
    };
    callbacks.on_message = [](UINT, WPARAM, LPARAM, LRESULT&) -> bool {
        return false;
    };
    callbacks.on_mouse_move = [](int x, int y, UINT) {
        // std::cerr << "[Test] mouse move: " << x << "," << y << "\n";
    };
    callbacks.on_mouse_button = [](int, int, UINT, bool) {};
    callbacks.on_key = [](WPARAM, bool) {};
    
    std::cerr << "[Test] Setting callbacks\n";
    window.set_callbacks(callbacks);
    
    std::cerr << "[Test] Creating window\n";
    if (!window.create(params, callbacks)) {
        std::cerr << "[Test] Failed to create window\n";
        return 1;
    }
    
    std::cerr << "[Test] Window created successfully, running message loop\n";
    return window.run_message_loop();
}