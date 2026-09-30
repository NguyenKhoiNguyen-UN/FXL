#include "lexer.hpp"
#include "diagnostic.hpp"
#include <vector>
#include <cassert>
#include <iostream>

using namespace fxl;

void test_basic_tokens() {
    std::string source = "import ui";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens.size() == 3);  // import, ui, EOF
    assert(tokens[0].type == TokenType::Import);
    assert(tokens[1].type == TokenType::Identifier);
    assert(tokens[1].text == "ui");
    assert(tokens[2].type == TokenType::EndOfFile);
    std::cout << "test_basic_tokens: PASSED\n";
}

void test_variable_assignment() {
    std::string source = "window = Window()";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens[0].type == TokenType::Identifier);
    assert(tokens[0].text == "window");
    assert(tokens[1].type == TokenType::Equals);
    assert(tokens[2].type == TokenType::Identifier);
    assert(tokens[2].text == "Window");
    assert(tokens[3].type == TokenType::LParen);
    assert(tokens[4].type == TokenType::RParen);
    assert(tokens[5].type == TokenType::EndOfFile);
    std::cout << "test_variable_assignment: PASSED\n";
}

void test_method_call() {
    std::string source = "window.setSize(800, 600)";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens[0].type == TokenType::Identifier);
    assert(tokens[0].text == "window");
    assert(tokens[1].type == TokenType::Dot);
    assert(tokens[2].type == TokenType::Identifier);
    assert(tokens[2].text == "setSize");
    assert(tokens[3].type == TokenType::LParen);
    assert(tokens[4].type == TokenType::Number);
    assert(tokens[4].as_number().value() == 800);
    assert(tokens[5].type == TokenType::Comma);
    assert(tokens[6].type == TokenType::Number);
    assert(tokens[6].as_number().value() == 600);
    assert(tokens[7].type == TokenType::RParen);
    std::cout << "test_method_call: PASSED\n";
}

void test_color_literal() {
    std::string source = "#000000";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens[0].type == TokenType::Color);
    assert(tokens[0].as_string().value() == "#000000");
    std::cout << "test_color_literal: PASSED\n";
}

void test_color_3digit() {
    std::string source = "#abc";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens[0].type == TokenType::Color);
    assert(tokens[0].as_string().value() == "#AABBCC");
    std::cout << "test_color_3digit: PASSED\n";
}

void test_percentage() {
    std::string source = "15%";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens[0].type == TokenType::Percentage);
    assert(tokens[0].as_number().value() == 15.0);
    std::cout << "test_percentage: PASSED\n";
}

void test_string_literal() {
    std::string source = R"("Click me")";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    assert(tokens[0].type == TokenType::String);
    assert(tokens[0].as_string().value() == "Click me");
    std::cout << "test_string_literal: PASSED\n";
}

void test_auto_set() {
    std::string source = "#autoSet { }";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    bool found_auto_set = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::AutoSet) {
            found_auto_set = true;
            assert(t.text == "#autoSet");
        }
    }
    assert(found_auto_set);
    std::cout << "test_auto_set: PASSED\n";
}

void test_auto_pick() {
    std::string source = "#autoPick { }";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    bool found_auto_pick = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::AutoPick) {
            found_auto_pick = true;
            assert(t.text == "#autoPick");
        }
    }
    assert(found_auto_pick);
    std::cout << "test_auto_pick: PASSED\n";
}

void test_combined_directive_tokens() {
    std::string source = "#autoSet #autoPick { }";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    // Lexer should tokenize both directives, parser will report the error
    assert(!diagnostics.has_errors());
    int auto_count = 0;
    for (const auto& t : tokens) {
        if (t.type == TokenType::AutoSet || t.type == TokenType::AutoPick) {
            auto_count++;
        }
    }
    assert(auto_count == 2);
    std::cout << "test_combined_directive_tokens: PASSED\n";
}

void test_numbers() {
    std::string source = "42 3.14";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    int num_count = 0;
    for (const auto& t : tokens) {
        if (t.type == TokenType::Number) {
            num_count++;
        }
    }
    assert(num_count == 2);
    std::cout << "test_numbers: PASSED\n";
}

void test_source_locations() {
    std::string source = "import ui\nwindow = Window()";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    // import at line 1, column 1
    assert(tokens[0].location.line == 1);
    assert(tokens[0].location.column == 1);
    // window at line 2, column 1
    assert(tokens[2].location.line == 2);
    assert(tokens[2].location.column == 1);
    std::cout << "test_source_locations: PASSED\n";
}

void test_nested_object_tokens() {
    std::string source = R"(
window = Window(
    window.setSize(800, 600)
    rectangle = Object(
        rectangle.setColor(#000000)
    )
)
)";
    DiagnosticBag diagnostics;
    Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    assert(!diagnostics.has_errors());
    int color_count = 0;
    for (const auto& t : tokens) {
        if (t.type == TokenType::Color) color_count++;
    }
    assert(color_count == 1);
    std::cout << "test_nested_object_tokens: PASSED\n";
}

int main() {
    test_basic_tokens();
    test_variable_assignment();
    test_method_call();
    test_color_literal();
    test_color_3digit();
    test_percentage();
    test_string_literal();
    test_auto_set();
    test_auto_pick();
    test_combined_directive_tokens();
    test_numbers();
    test_source_locations();
    test_nested_object_tokens();
    
    std::cout << "\nAll lexer tests PASSED!\n";
    return 0;
}