#pragma once

#include "token.hpp"
#include "diagnostic.hpp"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace fxl {

// Forward declarations
struct ASTNode;
struct Expression;
struct Statement;
struct ImportDecl;
struct ObjectDecl;
struct ObjectBlock;
struct MethodCall;
struct ShorthandCall;
struct Argument;
struct AutoDirective;
struct Identifier;
struct Literal;
struct EdgePosition;
struct CornerPosition;
struct RelativePosition;
struct VisibleTitleBar;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;

struct ASTNode {
    SourceLocation location;
    SourceRange range;

    ASTNode() = default;
    explicit ASTNode(SourceLocation loc) : location(loc), range(loc, loc) {}
    ASTNode(SourceLocation loc, SourceRange r) : location(loc), range(r) {}
    virtual ~ASTNode() = default;
};

struct Expression : ASTNode {
    Expression() = default;
    explicit Expression(SourceLocation loc) : ASTNode(loc) {}
    Expression(SourceLocation loc, SourceRange r) : ASTNode(loc, r) {}
    virtual ~Expression() = default;
};

struct Statement : ASTNode {
    Statement() = default;
    explicit Statement(SourceLocation loc) : ASTNode(loc) {}
    Statement(SourceLocation loc, SourceRange r) : ASTNode(loc, r) {}
    virtual ~Statement() = default;
};

// ExpressionStatement wraps an expression as a statement
struct ExpressionStatement : Statement {
    ExpressionPtr expression;

    ExpressionStatement() = default;
    ExpressionStatement(ExpressionPtr expr_, SourceLocation loc)
        : Statement(loc), expression(std::move(expr_)) {}
};

struct Identifier : Expression {
    std::string name;

    Identifier() = default;
    Identifier(std::string name_, SourceLocation loc) : Expression(loc), name(std::move(name_)) {}
};

struct Literal : Expression {
    enum class Kind {
        Number,
        String,
        Color,
        Percentage
    } kind;

    std::variant<double, std::string> value;

    Literal() = default;
    Literal(Kind kind_, std::variant<double, std::string> value_, SourceLocation loc)
        : Expression(loc), kind(kind_), value(std::move(value_)) {}
    
    static Literal number(double value_, SourceLocation loc) {
        return Literal(Kind::Number, value_, loc);
    }
    static Literal string(std::string value_, SourceLocation loc) {
        return Literal(Kind::String, std::move(value_), loc);
    }
    static Literal color(std::string value_, SourceLocation loc) {
        return Literal(Kind::Color, std::move(value_), loc);
    }
    static Literal percentage(double value_, SourceLocation loc) {
        return Literal(Kind::Percentage, value_, loc);
    }
};

struct Argument : ASTNode {
    ExpressionPtr expression;

    Argument() = default;
    Argument(ExpressionPtr expr_, SourceLocation loc) : ASTNode(loc), expression(std::move(expr_)) {}
};

struct MethodCall : Expression {
    ExpressionPtr target;
    std::string method_name;
    std::vector<Argument> arguments;

    MethodCall() = default;
    MethodCall(ExpressionPtr target_, std::string method_, std::vector<Argument> args_, SourceLocation loc)
        : Expression(loc), target(std::move(target_)), method_name(std::move(method_)), arguments(std::move(args_)) {}
};

// Shorthand call: Size(800, 600) - CapitalizedIdentifier(args) in autoSet context
struct ShorthandCall : Expression {
    std::string name;  // e.g., "Size", "Color"
    std::vector<Argument> arguments;
    
    // Filled in by AutoSetResolver
    std::string resolved_setter;  // e.g., "setSize"
    std::string resolved_target;  // e.g., "window"

    ShorthandCall() = default;
    ShorthandCall(std::string name_, std::vector<Argument> args_, SourceLocation loc)
        : Expression(loc), name(std::move(name_)), arguments(std::move(args_)) {}
};

// Edge position: left, right, top, bottom (with optional offset)
struct EdgePosition : Expression {
    enum class Edge {
        Left,
        Right,
        Top,
        Bottom
    } edge;
    ExpressionPtr offset;  // Optional offset value

    EdgePosition() = default;
    EdgePosition(Edge edge_, ExpressionPtr offset_, SourceLocation loc)
        : Expression(loc), edge(edge_), offset(std::move(offset_)) {}
};

// Corner position: topLeft, topRight, bottomLeft, bottomRight
struct CornerPosition : Expression {
    enum class Corner {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    } corner;

    CornerPosition() = default;
    CornerPosition(Corner corner_, SourceLocation loc)
        : Expression(loc), corner(corner_) {}
};

// Relative position: object.direction(distance).reference_object
struct RelativePosition : Expression {
    enum class Direction {
        Left,
        Right,
        Top,
        Bottom
    } direction;
    ExpressionPtr distance;
    std::string reference_object;  // Identifier of the reference object

