#include "parser.hpp"
#include "ast.hpp"
#include <stdexcept>
#include <cctype>
#include <algorithm>

namespace fxl {

Parser::Parser(const std::vector<Token>& tokens, DiagnosticBag* diagnostics)
    : tokens_(tokens), diagnostics_(diagnostics) {}

const Token& Parser::current_token() const {
    if (current_ < tokens_.size()) {
        return tokens_[current_];
    }
    static Token eof_token(TokenType::EndOfFile, "", SourceLocation{0, 0});
    return eof_token;
}

const Token& Parser::peek_token(size_t offset) const {
    if (current_ + offset < tokens_.size()) {
        return tokens_[current_ + offset];
    }
    static Token eof_token(TokenType::EndOfFile, "", SourceLocation{0, 0});
    return eof_token;
}

const Token& Parser::previous_token() const {
    if (current_ > 0) {
        return tokens_[current_ - 1];
    }
    static Token eof_token(TokenType::EndOfFile, "", SourceLocation{0, 0});
    return eof_token;
}

bool Parser::is_at_end() const {
    return current_token().is_eof();
}

bool Parser::check(TokenType type) const {
    return current_token().is(type);
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            current_++;
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& error_message) {
    if (check(type)) {
        return tokens_[current_++];
    }
    error_at_current(error_message);
    return current_token();
}

Token Parser::consume_identifier(const std::string& error_message) {
    if (current_token().is_identifier()) {
        return tokens_[current_++];
    }
    error_at_current(error_message);
    return current_token();
}

void Parser::error(const std::string& message, SourceLocation location) {
    if (diagnostics_) {
        diagnostics_->add_error(message, location);
    }
}

void Parser::error_at_current(const std::string& message) {
    error(message, current_token().location);
}

void Parser::error_at_previous(const std::string& message) {
    error(message, previous_token().location);
}

std::unique_ptr<Statement> Parser::synchronize() {
    while (!is_at_end()) {
        if (previous_token().is(TokenType::RBrace) || previous_token().is(TokenType::RParen)) {
            break;
        }
        switch (current_token().type) {
            case TokenType::Import:
            case TokenType::AutoSet:
            case TokenType::AutoPick:
            case TokenType::Identifier:
            case TokenType::RBrace:
                return nullptr;
            default:
                break;
        }
        current_++;
    }
    return nullptr;
}

std::unique_ptr<Program> Parser::parse_program() {
    auto program = std::make_unique<Program>();
    if (!tokens_.empty()) {
        program->location = tokens_[0].location;
    }

    while (!is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            program->statements.push_back(std::move(stmt));
        } else {
            current_++;
        }
    }
    return program;
}

std::unique_ptr<Statement> Parser::parse_statement() {
    if (check(TokenType::Import)) {
        return parse_import();
    }
    if (check(TokenType::AutoSet) || check(TokenType::AutoPick)) {
        return parse_auto_directive();
    }
    if (check(TokenType::Identifier)) {
        // Look ahead to see if it's followed by '=' (object declaration)
        // or '(' (shorthand call in autoSet context)
        // or '.' (method call)
        if (peek_token().is(TokenType::Equals)) {
            return parse_object_decl();
        } else {
            // Could be method call or shorthand call
            auto expr = parse_expression();
            if (expr) {
                // Wrap expression as a statement
                return std::make_unique<ExpressionStatement>(std::move(expr), current_token().location);
            }
        }
    }
    error_at_current("Expected statement");
    return synchronize();
}

std::unique_ptr<ImportDecl> Parser::parse_import() {
    Token import_token = consume(TokenType::Import, "Expected 'import'");
    Token name_token = consume_identifier("Expected module name after 'import'");
    
    auto import = std::make_unique<ImportDecl>(name_token.text, import_token.location);
    return import;
}

