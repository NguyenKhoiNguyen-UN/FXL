#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include "auto_resolver.hpp"
#include "diagnostic.hpp"
#include <cassert>
#include <iostream>

using namespace fxl;

std::unique_ptr<Program> parse_and_analyze(const std::string& source, DiagnosticBag& diagnostics) {
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    if (diagnostics.has_errors()) return nullptr;
    
    Parser parser(tokens, &diagnostics);
    auto program = parser.parse();
    if (diagnostics.has_errors()) return nullptr;
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    if (diagnostics.has_errors()) return nullptr;
    
    return program;
}

void test_auto_set_basic() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)
    )
}
)";
    DiagnosticBag diagnostics;
    auto program = parse_and_analyze(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    
    assert(!diagnostics.has_errors());
    
    // Check that shorthand was resolved
    auto auto_dir = dynamic_cast<AutoDirective*>(program->statements[0].get());
    assert(auto_dir != nullptr);
    auto obj = dynamic_cast<ObjectDecl*>(auto_dir->body[0].get());
    assert(obj != nullptr);
    auto shorthand_stmt = dynamic_cast<ExpressionStatement*>(obj->block->statements[0].get());
    assert(shorthand_stmt != nullptr);
    auto shorthand = dynamic_cast<ShorthandCall*>(shorthand_stmt->expression.get());
    assert(shorthand != nullptr);
    assert(shorthand->resolved_setter == "setSize");
    assert(shorthand->resolved_target == "window");
    std::cout << "test_auto_set_basic: PASSED\n";
}

void test_auto_set_multiple() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)
        Position(100, 100)
        Text("Hello")
    )
}
)";
    DiagnosticBag diagnostics;
    auto program = parse_and_analyze(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    
    assert(!diagnostics.has_errors());
    
    auto auto_dir = dynamic_cast<AutoDirective*>(program->statements[0].get());
    auto obj = dynamic_cast<ObjectDecl*>(auto_dir->body[0].get());
    
    // Check all three shorthands were resolved
    int resolved_count = 0;
    for (const auto& stmt : obj->block->statements) {
        auto shorthand_stmt = dynamic_cast<ExpressionStatement*>(stmt.get());
        assert(shorthand_stmt != nullptr);
        auto shorthand = dynamic_cast<ShorthandCall*>(shorthand_stmt->expression.get());
        if (shorthand && !shorthand->resolved_setter.empty()) {
            resolved_count++;
            if (shorthand->name == "Size") {
                assert(shorthand->resolved_setter == "setSize");
            } else if (shorthand->name == "Position") {
                assert(shorthand->resolved_setter == "setPosition");
            } else if (shorthand->name == "Text") {
                assert(shorthand->resolved_setter == "setText");
            }
            assert(shorthand->resolved_target == "window");
        }
    }
    assert(resolved_count == 3);
    std::cout << "test_auto_set_multiple: PASSED\n";
}

void test_auto_set_nested_objects() {
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
    DiagnosticBag diagnostics;
    auto program = parse_and_analyze(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    
    assert(!diagnostics.has_errors());
    
    auto auto_dir = dynamic_cast<AutoDirective*>(program->statements[0].get());
    auto window = dynamic_cast<ObjectDecl*>(auto_dir->body[0].get());
    
    // Check window.Size resolved
    auto window_size_stmt = dynamic_cast<ExpressionStatement*>(window->block->statements[0].get());
    assert(window_size_stmt != nullptr);
    auto window_size = dynamic_cast<ShorthandCall*>(window_size_stmt->expression.get());
    assert(window_size != nullptr);
    assert(window_size->resolved_setter == "setSize");
    assert(window_size->resolved_target == "window");
    
    // Check rectangle object
    auto rect = dynamic_cast<ObjectDecl*>(window->block->statements[1].get());
    assert(rect != nullptr);
    assert(rect->name == "rectangle");
    
    // Check rectangle's shorthands resolved to rectangle
    int rect_resolved = 0;
    for (const auto& stmt : rect->block->statements) {
        auto shorthand_stmt = dynamic_cast<ExpressionStatement*>(stmt.get());
        assert(shorthand_stmt != nullptr);
        auto shorthand = dynamic_cast<ShorthandCall*>(shorthand_stmt->expression.get());
        if (shorthand && !shorthand->resolved_setter.empty()) {
            rect_resolved++;
            assert(shorthand->resolved_target == "rectangle");
            if (shorthand->name == "Color") {
                assert(shorthand->resolved_setter == "setColor");
            } else if (shorthand->name == "Opacity") {
                assert(shorthand->resolved_setter == "setOpacity");
            } else if (shorthand->name == "Size") {
                assert(shorthand->resolved_setter == "setSize");
            }
        }
    }
    assert(rect_resolved == 3);
    std::cout << "test_auto_set_nested_objects: PASSED\n";
}

void test_unknown_auto_set_operation() {
    std::string source = R"(
#autoSet {
    window = Window(
        Rotate(30)
    )
}
)";
    DiagnosticBag diagnostics;
    auto program = parse_and_analyze(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    
    assert(diagnostics.has_errors());
    bool found_unknown = false;
    for (const auto& diag : diagnostics.diagnostics()) {
        if (diag.code == "E_UNKNOWN_AUTOSET_OP") {
            found_unknown = true;
        }
    }
    assert(found_unknown);
    std::cout << "test_unknown_auto_set_operation: PASSED\n";
}

