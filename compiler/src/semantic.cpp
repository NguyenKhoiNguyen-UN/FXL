#include "semantic.hpp"
#include <algorithm>

namespace fxl {

SemanticAnalyzer::SemanticAnalyzer(DiagnosticBag* diagnostics)
    : diagnostics_(diagnostics) {}

void SemanticAnalyzer::enter_scope() {
    auto new_scope = std::make_unique<Scope>();
    new_scope->parent = current_scope_;
    current_scope_ = new_scope.get();
    owned_scopes_.push_back(std::move(new_scope));
}

void SemanticAnalyzer::leave_scope() {
    if (current_scope_) {
        current_scope_ = current_scope_->parent;
    }
}

void SemanticAnalyzer::declare_variable(const std::string& name, ObjectDecl* decl, SourceLocation location) {
    // Check for duplicate identifier across entire file (not just current scope)
    if (declared_identifiers_.count(name)) {
        error("Identifier '" + name + "' already declared in this file", location, "E_DUPLICATE_IDENTIFIER");
        return;
    }
    
    if (current_scope_ && current_scope_->variables.count(name)) {
        error("Variable '" + name + "' already declared in this scope", location, "E_REDECLARE");
        return;
    }
    if (current_scope_) {
        current_scope_->variables[name] = decl;
    }
    
    // Track globally for duplicate detection
    declared_identifiers_.insert(name);
}

ObjectDecl* SemanticAnalyzer::lookup_variable(const std::string& name) const {
    Scope* scope = current_scope_;
    while (scope) {
        auto it = scope->variables.find(name);
        if (it != scope->variables.end()) {
            return it->second;
        }
        scope = scope->parent;
    }
    return nullptr;
}

void SemanticAnalyzer::analyze(Program& program) {
    enter_scope();  // Global scope
    for (auto& stmt : program.statements) {
        analyze_statement(*stmt);
    }
    leave_scope();
}

void SemanticAnalyzer::analyze_statement(Statement& stmt) {
    if (auto import = dynamic_cast<ImportDecl*>(&stmt)) {
        analyze_import(*import);
    } else if (auto obj_decl = dynamic_cast<ObjectDecl*>(&stmt)) {
        analyze_object_decl(*obj_decl);
    } else if (auto auto_dir = dynamic_cast<AutoDirective*>(&stmt)) {
        analyze_auto_directive(*auto_dir);
    } else if (auto expr_stmt = dynamic_cast<ExpressionStatement*>(&stmt)) {
        // Expression statement - analyze the wrapped expression
        if (expr_stmt->expression) {
            analyze_expression(*expr_stmt->expression);
        }
    } else if (auto method_call = dynamic_cast<MethodCall*>(&stmt)) {
        // Top-level method call - treat as expression statement
        analyze_method_call(*method_call);
    } else if (auto shorthand = dynamic_cast<ShorthandCall*>(&stmt)) {
        // Top-level shorthand call - will be resolved later
        analyze_shorthand_call(*shorthand);
    }
}

void SemanticAnalyzer::analyze_import([[maybe_unused]] ImportDecl& import) {
    // For now, just accept all imports
    // Could validate against known modules later
}

void SemanticAnalyzer::analyze_object_decl(ObjectDecl& decl) {
    // Declare the variable in current scope
    declare_variable(decl.name, &decl, decl.location);
    
    // If this object has a block, analyze it with this object as current
    if (decl.block) {
        ObjectDecl* prev_object = current_scope_ ? current_scope_->current_object : nullptr;
        if (current_scope_) {
            current_scope_->current_object = &decl;
        }
        analyze_object_block(*decl.block, &decl);
        if (current_scope_) {
            current_scope_->current_object = prev_object;
        }
    }
}

void SemanticAnalyzer::analyze_object_block(ObjectBlock& block, ObjectDecl* parent_object) {
    for (auto& stmt : block.statements) {
        if (auto obj_decl = dynamic_cast<ObjectDecl*>(stmt.get())) {
            // Nested object declaration
            analyze_object_decl(*obj_decl);
        } else if (auto expr_stmt = dynamic_cast<ExpressionStatement*>(stmt.get())) {
            // Expression statement (method call or shorthand call)
            if (expr_stmt->expression) {
                analyze_expression(*expr_stmt->expression);
            }
        } else if (auto method_call = dynamic_cast<MethodCall*>(stmt.get())) {
            // Method call inside object block
            analyze_method_call(*method_call);
        } else if (auto shorthand = dynamic_cast<ShorthandCall*>(stmt.get())) {
            // Shorthand call inside object block
            analyze_shorthand_call(*shorthand);
        } else if (auto auto_dir = dynamic_cast<AutoDirective*>(stmt.get())) {
            // Nested auto directive
            analyze_auto_directive(*auto_dir);
        }
    }
    (void)parent_object;  // Suppress unused parameter warning
}

void SemanticAnalyzer::analyze_method_call(MethodCall& call) {
    // Get current object from scope
    ObjectDecl* current_object = current_scope_ ? current_scope_->current_object : nullptr;
    validate_method_call(call, current_object);
    
    // Analyze arguments
    for (auto& arg : call.arguments) {
        if (arg.expression) {
            analyze_expression(*arg.expression);
        }
    }
}

void SemanticAnalyzer::analyze_shorthand_call(ShorthandCall& call) {
    // Shorthand calls are resolved in AutoSetResolver
    // Here we just analyze arguments
    
    // Analyze arguments
    for (auto& arg : call.arguments) {
        if (arg.expression) {
            analyze_expression(*arg.expression);
        }
    }
}

void SemanticAnalyzer::validate_method_call(MethodCall& call, ObjectDecl* current_object) {
    // Check if target is an identifier
    Identifier* target_ident = nullptr;
    if (auto ident = dynamic_cast<Identifier*>(call.target.get())) {
        target_ident = ident;
    }
    
    if (!target_ident) {
        error("Method call target must be an identifier", call.location, "E_INVALID_TARGET");
        return;
    }
    
    // Look up the target variable
    ObjectDecl* target_decl = lookup_variable(target_ident->name);
    if (!target_decl) {
        error("Unknown object '" + target_ident->name + "'", call.location, "E_UNKNOWN_OBJECT");
        return;
    }
    
    // In object blocks, method calls should typically be on the current object
    // or a parent object
    if (current_object && target_decl != current_object) {
        // Check if it's a parent object - simplified for now
    }
}

void SemanticAnalyzer::analyze_auto_directive(AutoDirective& directive) {
    // Check for combined auto directives (parser should catch this, but double-check)
    // This is a safety check
    
    // Save previous auto directive
    AutoDirective* prev_auto = current_scope_ ? current_scope_->current_auto_directive : nullptr;
    if (current_scope_) {
        current_scope_->current_auto_directive = &directive;
    }
    
    // NOTE: Do NOT enter a new scope for auto directive body
    // Variables declared inside #autoSet should be accessible outside
    
    for (auto& stmt : directive.body) {
        if (auto obj_decl = dynamic_cast<ObjectDecl*>(stmt.get())) {
            // Mark as being in autoSet context
            obj_decl->is_auto_set_context = (directive.kind == AutoDirective::Kind::AutoSet);
            analyze_object_decl(*obj_decl);
        } else if (auto method_call = dynamic_cast<MethodCall*>(stmt.get())) {
            analyze_method_call(*method_call);
        } else if (auto shorthand = dynamic_cast<ShorthandCall*>(stmt.get())) {
            analyze_shorthand_call(*shorthand);
        } else if (auto nested_auto = dynamic_cast<AutoDirective*>(stmt.get())) {
            // For nested auto directives, we DO enter a new auto directive context
            // but still not a new variable scope
            analyze_auto_directive(*nested_auto);
        }
    }
    
    if (current_scope_) {
        current_scope_->current_auto_directive = prev_auto;
    }
}

void SemanticAnalyzer::analyze_expression(Expression& expr) {
    if (auto literal = dynamic_cast<Literal*>(&expr)) {
        // Literals are self-contained
        (void)literal;
    } else if (auto ident = dynamic_cast<Identifier*>(&expr)) {
        // Could validate identifier exists
        (void)ident;
    } else if (auto method_call = dynamic_cast<MethodCall*>(&expr)) {
        analyze_method_call(*method_call);
    } else if (auto shorthand = dynamic_cast<ShorthandCall*>(&expr)) {
        analyze_shorthand_call(*shorthand);
    }
}

void SemanticAnalyzer::error(const std::string& message, SourceLocation location, const std::string& code) {
    if (diagnostics_) {
        diagnostics_->add_error(message, location, code);
    }
}

void SemanticAnalyzer::warning(const std::string& message, SourceLocation location, const std::string& code) {
    if (diagnostics_) {
        diagnostics_->add_warning(message, location, code);
    }
}

} // namespace fxl