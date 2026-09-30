#pragma once

#include "diagnostic.hpp"
#include <string>
#include <variant>
#include <optional>
#include <algorithm>

namespace fxl {

enum class TokenType {
    // End of file
    EndOfFile,

    // Identifiers
    Identifier,

    // Keywords
    Import,
    AutoSet,
    AutoPick,  // Recognized but not fully implemented
    Objects,   // Objects() constructor keyword
    Visible,   // visible keyword for window visibility
    TitleBar,  // titlebar keyword

    // Literals
    Number,      // Integer or floating point
    String,      // "string"
    Color,       // #RRGGBB or #RGB
    Percentage,  // 15%

    // Edge position keywords
    Left,        // left
    Right,       // right
    Top,         // top
    Bottom,      // bottom

    // Corner position keywords
    TopLeft,     // topLeft
    TopRight,    // topRight
    BottomLeft,  // bottomLeft
    BottomRight, // bottomRight

    // Relative position keywords
    RelativeLeft,   // left (relative)
    RelativeRight,  // right (relative)
    RelativeTop,    // top (relative)
    RelativeBottom, // bottom (relative)

    // Operators and punctuation
    Equals,      // =
    Dot,         // .
    Comma,       // ,
    Hash,        // #
    LParen,      // (
    RParen,      // )
    LBrace,      // {
    RBrace,      // }

    // Invalid token
    Invalid
};

struct Token {
    TokenType type = TokenType::Invalid;
    std::string text;
    SourceLocation location;
    std::variant<std::monostate, double, std::string> value;  // For literals

    Token() = default;
    Token(TokenType type_, std::string text_, SourceLocation location_)
        : type(type_), text(std::move(text_)), location(location_) {}

    Token(TokenType type_, std::string text_, SourceLocation location_, double value_)
        : type(type_), text(std::move(text_)), location(location_), value(value_) {}

    Token(TokenType type_, std::string text_, SourceLocation location_, std::string value_)
        : type(type_), text(std::move(text_)), location(location_), value(std::move(value_)) {}

    bool is(TokenType t) const { return type == t; }
    bool is_one_of(std::initializer_list<TokenType> types) const {
        return std::any_of(types.begin(), types.end(), [this](TokenType t) { return type == t; });
    }

    bool is_eof() const { return type == TokenType::EndOfFile; }
    bool is_identifier() const { return type == TokenType::Identifier; }
    bool is_keyword() const {
        return type == TokenType::Import || type == TokenType::AutoSet || type == TokenType::AutoPick;
    }

    std::optional<double> as_number() const {
        if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }
        return std::nullopt;
    }

    std::optional<std::string> as_string() const {
        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }
        return std::nullopt;
    }
};

std::string token_type_to_string(TokenType type);
std::ostream& operator<<(std::ostream& os, const Token& token);

} // namespace fxl