#include "codegen.hpp"
#include "ast.hpp"
#include <sstream>
#include <unordered_map>

namespace fxl {

CodeGenerator::CodeGenerator(DiagnosticBag* diagnostics)
    : diagnostics_(diagnostics) {}

std::string CodeGenerator::generate(const Program& program) {
    output_.str("");
    output_.clear();
    indent_level_ = 0;
    generate_program(program);
    return output_.str();
}

void CodeGenerator::generate(const Program& program, std::ostream& out) {
    out << generate(program);
}

void CodeGenerator::indent() {
    indent_level_++;
}

void CodeGenerator::dedent() {
    if (indent_level_ > 0) indent_level_--;
}

void CodeGenerator::write_indent() {
    for (int i = 0; i < indent_level_; ++i) {
        output_ << "    ";
    }
}

void CodeGenerator::write_line(const std::string& line) {
    write_indent();
    output_ << line << "\n";
}

void CodeGenerator::write(const std::string& text) {
    output_ << text;
}

void CodeGenerator::generate_program(const Program& program) {
    // Generate includes
    write_line("#include <memory>");
    write_line("#include <string>");
    write_line("#include <vector>");
    write_line("#include <fxl/runtime.hpp>");
    write_line("");
    
    // Generate main function
    write_line("int main() {");
    indent();
    
    // Generate each statement inside main
    for (const auto& stmt : program.statements) {
        generate_statement(*stmt);
    }
    
    write_line("return 0;");
    dedent();
    write_line("}");
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto import = dynamic_cast<const ImportDecl*>(&stmt)) {
        generate_import(*import);
    } else if (auto obj_decl = dynamic_cast<const ObjectDecl*>(&stmt)) {
        generate_object_decl(*obj_decl);
    } else if (auto expr_stmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        if (expr_stmt->expression) {
            generate_expression_statement(*expr_stmt);
        }
    } else if (auto method_call = dynamic_cast<const MethodCall*>(&stmt)) {
        generate_method_call(*method_call);
    } else if (auto shorthand = dynamic_cast<const ShorthandCall*>(&stmt)) {
        generate_shorthand_call(*shorthand);
    } else if (auto auto_dir = dynamic_cast<const AutoDirective*>(&stmt)) {
        generate_auto_directive(*auto_dir);
    }
}

void CodeGenerator::generate_import(const ImportDecl& import) {
    // For now, imports are comments in generated code
    write_line("// import " + import.module_name);
}

void CodeGenerator::generate_object_decl(const ObjectDecl& decl) {
    std::string cpp_type = map_type_to_cpp(decl.type_name);
    std::string var_name = decl.name;
    
    // Avoid variable name conflicting with type name
    if (var_name == cpp_type) {
        var_name = cpp_type + "1";
    }
    
    if (decl.block) {
        // Object with block: create object, then configure
        write_line("auto " + var_name + " = " + cpp_type + "();");
        generate_object_block(*decl.block, var_name);
    } else {
        // Simple object creation
        write_line("auto " + var_name + " = " + cpp_type + "();");
    }
}

void CodeGenerator::generate_object_block(const ObjectBlock& block, const std::string& parent_var) {
    for (const auto& stmt : block.statements) {
        if (auto obj_decl = dynamic_cast<const ObjectDecl*>(stmt.get())) {
            // Nested object
            std::string cpp_type = map_type_to_cpp(obj_decl->type_name);
            std::string var_name = obj_decl->name;
            // Avoid variable name conflicting with type name
            if (var_name == cpp_type) {
                var_name = cpp_type + "1";
            }
            write_line("auto " + var_name + " = " + cpp_type + "();");
            if (obj_decl->block) {
                generate_object_block(*obj_decl->block, var_name);
            }
            // Add to parent
            write_line(parent_var + ".add(" + var_name + ");");
        } else if (auto expr_stmt = dynamic_cast<const ExpressionStatement*>(stmt.get())) {
            // Expression statement (method call or shorthand call)
            if (expr_stmt->expression) {
                if (auto method_call = dynamic_cast<const MethodCall*>(expr_stmt->expression.get())) {
                    generate_method_call(*method_call);
                } else if (auto shorthand = dynamic_cast<const ShorthandCall*>(expr_stmt->expression.get())) {
                    generate_shorthand_call(*shorthand);
                } else if (auto vtb = dynamic_cast<const VisibleTitleBar*>(expr_stmt->expression.get())) {
                    generate_visible_title_bar(*vtb, parent_var);
                }
            }
        } else if (auto method_call = dynamic_cast<const MethodCall*>(stmt.get())) {
            // Method call on parent or nested object
            generate_method_call(*method_call);
        } else if (auto shorthand = dynamic_cast<const ShorthandCall*>(stmt.get())) {
            // Resolved shorthand call
            generate_shorthand_call(*shorthand);
        } else if (auto auto_dir = dynamic_cast<const AutoDirective*>(stmt.get())) {
            // Nested auto directive - generate its body
            generate_auto_directive(*auto_dir);
        }
    }
}

