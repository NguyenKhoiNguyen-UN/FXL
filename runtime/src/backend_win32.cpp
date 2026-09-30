// Win32 backend implementation
#include <fxl/backend_win32.hpp>
#include <iostream>
#include <stdexcept>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM

// OpenGL functions
#if defined(_WIN32) || defined(_WIN64)
#include <GL/gl.h>
#endif

namespace fxl {
namespace runtime {
namespace backend {

Win32Window::Win32Window() = default;

Win32Window::~Win32Window() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (hinstance_) {
        UnregisterClassW(L"FXLWindowClass", hinstance_);
        hinstance_ = nullptr;
    }
}

void Win32Window::register_window_class() {
    hinstance_ = GetModuleHandleW(nullptr);
    
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = wnd_proc_static;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hinstance_;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"FXLWindowClass";
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wcex)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            throw std::runtime_error("Failed to register window class: " + std::to_string(err));
        }
    }
}

bool Win32Window::create(const WindowParams& params, const WindowCallbacks& callbacks) {
    // Only update callbacks_ if new callbacks are provided (not all empty)
    bool has_new_callbacks = callbacks.on_create || callbacks.on_destroy || callbacks.on_resize || 
                             callbacks.on_paint || callbacks.on_message || callbacks.on_mouse_move ||
                             callbacks.on_mouse_button || callbacks.on_key;
    if (has_new_callbacks) {
        callbacks_ = callbacks;
    }
    width_ = params.width;
    height_ = params.height;
    
    register_window_class();
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!params.resizable) {
        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }
    
    DWORD exstyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    
    // Convert title to wide string
    int title_len = MultiByteToWideChar(CP_UTF8, 0, params.title.c_str(), -1, nullptr, 0);
    std::wstring wtitle(static_cast<size_t>(title_len), 0);
    MultiByteToWideChar(CP_UTF8, 0, params.title.c_str(), -1, wtitle.data(), title_len);
    
    // Create window
    hwnd_ = CreateWindowExW(
        exstyle,
        L"FXLWindowClass",
        wtitle.c_str(),
        style,
        params.x, params.y,
        params.width, params.height,
        nullptr, nullptr, hinstance_, this
    );
    
    if (!hwnd_) {
        DWORD err = GetLastError();
        std::cerr << "[Win32] CreateWindowExW failed: err=" << err << std::endl;
        return false;
    }
    
    // Check window rect
    RECT rect;
    if (GetWindowRect(hwnd_, &rect)) {
        // Window rect logged for debugging
    }
    
    // Store this pointer
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    created_ = true;
    
    if (params.visible) {
        show(true);
    }
    
    if (callbacks_.on_create) {
        callbacks_.on_create();
    }
    
    return true;
}

void Win32Window::show(bool show) {
    if (hwnd_) {
        ShowWindow(hwnd_, show ? SW_SHOW : SW_HIDE);
        if (show) UpdateWindow(hwnd_);
    }
}

void Win32Window::set_callbacks(const WindowCallbacks& callbacks) {
    callbacks_ = callbacks;
}

int Win32Window::run_message_loop() {
    MSG msg{};
    std::cout << "[Win32] Starting message loop\n";
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    std::cout << "[Win32] Message loop exited with code " << msg.wParam << "\n";
    return static_cast<int>(msg.wParam);
}

void Win32Window::post_quit(int exit_code) {
    PostQuitMessage(exit_code);
}

LRESULT CALLBACK Win32Window::wnd_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto create_struct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto window = static_cast<Win32Window*>(create_struct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    auto window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (window) {
        return window->wnd_proc(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Win32Window::wnd_proc(UINT msg, WPARAM wParam, LPARAM lParam) {
    // Allow custom message handling first
    if (callbacks_.on_message) {
        LRESULT result = 0;
        if (callbacks_.on_message(msg, wParam, lParam, result)) {
            return result;
        }
    }
    
    switch (msg) {
        case WM_DESTROY:
            if (callbacks_.on_destroy) callbacks_.on_destroy();
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE: {
            width_ = LOWORD(lParam);
            height_ = HIWORD(lParam);
            if (callbacks_.on_resize) callbacks_.on_resize(width_, height_);
            break;
        }
            
        case WM_PAINT: {
            if (callbacks_.on_paint) callbacks_.on_paint();
            PAINTSTRUCT ps;
            BeginPaint(hwnd_, &ps);
            EndPaint(hwnd_, &ps);
            return 0;
        }
            
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            if (callbacks_.on_mouse_move) callbacks_.on_mouse_move(x, y, static_cast<UINT>(wParam));
            break;
        }
            
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            UINT button = 0;
            bool down = false;
            switch (msg) {
                case WM_LBUTTONDOWN: button = 1; down = true; break;
                case WM_LBUTTONUP: button = 1; down = false; break;
                case WM_RBUTTONDOWN: button = 2; down = true; break;
                case WM_RBUTTONUP: button = 2; down = false; break;
                case WM_MBUTTONDOWN: button = 3; down = true; break;
                case WM_MBUTTONUP: button = 3; down = false; break;
            }
            if (callbacks_.on_mouse_button) callbacks_.on_mouse_button(x, y, button, down);
            break;
        }
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            bool down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            if (callbacks_.on_key) callbacks_.on_key(wParam, down);
            break;
        }
            
        case WM_CLOSE:
            DestroyWindow(hwnd_);
            return 0;
    }
    
    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

// Win32OpenGLContext implementation
Win32OpenGLContext::~Win32OpenGLContext() {
    if (hglrc_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(hglrc_);
        hglrc_ = nullptr;
    }
    if (hdc_) {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
}

bool Win32OpenGLContext::create(HWND hwnd) {
    hwnd_ = hwnd;
    hdc_ = GetDC(hwnd_);
    if (!hdc_) {
        return false;
    }
    
    // Pixel format descriptor
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    pixel_format_ = ChoosePixelFormat(hdc_, &pfd);
    if (!pixel_format_) {
        return false;
    }
    
    if (!SetPixelFormat(hdc_, pixel_format_, &pfd)) {
        return false;
    }
    
    hglrc_ = wglCreateContext(hdc_);
    if (!hglrc_) {
        return false;
    }
    
    if (!wglMakeCurrent(hdc_, hglrc_)) {
        return false;
    }
    
    return true;
}

bool Win32OpenGLContext::make_current() {
    return wglMakeCurrent(hdc_, hglrc_) != FALSE;
}

void Win32OpenGLContext::swap_buffers() {
    SwapBuffers(hdc_);
}

void Win32OpenGLContext::set_viewport(int width, int height) {
    glViewport(0, 0, width, height);
}

} // namespace backend
} // namespace runtime
} // namespace fxl