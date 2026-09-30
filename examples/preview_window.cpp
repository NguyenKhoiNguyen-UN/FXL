#include <memory>
#include <string>
#include <vector>
#include <fxl/runtime.hpp>

int main() {
    // import ui
    // #autoSet block
    auto window = fxl::runtime::Window();
    window.setSize(800, 600);
    return 0;
}
