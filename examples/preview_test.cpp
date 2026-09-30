#include <memory>
#include <string>
#include <vector>
#include <fxl/runtime.hpp>

int main() {
    // import ui
    // #autoSet block
    auto window = fxl::runtime::Window();
    window.setSize(800, 600);
    window.setPosition(100, 100);
    auto rectangle = fxl::runtime::Object();
    rectangle.setColor("#000000");
    rectangle.setOpacity(15);
    rectangle.setSize(13, 45);
    window.add(rectangle);
    return 0;
}
