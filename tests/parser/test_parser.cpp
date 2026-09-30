#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "diagnostic.hpp"
#include <cassert>
#include <iostream>

using namespace fxl;

std::unique_ptr<Program> parse_source(const std::string& source, DiagnosticBag& diagnostics) {
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    if (diagnostics.has_errors()) {
        return nullptr;
    }
    
    Parser parser(tokens, &diagnostics);
    return parser.parse();
}

void test_import() {
    std::string source = "import ui";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    auto import = dynamic_cast<ImportDecl*>(program->statements[0].get());
    assert(import != nullptr);
    assert(import->module_name == "ui");
    std::cout << "test_import: PASSED\n";
}

void test_object_declaration() {
    std::string source = "window = Window";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    auto obj = dynamic_cast<ObjectDecl*>(program->statements[0].get());
    assert(obj != nullptr);
    assert(obj->name == "window");
    assert(obj->type_name == "Window");
    assert(obj->block == nullptr);
    std::cout << "test_object_declaration: PASSED\n";
}

void test_object_with_block() {
    std::string source = "window = Window(window.setSize(800, 600))";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    auto obj = dynamic_cast<ObjectDecl*>(program->statements[0].get());
    assert(obj != nullptr);
    assert(obj->block != nullptr);
    assert(obj->block->statements.size() == 1);
    // Statement is wrapped in ExpressionStatement
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(obj->block->statements[0].get());
    assert(expr_stmt != nullptr);
    auto method = dynamic_cast<MethodCall*>(expr_stmt->expression.get());
    assert(method != nullptr);
    assert(method->method_name == "setSize");
    assert(method->arguments.size() == 2);
    std::cout << "test_object_with_block: PASSED\n";
}

void test_nested_objects() {
    std::string source = R"(
window = Window(
    rectangle = Object(
        rectangle.setColor(#000000)
    )
)
)";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    auto window = dynamic_cast<ObjectDecl*>(program->statements[0].get());
    assert(window != nullptr);
    assert(window->name == "window");
    assert(window->block != nullptr);
    assert(window->block->statements.size() == 1);
    
    auto rect = dynamic_cast<ObjectDecl*>(window->block->statements[0].get());
    assert(rect != nullptr);
    assert(rect->name == "rectangle");
    assert(rect->type_name == "Object");
    assert(rect->block != nullptr);
    assert(rect->block->statements.size() == 1);
    
    auto rect_expr_stmt = dynamic_cast<ExpressionStatement*>(rect->block->statements[0].get());
    assert(rect_expr_stmt != nullptr);
    auto method = dynamic_cast<MethodCall*>(rect_expr_stmt->expression.get());
    assert(method != nullptr);
    assert(method->method_name == "setColor");
    std::cout << "test_nested_objects: PASSED\n";
}

void test_auto_set_block() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)
    )
}
)";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    auto auto_dir = dynamic_cast<AutoDirective*>(program->statements[0].get());
    assert(auto_dir != nullptr);
    assert(auto_dir->kind == AutoDirective::Kind::AutoSet);
    assert(auto_dir->body.size() == 1);
    
    auto obj = dynamic_cast<ObjectDecl*>(auto_dir->body[0].get());
    assert(obj != nullptr);
    assert(obj->name == "window");
    assert(obj->is_auto_set_context == true);
    assert(obj->block != nullptr);
    assert(obj->block->statements.size() == 1);
    
    // The Size(800, 600) should be parsed as a ShorthandCall (wrapped in ExpressionStatement)
    auto shorthand_stmt = dynamic_cast<ExpressionStatement*>(obj->block->statements[0].get());
    assert(shorthand_stmt != nullptr);
    auto shorthand = dynamic_cast<ShorthandCall*>(shorthand_stmt->expression.get());
    assert(shorthand != nullptr);
    assert(shorthand->name == "Size");
    assert(shorthand->arguments.size() == 2);
    std::cout << "test_auto_set_block: PASSED\n";
}

