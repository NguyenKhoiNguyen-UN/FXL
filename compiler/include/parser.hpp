#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include "diagnostic.hpp"
#include <vector>
#include <memory>

namespace fxl {

class Parser {
public:
    Parser(const std::vector<Token>& tokens, DiagnosticBag* diagnostics = nullptr);
    
    std::unique_ptr<Program> parse();

private:
    const std::vector<Token>& tokens_;
    DiagnosticBag* diagnostics_;
    size_t current_ = 0;

    // Token access
    const Token& current_token() const;
    const Token& peek_token(size_t offset = 1) const;
    const Token& previous_token() const;
    bool is_at_end() const;
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& error_message);
    Token consume_identifier(const std::string& error_message);

    // Parsing methods
    std::unique_ptr<Program> parse_program();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<ImportDecl> parse_import();
    std::unique_ptr<ObjectDecl> parse_object_decl();
    std::unique_ptr<ObjectBlock> parse_object_block();
    ExpressionPtr parse_method_call(ExpressionPtr target);
    std::unique_ptr<AutoDirective> parse_auto_directive();
    std::vector<Argument> parse_arguments();
    ExpressionPtr parse_expression();
    ExpressionPtr parse_primary();
    ExpressionPtr parse_shorthand_call();
    ExpressionPtr parse_edge_position();
    ExpressionPtr parse_corner_position();
    ExpressionPtr parse_relative_position(ExpressionPtr target);
    ExpressionPtr parse_visible_title_bar(ExpressionPtr target);
    Identifier parse_identifier();
    Literal parse_literal();

    // Error handling
    void error(const std::string& message, SourceLocation location);
    void error_at_current(const std::string& message);
    void error_at_previous(const std::string& message);
    std::unique_ptr<Statement> synchronize();
};

} // namespace fxl