void CodeGenerator::generate_method_call(const MethodCall& call) {
    std::string target_str;
    
    // Generate target expression
    if (auto ident = dynamic_cast<const Identifier*>(call.target.get())) {
        target_str = ident->name;
    } else {
        // For other expression types, we'd need to generate them
        // For now, assume identifier
        target_str = "/*unknown*/";
    }
    
    // Generate arguments
    std::ostringstream args_stream;
    for (size_t i = 0; i < call.arguments.size(); ++i) {
        if (i > 0) args_stream << ", ";
        const auto& arg = call.arguments[i];
        if (arg.expression) {
            generate_argument_value(arg.expression.get(), args_stream);
        }
    }
    
    write_line(target_str + "." + call.method_name + "(" + args_stream.str() + ");");
}

void CodeGenerator::generate_shorthand_call(const ShorthandCall& call) {
    if (!call.resolved_setter.empty() && !call.resolved_target.empty()) {
        // This was resolved by AutoSetResolver
        std::ostringstream args_stream;
        for (size_t i = 0; i < call.arguments.size(); ++i) {
            if (i > 0) args_stream << ", ";
            const auto& arg = call.arguments[i];
            if (arg.expression) {
                generate_argument_value(arg.expression.get(), args_stream);
            }
        }
        write_line(call.resolved_target + "." + call.resolved_setter + "(" + args_stream.str() + ");");
    } else {
        // Not resolved - error or leave as comment
        write_line("// Unresolved shorthand: " + call.name + "(...)");
    }
}

void CodeGenerator::generate_auto_directive(const AutoDirective& directive) {
    if (directive.kind == AutoDirective::Kind::AutoSet) {
        write_line("// #autoSet block");
        for (const auto& stmt : directive.body) {
            generate_statement(*stmt);
        }
    } else {
        write_line("// #autoPick block (not implemented)");
        for (const auto& stmt : directive.body) {
            generate_statement(*stmt);
        }
    }
}

void CodeGenerator::generate_expression_statement(const ExpressionStatement& stmt) {
    if (stmt.expression) {
        if (auto method_call = dynamic_cast<const MethodCall*>(stmt.expression.get())) {
            generate_method_call(*method_call);
        } else if (auto shorthand = dynamic_cast<const ShorthandCall*>(stmt.expression.get())) {
            generate_shorthand_call(*shorthand);
        } else if (auto vtb = dynamic_cast<const VisibleTitleBar*>(stmt.expression.get())) {
            generate_visible_title_bar(*vtb, "window"); // TODO: track actual window variable
        }
    }
}

void CodeGenerator::generate_argument_value(const Expression* expr, std::ostringstream& out) {
    if (auto literal = dynamic_cast<const Literal*>(expr)) {
        switch (literal->kind) {
            case Literal::Kind::Number:
                out << std::get<double>(literal->value);
                break;
            case Literal::Kind::String:
                out << "\"" << std::get<std::string>(literal->value) << "\"";
                break;
            case Literal::Kind::Color:
                out << "\"" << std::get<std::string>(literal->value) << "\"";
                break;
            case Literal::Kind::Percentage:
                out << std::get<double>(literal->value);
                break;
        }
    } else if (auto ident = dynamic_cast<const Identifier*>(expr)) {
        out << ident->name;
    }
}

void CodeGenerator::generate_visible_title_bar(const VisibleTitleBar& vtb, const std::string& target_var) {
    // Generate window.visible.titlebar(value) call on the target variable
    if (vtb.value) {
        if (auto literal = dynamic_cast<const Literal*>(vtb.value.get())) {
            if (literal->kind == Literal::Kind::Number) {
                bool visible = std::get<double>(literal->value) != 0;
                write_line(target_var + ".visible.titlebar(" + std::string(visible ? "true" : "false") + ");");
            }
        }
    }
}

void CodeGenerator::generate_expression([[maybe_unused]] const Expression& expr) {
    // Not used directly in current implementation
}

void CodeGenerator::generate_identifier([[maybe_unused]] const Identifier& ident) {
    // Not used directly
}

void CodeGenerator::generate_literal([[maybe_unused]] const Literal& literal) {
    // Not used directly
}

void CodeGenerator::generate_arguments([[maybe_unused]] const std::vector<Argument>& args) {
    // Not used directly
}

std::string CodeGenerator::map_type_to_cpp(const std::string& fxl_type) const {
    // Map FXL types to C++ classes (fully qualified)
    if (fxl_type == "Window") return "fxl::runtime::Window";
    if (fxl_type == "Button") return "fxl::runtime::Button";
    if (fxl_type == "Object") return "fxl::runtime::Object";
    if (fxl_type == "Panel") return "fxl::runtime::Panel";
    if (fxl_type == "rectangle") return "fxl::runtime::rectangle";
    return fxl_type;  // Default to same name
}

void CodeGenerator::error(const std::string& message, SourceLocation location, const std::string& code) {
    if (diagnostics_) {
        diagnostics_->add_error(message, location, code);
    }
}

} // namespace fxl