void test_auto_set_scope_isolation() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)
    )
}

window.setTitle("Test")
)";
    DiagnosticBag diagnostics;
    auto program = parse_and_analyze(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    
    assert(!diagnostics.has_errors());
    
    // The Size(800, 600) inside #autoSet should be resolved
    // The window.setTitle("Test") outside should NOT be affected
    auto auto_dir = dynamic_cast<AutoDirective*>(program->statements[0].get());
    auto window = dynamic_cast<ObjectDecl*>(auto_dir->body[0].get());
    auto shorthand_stmt = dynamic_cast<ExpressionStatement*>(window->block->statements[0].get());
    assert(shorthand_stmt != nullptr);
    auto shorthand = dynamic_cast<ShorthandCall*>(shorthand_stmt->expression.get());
    assert(shorthand != nullptr);
    assert(shorthand->resolved_setter == "setSize");
    
    // The method call outside should remain as MethodCall (wrapped in ExpressionStatement)
    auto method_call_stmt = dynamic_cast<ExpressionStatement*>(program->statements[1].get());
    assert(method_call_stmt != nullptr);
    auto method_call = dynamic_cast<MethodCall*>(method_call_stmt->expression.get());
    assert(method_call != nullptr);
    assert(method_call->method_name == "setTitle");
    std::cout << "test_auto_set_scope_isolation: PASSED\n";
}

void test_nested_auto_directives() {
    std::string source = R"(
#autoSet {
    window = Window(
        Size(800, 600)
        
        #autoPick {
            rectangle = Object(
                Color(#000000)
            )
        }
    )
}
)";
    DiagnosticBag diagnostics;
    auto program = parse_and_analyze(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    AutoSetResolver resolver(&diagnostics);
    resolver.resolve(*program);
    
    assert(!diagnostics.has_errors());
    
    // The outer Size should be resolved
    // The inner Color should NOT be resolved (autoPick not implemented)
    auto auto_dir = dynamic_cast<AutoDirective*>(program->statements[0].get());
    auto window = dynamic_cast<ObjectDecl*>(auto_dir->body[0].get());
    auto window_size_stmt = dynamic_cast<ExpressionStatement*>(window->block->statements[0].get());
    assert(window_size_stmt != nullptr);
    auto window_size = dynamic_cast<ShorthandCall*>(window_size_stmt->expression.get());
    assert(window_size != nullptr);
    assert(window_size->resolved_setter == "setSize");
    
    // The nested autoPick should be present but its body not resolved
    auto nested_auto = dynamic_cast<AutoDirective*>(window->block->statements[1].get());
    assert(nested_auto != nullptr);
    assert(nested_auto->kind == AutoDirective::Kind::AutoPick);
    std::cout << "test_nested_auto_directives: PASSED\n";
}

int main() {
    test_auto_set_basic();
    test_auto_set_multiple();
    test_auto_set_nested_objects();
    test_unknown_auto_set_operation();
    test_auto_set_scope_isolation();
    test_nested_auto_directives();
    
    std::cout << "\nAll auto_set tests PASSED!\n";
    return 0;
}