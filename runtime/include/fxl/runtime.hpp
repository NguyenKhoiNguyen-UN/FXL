#pragma once

// FXL Runtime - Minimal UI backend for generated C++ code
// This header-only library provides the runtime classes that generated code depends on

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <optional>

// Windows and OpenGL includes for native backend
#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// Include backend types (outside namespace to avoid nesting)
#include "fxl/backend_win32.hpp"

namespace fxl {
namespace runtime {

// Base class for all UI objects
class Object {
public:
    Object() = default;
    virtual ~Object() = default;

    // Called when object is added to parent
    virtual void on_added_to_parent(Object* parent) {
        parent_ = parent;
    }

    Object* parent() const { return parent_; }

    // Lifecycle
    virtual void initialize() {}
    virtual void shutdown() {}

    // Rendering (to be overridden by derived classes)
    virtual void render() {}

    // Common setters (for #autoSet compatibility)
    virtual void setColor(const std::string& color) {
        color_ = color;
        std::cout << "[Object] setColor(\"" << color << "\")\n";
    }
    virtual void setOpacity(double opacity) {
        opacity_ = opacity;
        std::cout << "[Object] setOpacity(" << opacity << ")\n";
    }
    virtual void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        std::cout << "[Object] setSize(" << width << ", " << height << ")\n";
    }
    virtual void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
        std::cout << "[Object] setPosition(" << x << ", " << y << ")\n";
    }
    virtual void setText(const std::string& text) {
        text_ = text;
        std::cout << "[Object] setText(\"" << text << "\")\n";
    }

protected:
    Object* parent_ = nullptr;
    std::string color_ = "#FFFFFF";
    double opacity_ = 1.0;
    int width_ = 100;
    int height_ = 100;
    int x_ = 0;
    int y_ = 0;
    std::string text_ = "";
};

// Window - top-level window
class Window : public Object {
public:
    Window() = default;
    ~Window() override = default;

    // Delete copy, allow move
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = default;
    Window& operator=(Window&&) = default;

    void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        if (native_window_) {
            // Note: Win32 doesn't easily support programmatic resize after creation
            // This would require SetWindowPos, but we track the size for renderer
            std::cout << "[Window] setSize(" << width << ", " << height << ")\n";
        } else {
            std::cout << "[Window] setSize(" << width << ", " << height << ")\n";
        }
    }

    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
        std::cout << "[Window] setPosition(" << x << ", " << y << ")\n";
    }

    void setTitle(const std::string& title) {
        title_ = title;
        if (native_window_) {
            int title_len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
            std::wstring wtitle(title_len, 0);
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wtitle.data(), title_len);
            SetWindowTextW(native_window_->hwnd(), wtitle.c_str());
        }
        std::cout << "[Window] setTitle(\"" << title << "\")\n";
    }

    void setColor(const std::string& color) {
        color_ = color;
        std::cout << "[Window] setColor(\"" << color << "\")\n";
    }

    void setOpacity(double opacity) {
        opacity_ = opacity;
        std::cout << "[Window] setOpacity(" << opacity << ")\n";
    }

    void add(Object& child) {
        children_.push_back(std::shared_ptr<Object>(&child, [](Object*){}));  // Non-owning
        child.on_added_to_parent(this);
        std::cout << "[Window] add(child)\n";
    }

    // Create native window (called lazily or explicitly)
    bool create_native() {
        if (native_window_) return true;
        
        std::cerr << "[Window] create_native: creating Win32Window\n";
        native_window_ = std::make_unique<fxl::runtime::backend::Win32Window>();
        
        fxl::runtime::backend::WindowParams params;
        params.title = title_;
        params.width = width_;
        params.height = height_;
        params.x = (x_ != 0) ? x_ : CW_USEDEFAULT;
        params.y = (y_ != 0) ? y_ : CW_USEDEFAULT;
        params.visible = false; // We'll show after OpenGL context is ready
        params.resizable = true;
        
        // Create callbacks object first to avoid lambda capture issues with designated initializers
        fxl::runtime::backend::WindowCallbacks callbacks;
        callbacks.on_create = [this]() {
            // Create OpenGL context
            gl_context_ = std::make_unique<fxl::runtime::backend::Win32OpenGLContext>();
            if (gl_context_->create(native_window_->hwnd())) {
                native_window_->show(true);
            } else {
                native_window_->post_quit(-1);
            }
        };
        callbacks.on_destroy = []() {
        };
        callbacks.on_resize = [this](int w, int h) {
            width_ = w;
            height_ = h;
            if (gl_context_) gl_context_->set_viewport(w, h);
        };
        callbacks.on_paint = [this]() {
            render_frame();
        };
        callbacks.on_message = [](UINT, WPARAM, LPARAM, LRESULT&) -> bool {
            return false;
        };
        callbacks.on_mouse_move = [this](int x, int y, UINT flags) {
            // Handle mouse move
        };
        callbacks.on_mouse_button = [this](int x, int y, UINT button, bool down) {
            // Handle mouse button
        };
        callbacks.on_key = [this](WPARAM key, bool down) {
            // Handle key events
        };
        
        native_window_->set_callbacks(std::move(callbacks));
        
        return native_window_->create(params);
    }

    int run() {
        if (!native_window_) {
            if (!create_native()) {
                return -1;
            }
        }
        
        initialize_children();
        
        // Run message loop
        return native_window_->run_message_loop();
    }

    void close() {
        if (native_window_) {
            native_window_->post_quit(0);
        }
    }

    // Render one frame
    void render_frame() {
        if (!gl_context_ || !native_window_) return;
        
        gl_context_->make_current();
        
        // Clear with window color
        float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
        if (color_.size() == 7 && color_[0] == '#') {
            r = std::stoi(color_.substr(1, 2), nullptr, 16) / 255.0f;
            g = std::stoi(color_.substr(3, 2), nullptr, 16) / 255.0f;
            b = std::stoi(color_.substr(5, 2), nullptr, 16) / 255.0f;
        }
        a = static_cast<float>(opacity_);
        
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render children
        render_children();
        
        gl_context_->swap_buffers();
    }

    void render_children() {
        for (auto& child : children_) {
            child->render();
        }
    }

