#include "lexer.hpp"
#include <cctype>
#include <algorithm>

namespace fxl {

Lexer::Lexer(std::string source, DiagnosticBag* diagnostics)
    : source_(std::move(source)), diagnostics_(diagnostics) {
    if (!source_.empty()) {
        current_char_ = source_[0];
    }
}

SourceLocation Lexer::current_location() const {
    return SourceLocation{line_, column_};
}

void Lexer::advance() {
    if (current_char_ == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    position_++;
    if (position_ < source_.size()) {
        current_char_ = source_[position_];
    } else {
        current_char_ = 0;
    }
}

char Lexer::peek(size_t offset) const {
    size_t pos = position_ + offset;
    if (pos < source_.size()) {
        return source_[pos];
    }
    return 0;
}

bool Lexer::is_at_end() const {
    return current_char_ == 0;
}

Token Lexer::make_token(TokenType type, const std::string& text) {
    return Token(type, text, token_start_location_);
}

Token Lexer::make_token(TokenType type, const std::string& text, double value) {
    return Token(type, text, token_start_location_, value);
}

Token Lexer::make_token(TokenType type, const std::string& text, std::string value) {
    return Token(type, text, token_start_location_, std::move(value));
}

void Lexer::skip_whitespace() {
    while (!is_at_end() && (current_char_ == ' ' || current_char_ == '\t' || current_char_ == '\r' || current_char_ == '\n')) {
        advance();
    }
}

bool Lexer::skip_comment() {
    if (current_char_ == '/' && peek() == '/') {
        while (!is_at_end() && current_char_ != '\n') {
            advance();
        }
        return true;
    }
    return false;
}

bool Lexer::is_identifier_start(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::is_identifier_part(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::looks_like_color() const {
    // Check if the next characters form a valid color: # + 3 or 6 hex digits
    size_t pos = position_ + 1; // skip '#'
    int hex_count = 0;
    while (pos < source_.size() && std::isxdigit(static_cast<unsigned char>(source_[pos]))) {
        hex_count++;
        pos++;
    }
    return (hex_count == 3 || hex_count == 6) && (pos >= source_.size() || !std::isxdigit(static_cast<unsigned char>(source_[pos])));
}

Token Lexer::read_identifier() {
    size_t start = position_;

    while (!is_at_end() && is_identifier_part(current_char_)) {
        advance();
    }

    std::string text = source_.substr(start, position_ - start);
    
    // Check for keywords
    if (text == "import") {
        return make_token(TokenType::Import, text);
    } else if (text == "autoSet") {
        return make_token(TokenType::AutoSet, text);
    } else if (text == "autoPick") {
        return make_token(TokenType::AutoPick, text);
    } else if (text == "Objects") {
        return make_token(TokenType::Objects, text);
    } else if (text == "visible") {
        return make_token(TokenType::Visible, text);
    } else if (text == "titlebar") {
        return make_token(TokenType::TitleBar, text);
    } else if (text == "left") {
        return make_token(TokenType::Left, text);
    } else if (text == "right") {
        return make_token(TokenType::Right, text);
    } else if (text == "top") {
        return make_token(TokenType::Top, text);
    } else if (text == "bottom") {
        return make_token(TokenType::Bottom, text);
    } else if (text == "topLeft") {
        return make_token(TokenType::TopLeft, text);
    } else if (text == "topRight") {
        return make_token(TokenType::TopRight, text);
    } else if (text == "bottomLeft") {
        return make_token(TokenType::BottomLeft, text);
    } else if (text == "bottomRight") {
        return make_token(TokenType::BottomRight, text);
    } else if (text == "visible") {
        return make_token(TokenType::Visible, text);
    } else if (text == "titlebar") {
        return make_token(TokenType::TitleBar, text);
    } else if (text == "Window") {
        return make_token(TokenType::Identifier, text); // Window is a type but also can be identifier
    } else if (text == "Button") {
        return make_token(TokenType::Identifier, text);
    } else if (text == "Panel") {
        return make_token(TokenType::Identifier, text);
    } else if (text == "rectangle") {
        return make_token(TokenType::Identifier, text);
    } else if (text == "Object") {
        return make_token(TokenType::Identifier, text);
    }
    
    return make_token(TokenType::Identifier, text);
}

Token Lexer::read_number() {
    size_t start = position_;

    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(current_char_))) {
        advance();
    }

    if (current_char_ == '.' && peek() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance(); // consume '.'
        while (!is_at_end() && std::isdigit(static_cast<unsigned char>(current_char_))) {
            advance();
        }
    }

    std::string text = source_.substr(start, position_ - start);
    double value = std::stod(text);
    return make_token(TokenType::Number, text, value);
}

Token Lexer::read_string() {
    size_t start = position_;
    advance(); // consume opening quote

    std::string value;
    while (!is_at_end() && current_char_ != '"') {
        if (current_char_ == '\\') {
            advance();
            if (is_at_end()) break;
            switch (current_char_) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: value += current_char_; break;
            }
        } else {
            value += current_char_;
        }
        advance();
    }

    if (is_at_end()) {
        if (diagnostics_) {
            diagnostics_->add_error("Unterminated string literal", current_location());
        }
        std::string text = source_.substr(start, position_ - start);
        return make_token(TokenType::Invalid, text, value);
    }

    advance(); // consume closing quote
    std::string text = source_.substr(start, position_ - start);
    return make_token(TokenType::String, text, std::move(value));
}

Token Lexer::read_color() {
    size_t start = position_;
    advance(); // consume '#'

    std::string hex_chars;
    while (!is_at_end() && std::isxdigit(static_cast<unsigned char>(current_char_))) {
        hex_chars += current_char_;
        advance();
    }

    if (hex_chars.size() != 3 && hex_chars.size() != 6) {
        if (diagnostics_) {
            diagnostics_->add_error(
                "Invalid color format: expected 3 or 6 hex digits, got " + std::to_string(hex_chars.size()),
                SourceLocation{line_, column_ - static_cast<int>(hex_chars.size()) - 1}
            );
        }
        std::string text = source_.substr(start, position_ - start);
        return make_token(TokenType::Invalid, text);
    }

    std::string text = source_.substr(start, position_ - start);
    // Normalize to 6-digit format
    std::string normalized;
    if (hex_chars.size() == 3) {
        for (char c : hex_chars) {
            normalized += c;
            normalized += c;
        }
    } else {
        normalized = hex_chars;
    }
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
    std::string value = "#" + normalized;
    
    return make_token(TokenType::Color, text, std::move(value));
}

Token Lexer::read_percentage() {
    size_t start = position_;
    
    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(current_char_))) {
        advance();
    }

    if (current_char_ == '%') {
        advance(); // consume '%'
        std::string text = source_.substr(start, position_ - start);
        std::string num_str = source_.substr(start, position_ - start - 1);
        double value = std::stod(num_str);
        return make_token(TokenType::Percentage, text, value);
    } else {
        // Not a percentage, backtrack and read as number
        position_ = start;
        column_ = column_ - static_cast<int>(position_ - start);
        current_char_ = source_[position_];
        return read_number();
    }
}

