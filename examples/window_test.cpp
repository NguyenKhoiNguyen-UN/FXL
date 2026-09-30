#include <memory>
#include <string>
#include <vector>
#include <fxl/runtime.hpp>
using namespace fxl::runtime;

int main() {
    // import ui
    // #autoSet block
    auto window = Window();
    window.setSize(800, 600);
    window.setPosition(100, 100);
    auto rectangle = Object();
    rectangle.setColor("#000000");
    rectangle.setOpacity(15);
    rectangle.setSize(13, 45);
    window.add(rectangle);
    return 0;
}
