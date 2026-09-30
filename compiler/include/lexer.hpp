#pragma once

#include "token.hpp"
#include "diagnostic.hpp"
#include <string>
#include <vector>
#include <memory>

namespace fxl {

class Lexer {
public:
    explicit Lexer(std::string source, DiagnosticBag* diagnostics = nullptr);
    
    // Get the next token
    Token next_token();
    
    // Tokenize the entire source
    std::vector<Token> tokenize();
    
    // Get current source location
    SourceLocation current_location() const;

private:
    std::string source_;
    DiagnosticBag* diagnostics_;
    size_t position_ = 0;
    int line_ = 1;
    int column_ = 1;
    char current_char_ = 0;
    SourceLocation token_start_location_;

    void advance();
    char peek(size_t offset = 1) const;
    bool is_at_end() const;
    
    Token make_token(TokenType type, const std::string& text);
    Token make_token(TokenType type, const std::string& text, double value);
    Token make_token(TokenType type, const std::string& text, std::string value);
    
    void skip_whitespace();
    bool skip_comment();
    
    Token read_identifier();
    Token read_number();
    Token read_string();
    Token read_color();
    Token read_percentage();
    
    bool is_identifier_start(char c) const;
    bool is_identifier_part(char c) const;
    bool looks_like_color() const;
};

} // namespace fxl