void test_method_call_parsing() {
    std::string source = "window.setSize(800, 600)";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    // Top-level method call is wrapped in ExpressionStatement
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(program->statements[0].get());
    assert(expr_stmt != nullptr);
    auto method = dynamic_cast<MethodCall*>(expr_stmt->expression.get());
    assert(method != nullptr);
    assert(method->method_name == "setSize");
    assert(method->arguments.size() == 2);
    
    auto target = dynamic_cast<Identifier*>(method->target.get());
    assert(target != nullptr);
    assert(target->name == "window");
    std::cout << "test_method_call_parsing: PASSED\n";
}

void test_multiple_auto_directives() {
    std::string source = R"(
#autoSet {
    window = Window(Size(800, 600))
}

#autoPick {
    rectangle = Object(Color(#000000))
}
)";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 2);
    
    auto auto_set = dynamic_cast<AutoDirective*>(program->statements[0].get());
    assert(auto_set != nullptr);
    assert(auto_set->kind == AutoDirective::Kind::AutoSet);
    
    auto auto_pick = dynamic_cast<AutoDirective*>(program->statements[1].get());
    assert(auto_pick != nullptr);
    assert(auto_pick->kind == AutoDirective::Kind::AutoPick);
    std::cout << "test_multiple_auto_directives: PASSED\n";
}

void test_combined_directive_error() {
    std::string source = "#autoSet #autoPick { }";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    // Parser should still produce a program but with error
    assert(diagnostics.has_errors());
    bool found_combined_error = false;
    for (const auto& diag : diagnostics.diagnostics()) {
        if (diag.message.find("combine") != std::string::npos ||
            diag.message.find("multiple") != std::string::npos) {
            found_combined_error = true;
        }
    }
    assert(found_combined_error);
    std::cout << "test_combined_directive_error: PASSED\n";
}

void test_shorthand_call_outside_auto_set() {
    std::string source = "window = Window(Size(800, 600))";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    auto obj = dynamic_cast<ObjectDecl*>(program->statements[0].get());
    assert(obj != nullptr);
    assert(obj->block != nullptr);
    assert(obj->block->statements.size() == 1);
    
    // Should be parsed as ShorthandCall (wrapped in ExpressionStatement)
    auto shorthand_stmt = dynamic_cast<ExpressionStatement*>(obj->block->statements[0].get());
    assert(shorthand_stmt != nullptr);
    auto shorthand = dynamic_cast<ShorthandCall*>(shorthand_stmt->expression.get());
    assert(shorthand != nullptr);
    assert(shorthand->name == "Size");
    std::cout << "test_shorthand_call_outside_auto_set: PASSED\n";
}

void test_string_and_number_args() {
    std::string source = R"(button.setText("Click me"))";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    assert(program->statements.size() == 1);
    // Top-level method call is wrapped in ExpressionStatement
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(program->statements[0].get());
    assert(expr_stmt != nullptr);
    auto method = dynamic_cast<MethodCall*>(expr_stmt->expression.get());
    assert(method != nullptr);
    assert(method->method_name == "setText");
    assert(method->arguments.size() == 1);
    
    auto arg_literal = dynamic_cast<Literal*>(method->arguments[0].expression.get());
    assert(arg_literal != nullptr);
    assert(arg_literal->kind == Literal::Kind::String);
    assert(std::get<std::string>(arg_literal->value) == "Click me");
    std::cout << "test_string_and_number_args: PASSED\n";
}

int main() {
    test_import();
    test_object_declaration();
    test_object_with_block();
    test_nested_objects();
    test_auto_set_block();
    test_method_call_parsing();
    test_multiple_auto_directives();
    test_combined_directive_error();
    test_shorthand_call_outside_auto_set();
    test_string_and_number_args();
    
    std::cout << "\nAll parser tests PASSED!\n";
    return 0;
}