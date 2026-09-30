#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <optional>

namespace fxl {

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(DiagnosticBag* diagnostics = nullptr);
    
    void analyze(Program& program);

private:
    DiagnosticBag* diagnostics_;
    
    // Scope management
    struct Scope {
        std::unordered_map<std::string, ObjectDecl*> variables;
        Scope* parent = nullptr;
        ObjectDecl* current_object = nullptr;  // For object scope tracking
        AutoDirective* current_auto_directive = nullptr;  // For auto directive scope
    };
    
    Scope* current_scope_ = nullptr;
    std::vector<std::unique_ptr<Scope>> owned_scopes_;
    
    // Global identifier tracking for duplicate detection across entire file
    std::unordered_set<std::string> declared_identifiers_;
    
    void enter_scope();
    void leave_scope();
    void declare_variable(const std::string& name, ObjectDecl* decl, SourceLocation location);
    ObjectDecl* lookup_variable(const std::string& name) const;
    
    // Analysis methods
    void analyze_statement(Statement& stmt);
    void analyze_import(ImportDecl& import);
    void analyze_object_decl(ObjectDecl& decl);
    void analyze_object_block(ObjectBlock& block, ObjectDecl* parent_object);
    void analyze_method_call(MethodCall& call);
    void analyze_shorthand_call(ShorthandCall& call);
    void analyze_auto_directive(AutoDirective& directive);
    void analyze_expression(Expression& expr);
    
    // Validation
    void validate_method_call(MethodCall& call, ObjectDecl* current_object);
    void validate_auto_directive(AutoDirective& directive);
    void check_combined_auto_directives(const AutoDirective& directive);
    
    // Error reporting
    void error(const std::string& message, SourceLocation location, const std::string& code = "");
    void warning(const std::string& message, SourceLocation location, const std::string& code = "");
};

} // namespace fxl