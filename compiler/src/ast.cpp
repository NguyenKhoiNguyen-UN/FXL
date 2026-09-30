#include "ast.hpp"
#include <sstream>

namespace fxl {

void accept(ASTNode& node, ASTVisitor& visitor) {
    (void)node;
    (void)visitor;
    // Base class does nothing - concrete types override
}

void accept(Program& node, ASTVisitor& visitor) {
    visitor.visit_program(node);
    for (auto& stmt : node.statements) {
        accept(*stmt, visitor);
    }
}

void accept(ImportDecl& node, ASTVisitor& visitor) {
    visitor.visit_import(node);
}

void accept(ObjectDecl& node, ASTVisitor& visitor) {
    visitor.visit_object_decl(node);
    if (node.block) {
        accept(*node.block, visitor);
    }
}

void accept(ObjectBlock& node, ASTVisitor& visitor) {
    visitor.visit_object_block(node);
    for (auto& stmt : node.statements) {
        accept(*stmt, visitor);
    }
}

void accept(MethodCall& node, ASTVisitor& visitor) {
    visitor.visit_method_call(node);
    if (node.target) {
        accept(*node.target, visitor);
    }
    for (auto& arg : node.arguments) {
        accept(arg, visitor);
    }
}

void accept(ShorthandCall& node, ASTVisitor& visitor) {
    visitor.visit_shorthand_call(node);
    for (auto& arg : node.arguments) {
        accept(arg, visitor);
    }
}

void accept(AutoDirective& node, ASTVisitor& visitor) {
    visitor.visit_auto_directive(node);
    for (auto& stmt : node.body) {
        accept(*stmt, visitor);
    }
}

void accept(EdgePosition& node, ASTVisitor& visitor) {
    visitor.visit_edge_position(node);
    if (node.offset) {
        accept(*node.offset, visitor);
    }
}

void accept(CornerPosition& node, ASTVisitor& visitor) {
    visitor.visit_corner_position(node);
}

void accept(RelativePosition& node, ASTVisitor& visitor) {
    visitor.visit_relative_position(node);
    if (node.distance) {
        accept(*node.distance, visitor);
    }
}

void accept(VisibleTitleBar& node, ASTVisitor& visitor) {
    visitor.visit_visible_title_bar(node);
    if (node.value) {
        accept(*node.value, visitor);
    }
}

void accept(ExpressionStatement& node, ASTVisitor& visitor) {
    visitor.visit_expression_statement(node);
    if (node.expression) {
        accept(*node.expression, visitor);
    }
}

void accept(Identifier& node, ASTVisitor& visitor) {
    visitor.visit_identifier(node);
}

void accept(Literal& node, ASTVisitor& visitor) {
    visitor.visit_literal(node);
}

void accept(Argument& node, ASTVisitor& visitor) {
    visitor.visit_argument(node);
    if (node.expression) {
        accept(*node.expression, visitor);
    }
}

std::string ast_to_string(const Literal& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    switch (node.kind) {
        case Literal::Kind::Number:
            oss << prefix << "NumberLiteral(" << std::get<double>(node.value) << ")";
            break;
        case Literal::Kind::String:
            oss << prefix << "StringLiteral(\"" << std::get<std::string>(node.value) << "\")";
            break;
        case Literal::Kind::Color:
            oss << prefix << "ColorLiteral(" << std::get<std::string>(node.value) << ")";
            break;
        case Literal::Kind::Percentage:
            oss << prefix << "PercentageLiteral(" << std::get<double>(node.value) << "%)";
            break;
    }
    return oss.str();
}

std::string ast_to_string(const Identifier& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "Identifier(" << node.name << ")";
    return oss.str();
}

std::string ast_to_string(const Argument& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "Argument:\n";
    if (node.expression) {
        oss << ast_to_string(*node.expression, indent + 2);
    }
    return oss.str();
}

std::string ast_to_string(const ExpressionStatement& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "ExpressionStatement:\n";
    if (node.expression) {
        oss << ast_to_string(*node.expression, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const MethodCall& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "MethodCall(" << node.method_name << "):\n";
    if (node.target) {
        oss << ast_to_string(*node.target, indent + 2) << "\n";
    }
    for (const auto& arg : node.arguments) {
        oss << ast_to_string(arg, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const ShorthandCall& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "ShorthandCall(" << node.name << "):\n";
    for (const auto& arg : node.arguments) {
        oss << ast_to_string(arg, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const ObjectBlock& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "ObjectBlock:\n";
    for (const auto& stmt : node.statements) {
        oss << ast_to_string(*stmt, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const ObjectDecl& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "ObjectDecl(" << node.name << " : " << node.type_name;
    if (node.is_auto_set_context) oss << " [autoSet]";
    oss << "):\n";
    if (node.block) {
        oss << ast_to_string(*node.block, indent + 2);
    }
    return oss.str();
}

std::string ast_to_string(const ImportDecl& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "Import(" << node.module_name << ")";
    return oss.str();
}

std::string ast_to_string(const AutoDirective& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << (node.kind == AutoDirective::Kind::AutoSet ? "AutoSet" : "AutoPick") << ":\n";
    for (const auto& stmt : node.body) {
        oss << ast_to_string(*stmt, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const EdgePosition& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    std::string edge_str;
    switch (node.edge) {
        case EdgePosition::Edge::Left: edge_str = "Left"; break;
        case EdgePosition::Edge::Right: edge_str = "Right"; break;
        case EdgePosition::Edge::Top: edge_str = "Top"; break;
        case EdgePosition::Edge::Bottom: edge_str = "Bottom"; break;
    }
    oss << prefix << "EdgePosition(" << edge_str;
    if (node.offset) {
        oss << " offset=";
        if (auto lit = dynamic_cast<Literal*>(node.offset.get())) {
            if (lit->kind == Literal::Kind::Number) {
                oss << std::get<double>(lit->value);
            }
        }
    }
    oss << "):\n";
    if (node.offset) {
        oss << ast_to_string(*node.offset, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const CornerPosition& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    std::string corner_str;
    switch (node.corner) {
        case CornerPosition::Corner::TopLeft: corner_str = "TopLeft"; break;
        case CornerPosition::Corner::TopRight: corner_str = "TopRight"; break;
        case CornerPosition::Corner::BottomLeft: corner_str = "BottomLeft"; break;
        case CornerPosition::Corner::BottomRight: corner_str = "BottomRight"; break;
    }
    oss << prefix << "CornerPosition(" << corner_str << "):\n";
    return oss.str();
}

std::string ast_to_string(const RelativePosition& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    std::string dir_str;
    switch (node.direction) {
        case RelativePosition::Direction::Left: dir_str = "Left"; break;
        case RelativePosition::Direction::Right: dir_str = "Right"; break;
        case RelativePosition::Direction::Top: dir_str = "Top"; break;
        case RelativePosition::Direction::Bottom: dir_str = "Bottom"; break;
    }
    oss << prefix << "RelativePosition(" << dir_str << " distance=";
    if (node.distance) {
        if (auto lit = dynamic_cast<Literal*>(node.distance.get())) {
            if (lit->kind == Literal::Kind::Number) {
                oss << std::get<double>(lit->value);
            }
        }
    }
    oss << " ref=" << node.reference_object << "):\n";
    if (node.distance) {
        oss << ast_to_string(*node.distance, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const VisibleTitleBar& node, int indent) {
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "VisibleTitleBar";
    if (node.value) {
        if (auto lit = dynamic_cast<Literal*>(node.value.get())) {
            if (lit->kind == Literal::Kind::Number) {
                oss << "(" << (std::get<double>(lit->value) != 0 ? "true" : "false") << ")";
            }
        }
    }
    oss << ":\n";
    if (node.value) {
        oss << ast_to_string(*node.value, indent + 2) << "\n";
    }
    return oss.str();
}

std::string ast_to_string(const ASTNode& node, int indent) {
    (void)node;
    std::ostringstream oss;
    std::string prefix(static_cast<size_t>(indent), ' ');
    oss << prefix << "ASTNode";
    return oss.str();
}

std::string ast_to_string(const Program& program) {
    std::ostringstream oss;
    oss << "Program:\n";
    for (const auto& stmt : program.statements) {
        oss << ast_to_string(*stmt, 2) << "\n";
    }
    return oss.str();
}

} // namespace fxl