std::unique_ptr<ObjectDecl> Parser::parse_object_decl() {
    Token name_token = consume_identifier("Expected object name");
    consume(TokenType::Equals, "Expected '=' after object name");
    
    // Check if next token is 'Objects' keyword (without import form)
    if (check(TokenType::Objects)) {
        // Form: identifier = object_type = Objects(...)
        Token objects_token = consume(TokenType::Objects, "Expected 'Objects'");
        consume(TokenType::Equals, "Expected '=' after 'Objects'");
        Token type_token = consume_identifier("Expected object type (e.g., Window, Object)");
        
        std::unique_ptr<ObjectBlock> block = nullptr;
        if (check(TokenType::LParen)) {
            block = parse_object_block();
        }
        
        auto decl = std::make_unique<ObjectDecl>(name_token.text, type_token.text, std::move(block), name_token.location);
        return decl;
    } else {
        // Form: identifier = object_type(...)  (with import FXL.Objects)
        // Or: identifier = object_type = Objects(...) without import (handled above)
        Token type_token = consume_identifier("Expected object type (e.g., Window, Object)");
        
        std::unique_ptr<ObjectBlock> block = nullptr;
        if (check(TokenType::LParen)) {
            block = parse_object_block();
        }
        
        auto decl = std::make_unique<ObjectDecl>(name_token.text, type_token.text, std::move(block), name_token.location);
        return decl;
    }
}

std::unique_ptr<ObjectBlock> Parser::parse_object_block() {
    Token lparen = consume(TokenType::LParen, "Expected '(' to start object block");
    
    auto block = std::make_unique<ObjectBlock>();
    block->location = lparen.location;
    
    while (!check(TokenType::RParen) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        } else {
            if (is_at_end()) break;
            current_++;
        }
    }
    
    Token rparen = consume(TokenType::RParen, "Expected ')' to end object block");
    block->range = SourceRange{lparen.location, rparen.location};
    
    return block;
}

ExpressionPtr Parser::parse_method_call(ExpressionPtr target) {
    Token dot = consume(TokenType::Dot, "Expected '.'");
    Token method_token = consume_identifier("Expected method name after '.'");
    
    consume(TokenType::LParen, "Expected '(' after method name");
    std::vector<Argument> args = parse_arguments();
    
    auto call = std::make_unique<MethodCall>(
        std::move(target), 
        method_token.text, 
        std::move(args), 
        dot.location
    );
    return call;
}

std::vector<Argument> Parser::parse_arguments() {
    std::vector<Argument> args;
    
    if (check(TokenType::RParen)) {
        return args;
    }
    
    do {
        ExpressionPtr expr = parse_expression();
        if (expr) {
            args.emplace_back(std::move(expr), current_token().location);
        }
    } while (match({TokenType::Comma}));
    
    consume(TokenType::RParen, "Expected ')' after arguments");
    
    return args;
}

ExpressionPtr Parser::parse_expression() {
    return parse_primary();
}

// Check if an identifier starts with uppercase (for shorthand calls)
bool is_capitalized(const std::string& str) {
    return !str.empty() && std::isupper(static_cast<unsigned char>(str[0]));
}

ExpressionPtr Parser::parse_primary() {
    if (check(TokenType::Number)) {
        return std::make_unique<Literal>(parse_literal());
    }
    if (check(TokenType::String)) {
        return std::make_unique<Literal>(parse_literal());
    }
    if (check(TokenType::Color)) {
        return std::make_unique<Literal>(parse_literal());
    }
    if (check(TokenType::Percentage)) {
        return std::make_unique<Literal>(parse_literal());
    }
    if (check(TokenType::Identifier)) {
        // Check if it's a capitalized identifier followed by '(' - shorthand call
        if (is_capitalized(current_token().text) && peek_token().is(TokenType::LParen)) {
            return parse_shorthand_call();
        }
        
        auto ident = parse_identifier();
        ExpressionPtr expr = std::make_unique<Identifier>(std::move(ident));
        
        // Check for method call: identifier.method(args)
        // Check for visible.titlebar: window.visible.titlebar(true/false)
        // Check for relative position: object.direction(distance).reference_object
        while (check(TokenType::Dot)) {
            // Peek ahead to see what's after the dot
            if (peek_token().is(TokenType::Visible)) {
                expr = parse_visible_title_bar(std::move(expr));
            } else if (peek_token().is(TokenType::RelativeLeft) || peek_token().is(TokenType::RelativeRight) || 
                       peek_token().is(TokenType::RelativeTop) || peek_token().is(TokenType::RelativeBottom)) {
                expr = parse_relative_position(std::move(expr));
            } else {
                expr = parse_method_call(std::move(expr));
            }
        }
        
        return expr;
    }
    if (check(TokenType::LParen)) {
        error_at_current("Unexpected '(' in expression");
        current_++;
        return nullptr;
    }
    
    error_at_current("Expected expression");
    current_++;
    return nullptr;
}

