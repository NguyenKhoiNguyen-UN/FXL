#pragma once

// Win32 backend for FXL native window
// Provides minimal window creation, message loop, and OpenGL context

#include <windows.h>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <optional>
#include <iostream>

namespace fxl {
namespace runtime {
namespace backend {

// Forward declarations
class Win32Window;
class Win32OpenGLContext;

// Window creation parameters
struct WindowParams {
    std::string title = "FXL Window";
    int width = 800;
    int height = 600;
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    bool visible = true;
    bool resizable = true;
};

// Window event callbacks
struct WindowCallbacks {
    std::function<void()> on_create;
    std::function<void()> on_destroy;
    std::function<void(int width, int height)> on_resize;
    std::function<void()> on_paint;
    std::function<bool(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)> on_message;
    std::function<void(int x, int y, UINT flags)> on_mouse_move;
    std::function<void(int x, int y, UINT button, bool down)> on_mouse_button;
    std::function<void(WPARAM key, bool down)> on_key;
};

// Win32 Window wrapper
class Win32Window {
public:
    Win32Window();
    ~Win32Window();

    // Non-copyable, movable
    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;
    Win32Window(Win32Window&& other) noexcept {
        std::cerr << "[Win32] Win32Window move constructor\n";
        hwnd_ = other.hwnd_;
        hinstance_ = other.hinstance_;
        callbacks_ = std::move(other.callbacks_);
        width_ = other.width_;
        height_ = other.height_;
        created_ = other.created_;
        other.hwnd_ = nullptr;
        other.hinstance_ = nullptr;
        other.created_ = false;
    }
    Win32Window& operator=(Win32Window&& other) noexcept {
        std::cerr << "[Win32] Win32Window move assignment\n";
        if (this != &other) {
            hwnd_ = other.hwnd_;
            hinstance_ = other.hinstance_;
            callbacks_ = std::move(other.callbacks_);
            width_ = other.width_;
            height_ = other.height_;
            created_ = other.created_;
            other.hwnd_ = nullptr;
            other.hinstance_ = nullptr;
            other.created_ = false;
        }
        return *this;
    }

    // Create window
    bool create(const WindowParams& params, const WindowCallbacks& callbacks = {});

    // Show/hide
    void show(bool show = true);

    // Message loop
    int run_message_loop();
    void post_quit(int exit_code = 0);

    // Getters
    HWND hwnd() const { return hwnd_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool is_valid() const { return hwnd_ != nullptr; }

    // Set callbacks (can be called after create)
    void set_callbacks(const WindowCallbacks& callbacks);

private:
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    WindowCallbacks callbacks_;
    int width_ = 800;
    int height_ = 600;
    bool created_ = false;

    static LRESULT CALLBACK wnd_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT wnd_proc(UINT msg, WPARAM wParam, LPARAM lParam);

    void register_window_class();
};

// OpenGL Context wrapper
class Win32OpenGLContext {
public:
    Win32OpenGLContext() = default;
    ~Win32OpenGLContext();

    Win32OpenGLContext(const Win32OpenGLContext&) = delete;
    Win32OpenGLContext& operator=(const Win32OpenGLContext&) = delete;
    Win32OpenGLContext(Win32OpenGLContext&& other) noexcept = default;
    Win32OpenGLContext& operator=(Win32OpenGLContext&& other) noexcept = default;

    // Create OpenGL context for window
    bool create(HWND hwnd);

    // Make context current
    bool make_current();

    // Swap buffers
    void swap_buffers();

    // Getters
    HGLRC hglrc() const { return hglrc_; }
    HDC hdc() const { return hdc_; }
    bool is_valid() const { return hglrc_ != nullptr; }

    // Viewport
    void set_viewport(int width, int height);

private:
    HGLRC hglrc_ = nullptr;
    HDC hdc_ = nullptr;
    HWND hwnd_ = nullptr;
    int pixel_format_ = 0;
};

} // namespace backend
} // namespace runtime
} // namespace fxl