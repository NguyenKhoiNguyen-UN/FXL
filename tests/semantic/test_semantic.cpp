#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include "diagnostic.hpp"
#include <cassert>
#include <iostream>

using namespace fxl;

std::unique_ptr<Program> parse_source(const std::string& source, DiagnosticBag& diagnostics) {
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    if (diagnostics.has_errors()) return nullptr;
    
    Parser parser(tokens, &diagnostics);
    return parser.parse();
}

void test_semantic_basic() {
    std::string source = "import ui\nwindow = Window()";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    assert(!diagnostics.has_errors());
    std::cout << "test_semantic_basic: PASSED\n";
}

void test_semantic_duplicate_declaration() {
    std::string source = "window = Window()\nwindow = Button()";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    assert(diagnostics.has_errors());
    bool found_duplicate = false;
    for (const auto& diag : diagnostics.diagnostics()) {
        if (diag.code == "E_DUPLICATE_IDENTIFIER") {
            found_duplicate = true;
        }
    }
    assert(found_duplicate);
    std::cout << "test_semantic_duplicate_declaration: PASSED\n";
}

void test_semantic_unknown_object() {
    std::string source = "window.setSize(800, 600)";
    DiagnosticBag diagnostics;
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    assert(diagnostics.has_errors());
    bool found_unknown = false;
    for (const auto& diag : diagnostics.diagnostics()) {
        if (diag.code == "E_UNKNOWN_OBJECT") {
            found_unknown = true;
        }
    }
    assert(found_unknown);
    std::cout << "test_semantic_unknown_object: PASSED\n";
}

void test_semantic_nested_objects() {
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
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    assert(!diagnostics.has_errors());
    std::cout << "test_semantic_nested_objects: PASSED\n";
}

void test_semantic_auto_set() {
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
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    assert(!diagnostics.has_errors());
    std::cout << "test_semantic_auto_set: PASSED\n";
}

void test_semantic_nested_auto_directives() {
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
    auto program = parse_source(source, diagnostics);
    
    assert(program != nullptr);
    assert(!diagnostics.has_errors());
    
    SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    assert(!diagnostics.has_errors());
    std::cout << "test_semantic_nested_auto_directives: PASSED\n";
}

int main() {
    test_semantic_basic();
    test_semantic_duplicate_declaration();
    test_semantic_unknown_object();
    test_semantic_nested_objects();
    test_semantic_auto_set();
    test_semantic_nested_auto_directives();
    
    std::cout << "\nAll semantic tests PASSED!\n";
    return 0;
}