ExpressionPtr Parser::parse_shorthand_call() {
    // Parse CapitalizedIdentifier(args)
    Token name_token = consume_identifier("Expected identifier");
    std::string name = name_token.text;
    
    // Consume '('
    consume(TokenType::LParen, "Expected '(' after shorthand name");
    
    std::vector<Argument> args = parse_arguments();
    
    auto call = std::make_unique<ShorthandCall>(name, std::move(args), name_token.location);
    return call;
}

ExpressionPtr Parser::parse_edge_position() {
    // Parse left, right, top, bottom with optional offset
    TokenType edge_type = current_token().type;
    Token edge_token = current_token();
    current_++;
    
    EdgePosition::Edge edge;
    switch (edge_type) {
        case TokenType::Left: edge = EdgePosition::Edge::Left; break;
        case TokenType::Right: edge = EdgePosition::Edge::Right; break;
        case TokenType::Top: edge = EdgePosition::Edge::Top; break;
        case TokenType::Bottom: edge = EdgePosition::Edge::Bottom; break;
        default:
            error_at_current("Expected edge position (left, right, top, bottom)");
            return nullptr;
    }
    
    ExpressionPtr offset = nullptr;
    if (check(TokenType::LParen)) {
        consume(TokenType::LParen, "Expected '(' after edge position");
        auto expr = parse_expression();
        offset = std::move(expr);
        consume(TokenType::RParen, "Expected ')' after edge position offset");
    }
    
    auto pos = std::make_unique<EdgePosition>(edge, std::move(offset), edge_token.location);
    return pos;
}

ExpressionPtr Parser::parse_corner_position() {
    // Parse topLeft, topRight, bottomLeft, bottomRight
    TokenType corner_type = current_token().type;
    Token corner_token = current_token();
    current_++;
    
    CornerPosition::Corner corner;
    switch (corner_type) {
        case TokenType::TopLeft: corner = CornerPosition::Corner::TopLeft; break;
        case TokenType::TopRight: corner = CornerPosition::Corner::TopRight; break;
        case TokenType::BottomLeft: corner = CornerPosition::Corner::BottomLeft; break;
        case TokenType::BottomRight: corner = CornerPosition::Corner::BottomRight; break;
        default:
            error_at_current("Expected corner position (topLeft, topRight, bottomLeft, bottomRight)");
            return nullptr;
    }
    
    auto pos = std::make_unique<CornerPosition>(corner, corner_token.location);
    return pos;
}

