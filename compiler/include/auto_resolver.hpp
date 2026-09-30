#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace fxl {

class AutoSetResolver {
public:
    explicit AutoSetResolver(DiagnosticBag* diagnostics = nullptr);
    
    void resolve(Program& program);

private:
    DiagnosticBag* diagnostics_;
    
    // AutoSet mapping
    static const std::unordered_map<std::string, std::string> SETTER_MAPPING;
    
    // Resolution state
    struct ResolutionContext {
        ObjectDecl* current_object = nullptr;
        AutoDirective* current_auto_directive = nullptr;
        std::vector<ObjectDecl*> object_stack;  // For nested objects
    };
    
    ResolutionContext context_;
    
    void resolve_program(Program& program);
    void resolve_statement(Statement& stmt);
    void resolve_expression(Expression& expr);
    void resolve_object_decl(ObjectDecl& decl);
    void resolve_object_block(ObjectBlock& block);
    void resolve_method_call(MethodCall& call);
    void resolve_shorthand_call(ShorthandCall& call);
    void resolve_auto_directive(AutoDirective& directive);
    
    // Core resolution logic
    bool try_resolve_auto_set(MethodCall& call);
    std::string resolve_setter_name(const std::string& shorthand) const;
    void replace_with_setter_call(MethodCall& call, const std::string& setter_name);
    
    void error(const std::string& message, SourceLocation location, const std::string& code = "");
};

} // namespace fxl