    RelativePosition() = default;
    RelativePosition(Direction direction_, ExpressionPtr distance_, std::string reference_, SourceLocation loc)
        : Expression(loc), direction(direction_), distance(std::move(distance_)), reference_object(std::move(reference_)) {}
};

// Window visible titlebar: window.visible.titlebar(true/false)
struct VisibleTitleBar : Expression {
    ExpressionPtr value;  // Boolean literal

    VisibleTitleBar() = default;
    VisibleTitleBar(ExpressionPtr value_, SourceLocation loc)
        : Expression(loc), value(std::move(value_)) {}
};

struct ObjectBlock : Expression {
    std::vector<StatementPtr> statements;

    ObjectBlock() = default;
    ObjectBlock(std::vector<StatementPtr> stmts_, SourceLocation loc)
        : Expression(loc), statements(std::move(stmts_)) {}
};

struct ObjectDecl : Statement {
    std::string name;
    std::string type_name;  // e.g., "Window", "Object"
    std::unique_ptr<ObjectBlock> block;  // null if no block
    bool is_auto_set_context = false;  // true if created in #autoSet context

    ObjectDecl() = default;
    ObjectDecl(std::string name_, std::string type_, std::unique_ptr<ObjectBlock> block_, SourceLocation loc)
        : Statement(loc), name(std::move(name_)), type_name(std::move(type_)), block(std::move(block_)) {}
};

struct ImportDecl : Statement {
    std::string module_name;

    ImportDecl() = default;
    ImportDecl(std::string module_, SourceLocation loc) : Statement(loc), module_name(std::move(module_)) {}
};

struct AutoDirective : Statement {
    enum class Kind {
        AutoSet,
        AutoPick
    } kind;

    std::vector<StatementPtr> body;

    AutoDirective() = default;
    AutoDirective(Kind kind_, std::vector<StatementPtr> body_, SourceLocation loc)
        : Statement(loc), kind(kind_), body(std::move(body_)) {}
    
    static AutoDirective auto_set(std::vector<StatementPtr> body_, SourceLocation loc) {
        return AutoDirective(Kind::AutoSet, std::move(body_), loc);
    }
    static AutoDirective auto_pick(std::vector<StatementPtr> body_, SourceLocation loc) {
        return AutoDirective(Kind::AutoPick, std::move(body_), loc);
    }
};

struct Program : ASTNode {
    std::vector<StatementPtr> statements;

    Program() = default;
    explicit Program(SourceLocation loc) : ASTNode(loc) {}
};

using ProgramPtr = std::unique_ptr<Program>;

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visit_program(Program& node) = 0;
    virtual void visit_import(ImportDecl& node) = 0;
    virtual void visit_object_decl(ObjectDecl& node) = 0;
    virtual void visit_object_block(ObjectBlock& node) = 0;
    virtual void visit_method_call(MethodCall& node) = 0;
    virtual void visit_shorthand_call(ShorthandCall& node) = 0;
    virtual void visit_edge_position(EdgePosition& node) = 0;
    virtual void visit_corner_position(CornerPosition& node) = 0;
    virtual void visit_relative_position(RelativePosition& node) = 0;
    virtual void visit_visible_title_bar(VisibleTitleBar& node) = 0;
    virtual void visit_auto_directive(AutoDirective& node) = 0;
    virtual void visit_identifier(Identifier& node) = 0;
    virtual void visit_literal(Literal& node) = 0;
    virtual void visit_argument(Argument& node) = 0;
    virtual void visit_expression_statement(ExpressionStatement& node) = 0;
};

void accept(ASTNode& node, ASTVisitor& visitor);
void accept(Program& node, ASTVisitor& visitor);
void accept(ImportDecl& node, ASTVisitor& visitor);
void accept(ObjectDecl& node, ASTVisitor& visitor);
void accept(ObjectBlock& node, ASTVisitor& visitor);
void accept(MethodCall& node, ASTVisitor& visitor);
void accept(ShorthandCall& node, ASTVisitor& visitor);
void accept(EdgePosition& node, ASTVisitor& visitor);
void accept(CornerPosition& node, ASTVisitor& visitor);
void accept(RelativePosition& node, ASTVisitor& visitor);
void accept(VisibleTitleBar& node, ASTVisitor& visitor);
void accept(AutoDirective& node, ASTVisitor& visitor);
void accept(Identifier& node, ASTVisitor& visitor);
void accept(Literal& node, ASTVisitor& visitor);
void accept(Argument& node, ASTVisitor& visitor);
void accept(ExpressionStatement& node, ASTVisitor& visitor);

// AST printing for debugging
std::string ast_to_string(const ASTNode& node, int indent = 0);
std::string ast_to_string(const Program& program);

} // namespace fxl