ExpressionPtr Parser::parse_relative_position(ExpressionPtr /*target*/) {
    // Parse relative position: object.direction(distance).reference_object
    // target is the object on the left side (e.g., hinh_tron in hinh_tron.left(10).hinh_chu_nhat)
    Token dot = consume(TokenType::Dot, "Expected '.'");
    Token dir_token = current_token();
    
    RelativePosition::Direction direction;
    if (check(TokenType::RelativeLeft)) {
        direction = RelativePosition::Direction::Left;
    } else if (check(TokenType::RelativeRight)) {
        direction = RelativePosition::Direction::Right;
    } else if (check(TokenType::RelativeTop)) {
        direction = RelativePosition::Direction::Top;
    } else if (check(TokenType::RelativeBottom)) {
        direction = RelativePosition::Direction::Bottom;
    } else {
        error_at_current("Expected relative direction (left, right, top, bottom)");
        return nullptr;
    }
    current_++; // consume direction token
    
    consume(TokenType::LParen, "Expected '(' after relative direction");
    ExpressionPtr distance = parse_expression();
    consume(TokenType::RParen, "Expected ')' after relative distance");
    
    consume(TokenType::Dot, "Expected '.' after relative position");
    Token ref_token = consume_identifier("Expected reference object identifier after '.'");
    std::string reference_object = ref_token.text;
    
    auto rel_pos = std::make_unique<RelativePosition>(direction, std::move(distance), std::move(reference_object), dot.location);
    return rel_pos;
}

ExpressionPtr Parser::parse_visible_title_bar(ExpressionPtr /*target*/) {
    // Parse window.visible.titlebar(true/false)
    consume(TokenType::Dot, "Expected '.'");
    consume(TokenType::Visible, "Expected 'visible'");
    consume(TokenType::Dot, "Expected '.'");
    consume(TokenType::TitleBar, "Expected 'titlebar'");
    consume(TokenType::LParen, "Expected '(' after 'titlebar'");
    
    ExpressionPtr value = parse_expression(); // Should be boolean literal
    consume(TokenType::RParen, "Expected ')' after titlebar value");
    
    auto vtb = std::make_unique<VisibleTitleBar>(std::move(value), current_token().location);
    return vtb;
}

Identifier Parser::parse_identifier() {
    Token token = consume_identifier("Expected identifier");
    return Identifier(token.text, token.location);
}

Literal Parser::parse_literal() {
    Token token = current_token();
    current_++;
    
    switch (token.type) {
        case TokenType::Number: {
            double value = token.as_number().value_or(0.0);
            return Literal::number(value, token.location);
        }
        case TokenType::String: {
            std::string value = token.as_string().value_or("");
            return Literal::string(std::move(value), token.location);
        }
        case TokenType::Color: {
            std::string value = token.as_string().value_or("");
            return Literal::color(std::move(value), token.location);
        }
        case TokenType::Percentage: {
            double value = token.as_number().value_or(0.0);
            return Literal::percentage(value, token.location);
        }
        default:
            error_at_previous("Expected literal");
            return Literal::number(0, token.location);
    }
}

std::unique_ptr<AutoDirective> Parser::parse_auto_directive() {
    Token directive_token = current_token();
    AutoDirective::Kind kind;
    
    if (check(TokenType::AutoSet)) {
        kind = AutoDirective::Kind::AutoSet;
        current_++;
    } else if (check(TokenType::AutoPick)) {
        kind = AutoDirective::Kind::AutoPick;
        current_++;
    } else {
        error_at_current("Expected #autoSet or #autoPick");
        return nullptr;
    }
    
    // Check for combined directive (invalid)
    if (check(TokenType::AutoSet) || check(TokenType::AutoPick)) {
        error_at_current("Cannot combine multiple auto directives");
    }
    
    // Expect '{'
    Token lbrace = consume(TokenType::LBrace, "Expected '{' after auto directive");
    
    std::vector<StatementPtr> body;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            // Mark object declarations in autoSet context
            if (kind == AutoDirective::Kind::AutoSet) {
                if (auto obj_decl = dynamic_cast<ObjectDecl*>(stmt.get())) {
                    obj_decl->is_auto_set_context = true;
                }
            }
            body.push_back(std::move(stmt));
        } else {
            if (is_at_end()) break;
            current_++;
        }
    }
    
    Token rbrace = consume(TokenType::RBrace, "Expected '}' to end auto directive block");
    
    auto directive = std::make_unique<AutoDirective>(kind, std::move(body), directive_token.location);
    directive->range = SourceRange{lbrace.location, rbrace.location};
    
    return directive;
}

std::unique_ptr<Program> Parser::parse() {
    return parse_program();
}

} // namespace fxl