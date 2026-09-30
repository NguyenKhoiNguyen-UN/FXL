#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include "auto_resolver.hpp"
#include "codegen.hpp"
#include "diagnostic.hpp"
#include <cassert>
#include <iostream>
#include <string>

using namespace fxl;

std::unique_ptr<Program> compile_pipeline(const std::string& source, DiagnosticBag& diagnostics) {
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    if (diagnostics.has_errors()) return nullptr;
    
    Parser parser(tokens, &diagnostics);
    auto program = parser.parse();
    if (diagnostics.has_errors()) return nullptr;
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    if (diagnostics.has_errors()) return nullptr;
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    if (diagnostics.has_errors()) return nullptr;
    
    return program;
}

std::string generate_code(const std::string& source) {
    DiagnosticBag diagnostics;
    auto program = compile_pipeline(source, diagnostics);
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    CodeGenerator codegen(&diagnostics);
    return codegen.generate(*program);
}

void test_codegen_basic() {
    std::string source = "import ui\nwindow = Window()";
    std::string output = generate_code(source);
    
    assert(output.find("auto window = fxl::runtime::Window();") != std::string::npos);
    assert(output.find("import ui") != std::string::npos);
    std::cout << "test_codegen_basic: PASSED\n";
}

void test_codegen_method_call() {
    std::string source = "window = Window()\nwindow.setSize(800, 600)";
    std::string output = generate_code(source);
    
    assert(output.find("auto window = fxl::runtime::Window();") != std::string::npos);
    assert(output.find("window.setSize(800, 600);") != std::string::npos);
    std::cout << "test_codegen_method_call: PASSED\n";
}

void test_codegen_object_block() {
    std::string source = "window = Window(window.setSize(800, 600))";
    std::string output = generate_code(source);
    
    assert(output.find("auto window = fxl::runtime::Window();") != std::string::npos);
    assert(output.find("window.setSize(800, 600);") != std::string::npos);
    std::cout << "test_codegen_object_block: PASSED\n";
}

void test_codegen_nested_objects() {
    std::string source = R"(
window = Window(
    rectangle = Object(
        rectangle.setColor(#000000)
    )
)
)";
    std::string output = generate_code(source);
    
    assert(output.find("auto window = fxl::runtime::Window();") != std::string::npos);
    assert(output.find("auto rectangle = fxl::runtime::Object();") != std::string::npos);
    assert(output.find("rectangle.setColor(\"#000000\");") != std::string::npos);
    assert(output.find("window.add(rectangle);") != std::string::npos);
    std::cout << "test_codegen_nested_objects: PASSED\n";
}

void test_codegen_auto_set() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)
    )
}
)";
    std::string output = generate_code(source);
    
    assert(output.find("auto window = fxl::runtime::Window();") != std::string::npos);
    assert(output.find("window.setSize(800, 600);") != std::string::npos);
    std::cout << "test_codegen_auto_set: PASSED\n";
}

void test_codegen_auto_set_nested() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)

        rectangle = Object(
            Color(#000000)
            Opacity(15%)
            Size(13, 45)
        )
    )
}
)";
    std::string output = generate_code(source);
    
    assert(output.find("auto window = fxl::runtime::Window();") != std::string::npos);
    assert(output.find("window.setSize(800, 600);") != std::string::npos);
    assert(output.find("auto rectangle = fxl::runtime::Object();") != std::string::npos);
    assert(output.find("rectangle.setColor(\"#000000\");") != std::string::npos);
    assert(output.find("rectangle.setOpacity(15);") != std::string::npos);
    assert(output.find("rectangle.setSize(13, 45);") != std::string::npos);
    assert(output.find("window.add(rectangle);") != std::string::npos);
    std::cout << "test_codegen_auto_set_nested: PASSED\n";
}

void test_codegen_string_args() {
    std::string source = "button = Button()\nbutton.setText(\"Click me\")\n";
    std::string output = generate_code(source);
    
    assert(output.find("auto button = fxl::runtime::Button();") != std::string::npos);
    assert(output.find("button.setText(\"Click me\");") != std::string::npos);
    std::cout << "test_codegen_string_args: PASSED\n";
}

void test_codegen_percentage() {
    std::string source = "window = Window(window.setOpacity(50%))";
    std::string output = generate_code(source);
    
    assert(output.find("window.setOpacity(50);") != std::string::npos);
    std::cout << "test_codegen_percentage: PASSED\n";
}

int main() {
    test_codegen_basic();
    test_codegen_method_call();
    test_codegen_object_block();
    test_codegen_nested_objects();
    test_codegen_auto_set();
    test_codegen_auto_set_nested();
    test_codegen_string_args();
    test_codegen_percentage();
    
    std::cout << "\nAll codegen tests PASSED!\n";
    return 0;
}