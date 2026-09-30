#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"
#include <string>
#include <ostream>
#include <vector>
#include <memory>
#include <sstream>

namespace fxl {

class CodeGenerator {
public:
    explicit CodeGenerator(DiagnosticBag* diagnostics = nullptr);
    
    // Generate C++ code from resolved AST
    std::string generate(const Program& program);
    
    // Generate to output stream
    void generate(const Program& program, std::ostream& out);

private:
    DiagnosticBag* diagnostics_;
    std::ostringstream output_;
    int indent_level_ = 0;
    
    // Indentation
    void indent();
    void dedent();
    void write_indent();
    void write_line(const std::string& line = "");
    void write(const std::string& text);
    
    // Generation methods
    void generate_program(const Program& program);
    void generate_statement(const Statement& stmt);
    void generate_import(const ImportDecl& import);
    void generate_object_decl(const ObjectDecl& decl);
    void generate_object_block(const ObjectBlock& block, const std::string& parent_var);
    void generate_method_call(const MethodCall& call);
    void generate_shorthand_call(const ShorthandCall& call);
    void generate_auto_directive(const AutoDirective& directive);
    void generate_expression_statement(const ExpressionStatement& stmt);
    void generate_visible_title_bar(const VisibleTitleBar& vtb, const std::string& target_var);
    void generate_expression(const Expression& expr);
    void generate_identifier(const Identifier& ident);
    void generate_literal(const Literal& literal);
    void generate_arguments(const std::vector<Argument>& args);
    void generate_argument_value(const Expression* expr, std::ostringstream& out);
    
    // Type mapping for C++ generation
    std::string map_type_to_cpp(const std::string& fxl_type) const;
    
    // Error handling
    void error(const std::string& message, SourceLocation location, const std::string& code = "");
};

} // namespace fxl