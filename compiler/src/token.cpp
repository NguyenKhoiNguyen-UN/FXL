#include "token.hpp"
#include <ostream>

namespace fxl {

std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::EndOfFile:   return "EndOfFile";
        case TokenType::Identifier:  return "Identifier";
        case TokenType::Import:      return "Import";
        case TokenType::AutoSet:     return "AutoSet";
        case TokenType::AutoPick:    return "AutoPick";
        case TokenType::Objects:     return "Objects";
        case TokenType::Visible:     return "Visible";
        case TokenType::TitleBar:    return "TitleBar";
        case TokenType::Number:      return "Number";
        case TokenType::String:      return "String";
        case TokenType::Color:       return "Color";
        case TokenType::Percentage:  return "Percentage";
        case TokenType::Equals:      return "Equals";
        case TokenType::Dot:         return "Dot";
        case TokenType::Comma:       return "Comma";
        case TokenType::Hash:        return "Hash";
        case TokenType::LParen:      return "LParen";
        case TokenType::RParen:      return "RParen";
        case TokenType::LBrace:      return "LBrace";
        case TokenType::RBrace:      return "RBrace";
        case TokenType::Left:        return "Left";
        case TokenType::Right:       return "Right";
        case TokenType::Top:         return "Top";
        case TokenType::Bottom:      return "Bottom";
        case TokenType::TopLeft:     return "TopLeft";
        case TokenType::TopRight:    return "TopRight";
        case TokenType::BottomLeft:  return "BottomLeft";
        case TokenType::BottomRight: return "BottomRight";
        case TokenType::RelativeLeft:   return "RelativeLeft";
        case TokenType::RelativeRight:  return "RelativeRight";
        case TokenType::RelativeTop:    return "RelativeTop";
        case TokenType::RelativeBottom: return "RelativeBottom";
        case TokenType::Invalid:     return "Invalid";
    }
    return "Unknown";
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token_type_to_string(token.type) << "(" << token.text;
    if (std::holds_alternative<double>(token.value)) {
        os << ", value=" << std::get<double>(token.value);
    } else if (std::holds_alternative<std::string>(token.value)) {
        os << ", value=" << std::get<std::string>(token.value);
    }
    os << ") at " << token.location;
    return os;
}

} // namespace fxl