private:
    void initialize_children() {
        for (auto& child : children_) {
            child->initialize();
        }
    }

    std::string title_ = "FXL Window";
    int width_ = 800;
    int height_ = 600;
    int x_ = 100;
    int y_ = 100;
    std::string color_ = "#FFFFFF";
    double opacity_ = 1.0;
    std::vector<std::shared_ptr<Object>> children_;
    
    // Native backend
    std::unique_ptr<fxl::runtime::backend::Win32Window> native_window_;
    std::unique_ptr<fxl::runtime::backend::Win32OpenGLContext> gl_context_;
};

// Button - clickable button
class Button : public Object {
public:
    Button() = default;
    ~Button() override = default;

    void setText(const std::string& text) {
        text_ = text;
        std::cout << "[Button] setText(\"" << text << "\")\n";
    }

    void setColor(const std::string& color) {
        color_ = color;
        std::cout << "[Button] setColor(\"" << color << "\")\n";
    }

    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
        std::cout << "[Button] setPosition(" << x << ", " << y << ")\n";
    }

    void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        std::cout << "[Button] setSize(" << width << ", " << height << ")\n";
    }

    void setOpacity(double opacity) {
        opacity_ = opacity;
        std::cout << "[Button] setOpacity(" << opacity << ")\n";
    }

    void on_click(std::function<void()> handler) {
        click_handler_ = std::move(handler);
    }

    void simulate_click() {
        if (click_handler_) click_handler_();
    }

private:
    std::string text_ = "Button";
    std::string color_ = "#CCCCCC";
    int x_ = 0;
    int y_ = 0;
    int width_ = 100;
    int height_ = 30;
    double opacity_ = 1.0;
    std::function<void()> click_handler_;
};

// Panel - container widget
class Panel : public Object {
public:
    Panel() = default;
    ~Panel() override = default;

    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
        std::cout << "[Panel] setPosition(" << x << ", " << y << ")\n";
    }

    void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        std::cout << "[Panel] setSize(" << width << ", " << height << ")\n";
    }

    void setColor(const std::string& color) {
        color_ = color;
        std::cout << "[Panel] setColor(\"" << color << "\")\n";
    }

    void setOpacity(double opacity) {
        opacity_ = opacity;
        std::cout << "[Panel] setOpacity(" << opacity << ")\n";
    }

    void add(Object& child) {
        children_.push_back(std::shared_ptr<Object>(&child, [](Object*){}));
        child.on_added_to_parent(this);
        std::cout << "[Panel] add(child)\n";
    }

private:
    int x_ = 0;
    int y_ = 0;
    int width_ = 200;
    int height_ = 200;
    std::string color_ = "#EEEEEE";
    double opacity_ = 1.0;
    std::vector<std::shared_ptr<Object>> children_;
};

// rectangle - simple shape (renamed from Rectangle to avoid Windows.h conflict)
class rectangle : public Object {
public:
    rectangle() = default;
    ~rectangle() override = default;

    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
        std::cout << "[rectangle] setPosition(" << x << ", " << y << ")\n";
    }

    void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        std::cout << "[rectangle] setSize(" << width << ", " << height << ")\n";
    }

    void setColor(const std::string& color) {
        color_ = color;
        std::cout << "[rectangle] setColor(\"" << color << "\")\n";
    }

    void setOpacity(double opacity) {
        opacity_ = opacity;
        std::cout << "[rectangle] setOpacity(" << opacity << ")\n";
    }

    void add(Object& child) {
        children_.push_back(std::shared_ptr<Object>(&child, [](Object*){}));
        child.on_added_to_parent(this);
        std::cout << "[rectangle] add(child)\n";
    }

    void render() override {
        std::cout << "[rectangle] Rendering rect at (" << x_ << "," << y_ << ") size (" << width_ << "x" << height_ << ") color " << color_ << "\n";
    }

private:
    int x_ = 0;
    int y_ = 0;
    int width_ = 50;
    int height_ = 50;
    std::string color_ = "#FF0000";
    double opacity_ = 1.0;
    std::vector<std::shared_ptr<Object>> children_;
};

// Factory for creating objects by type name
class ObjectFactory {
public:
    using Creator = std::function<std::shared_ptr<Object>()>;

    static ObjectFactory& instance() {
        static ObjectFactory factory;
        return factory;
    }

    void register_type(const std::string& name, Creator creator) {
        creators_[name] = std::move(creator);
    }

    std::shared_ptr<Object> create(const std::string& name) const {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        // Fallback: create generic Object
        std::cout << "[Factory] Unknown type '" << name << "', creating base Object\n";
        return std::make_shared<Object>();
    }

private:
    ObjectFactory() {
        // Register built-in types
        register_type("Window", []() { return std::make_shared<Window>(); });
        register_type("Button", []() { return std::make_shared<Button>(); });
        register_type("Panel", []() { return std::make_shared<Panel>(); });
        register_type("rectangle", []() { return std::make_shared<rectangle>(); });
        register_type("Object", []() { return std::make_shared<Object>(); });
    }

    std::unordered_map<std::string, Creator> creators_;
};

} // namespace runtime
} // namespace fxl