Token Lexer::next_token() {
    skip_whitespace();
    
    // Skip comments
    while (skip_comment()) {
        skip_whitespace();
    }

    // Save start location for this token
    token_start_location_ = current_location();

    if (is_at_end()) {
        return make_token(TokenType::EndOfFile, "");
    }

    // Single-character tokens
    switch (current_char_) {
        case '=': advance(); return make_token(TokenType::Equals, "=");
        case '.': advance(); return make_token(TokenType::Dot, ".");
        case ',': advance(); return make_token(TokenType::Comma, ",");
        case '(': advance(); return make_token(TokenType::LParen, "(");
        case ')': advance(); return make_token(TokenType::RParen, ")");
        case '{': advance(); return make_token(TokenType::LBrace, "{");
        case '}': advance(); return make_token(TokenType::RBrace, "}");
        case '#': {
            // Try to parse as color first (3 or 6 hex digits)
            if (looks_like_color()) {
                return read_color();
            }
            // Check for #autoSet or #autoPick
            if (peek() && is_identifier_start(peek())) {
                size_t saved_pos = position_;
                
                advance(); // consume '#'
                while (!is_at_end() && is_identifier_part(current_char_)) {
                    advance();
                }
                std::string text = source_.substr(saved_pos, position_ - saved_pos);
                
                if (text == "#autoSet") {
                    return make_token(TokenType::AutoSet, text);
                } else if (text == "#autoPick") {
                    return make_token(TokenType::AutoPick, text);
                } else {
                    if (diagnostics_) {
                        diagnostics_->add_error("Unknown directive '" + text + "'", token_start_location_);
                    }
                    return make_token(TokenType::Invalid, text);
                }
            } else {
                // Just a standalone # character
                advance();
                return make_token(TokenType::Hash, "#");
            }
        }
        case '"': return read_string();
        default:
            if (std::isdigit(static_cast<unsigned char>(current_char_))) {
                // Check if it's a percentage by looking ahead
                Token token = read_percentage();
                if (token.type == TokenType::Percentage) {
                    return token;
                }
                // If not percentage, the read_number was already called inside read_percentage
                return token;
            }
            if (is_identifier_start(current_char_)) {
                return read_identifier();
            }
            // Invalid character
            char c = current_char_;
            advance();
            if (diagnostics_) {
                diagnostics_->add_error("Unexpected character '" + std::string(1, c) + "'", token_start_location_);
            }
            return make_token(TokenType::Invalid, std::string(1, c));
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token token = next_token();
        tokens.push_back(token);
        if (token.is_eof()) break;
    }
    return tokens;
}

} // namespace fxl