#include "auto_resolver.hpp"
#include <algorithm>

namespace fxl {

const std::unordered_map<std::string, std::string> AutoSetResolver::SETTER_MAPPING = {
    {"Size", "setSize"},
    {"Color", "setColor"},
    {"Opacity", "setOpacity"},
    {"Position", "setPosition"},
    {"Text", "setText"}
};

AutoSetResolver::AutoSetResolver(DiagnosticBag* diagnostics)
    : diagnostics_(diagnostics) {}

void AutoSetResolver::resolve(Program& program) {
    resolve_program(program);
}

void AutoSetResolver::resolve_program(Program& program) {
    for (auto& stmt : program.statements) {
        resolve_statement(*stmt);
    }
}

void AutoSetResolver::resolve_statement(Statement& stmt) {
    if (auto obj_decl = dynamic_cast<ObjectDecl*>(&stmt)) {
        resolve_object_decl(*obj_decl);
    } else if (auto expr_stmt = dynamic_cast<ExpressionStatement*>(&stmt)) {
        if (expr_stmt->expression) {
            resolve_expression(*expr_stmt->expression);
        }
    } else if (auto method_call = dynamic_cast<MethodCall*>(&stmt)) {
        resolve_method_call(*method_call);
    } else if (auto shorthand = dynamic_cast<ShorthandCall*>(&stmt)) {
        resolve_shorthand_call(*shorthand);
    } else if (auto auto_dir = dynamic_cast<AutoDirective*>(&stmt)) {
        resolve_auto_directive(*auto_dir);
    }
}

void AutoSetResolver::resolve_expression(Expression& expr) {
    if (auto method_call = dynamic_cast<MethodCall*>(&expr)) {
        resolve_method_call(*method_call);
    } else if (auto shorthand = dynamic_cast<ShorthandCall*>(&expr)) {
        resolve_shorthand_call(*shorthand);
    }
}

void AutoSetResolver::resolve_object_decl(ObjectDecl& decl) {
    // Push current object onto stack
    ObjectDecl* prev_object = context_.current_object;
    context_.current_object = &decl;
    context_.object_stack.push_back(&decl);
    
    if (decl.block) {
        resolve_object_block(*decl.block);
    }
    
    // Pop
    context_.object_stack.pop_back();
    context_.current_object = prev_object;
}

void AutoSetResolver::resolve_object_block(ObjectBlock& block) {
    for (auto& stmt : block.statements) {
        resolve_statement(*stmt);
    }
}

void AutoSetResolver::resolve_method_call(MethodCall& call) {
    // Resolve arguments
    for (auto& arg : call.arguments) {
        if (arg.expression) {
            // Arguments could contain nested expressions that need resolution
            // For now, literals don't need resolution
        }
    }
}

void AutoSetResolver::resolve_shorthand_call(ShorthandCall& call) {
    // Only resolve if we're in an autoSet context
    if (context_.current_auto_directive && 
        context_.current_auto_directive->kind == AutoDirective::Kind::AutoSet) {
        
        auto it = SETTER_MAPPING.find(call.name);
        if (it != SETTER_MAPPING.end()) {
            if (context_.current_object) {
                // Mark as resolved - code generator will use this
                call.resolved_setter = it->second;
                call.resolved_target = context_.current_object->name;
            } else {
                error("Auto-set shorthand '" + call.name + "' used outside object context",
                      call.location, "E_AUTOSET_NO_CONTEXT");
            }
        } else {
            error("Unknown auto-set operation '" + call.name + "'",
                  call.location, "E_UNKNOWN_AUTOSET_OP");
        }
    }
    // If not in autoSet context, leave as-is (will be handled as error in semantic or codegen)
}

void AutoSetResolver::resolve_auto_directive(AutoDirective& directive) {
    AutoDirective* prev_auto = context_.current_auto_directive;
    context_.current_auto_directive = &directive;
    
    if (directive.kind == AutoDirective::Kind::AutoSet) {
        for (auto& stmt : directive.body) {
            resolve_statement(*stmt);
        }
    } else {
        // AutoPick - traverse for nested directives
        for (auto& stmt : directive.body) {
            if (auto nested_auto = dynamic_cast<AutoDirective*>(stmt.get())) {
                resolve_auto_directive(*nested_auto);
            }
        }
    }
    
    context_.current_auto_directive = prev_auto;
}

std::string AutoSetResolver::resolve_setter_name(const std::string& shorthand) const {
    auto it = SETTER_MAPPING.find(shorthand);
    if (it != SETTER_MAPPING.end()) {
        return it->second;
    }
    return "";
}

void AutoSetResolver::error(const std::string& message, SourceLocation location, const std::string& code) {
    if (diagnostics_) {
        diagnostics_->add_error(message, location, code);
    }
}

} // namespace fxl