#ifndef XVR_AST_NODE_HPP
#define XVR_AST_NODE_HPP

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "xvr_literal.h"
#include "xvr_opcodes.h"
#include "xvr_token_types.h"

namespace xvr {

enum class ASTNodeType {
    Error,
    Literal,
    Unary,
    Binary,
    Ternary,
    Grouping,
    Block,
    Compound,
    Pair,
    Index,
    VarDecl,
    FnDecl,
    FnCollection,
    FnCall,
    FnReturn,
    If,
    While,
    For,
    Break,
    Continue,
    PrefixIncrement,
    PostfixIncrement,
    PrefixDecrement,
    PostfixDecrement,
    Cast,
    Import,
    Pass
};

struct NodeLiteral {
    ASTNodeType type{ASTNodeType::Literal};
    Xvr_Literal literal;
};

struct NodeUnary {
    ASTNodeType type{ASTNodeType::Unary};
    Xvr_Opcode opcode;
    std::unique_ptr<class ASTNode> child;
};

struct NodeBinary {
    ASTNodeType type{ASTNodeType::Binary};
    Xvr_Opcode opcode;
    std::unique_ptr<class ASTNode> left;
    std::unique_ptr<class ASTNode> right;
};

struct NodeTernary {
    ASTNodeType type{ASTNodeType::Ternary};
    std::unique_ptr<class ASTNode> condition;
    std::unique_ptr<class ASTNode> thenPath;
    std::unique_ptr<class ASTNode> elsePath;
};

struct NodeGrouping {
    ASTNodeType type{ASTNodeType::Grouping};
    std::unique_ptr<class ASTNode> child;
};

struct NodeBlock {
    ASTNodeType type{ASTNodeType::Block};
    std::vector<std::unique_ptr<class ASTNode>> nodes;
};

struct NodeCompound {
    ASTNodeType type{ASTNodeType::Compound};
    Xvr_LiteralType literalType;
    std::vector<std::unique_ptr<class ASTNode>> nodes;
};

struct NodePair {
    ASTNodeType type{ASTNodeType::Pair};
    std::unique_ptr<class ASTNode> left;
    std::unique_ptr<class ASTNode> right;
};

struct NodeIndex {
    ASTNodeType type{ASTNodeType::Index};
    std::unique_ptr<class ASTNode> first;
    std::optional<std::unique_ptr<class ASTNode>> second;
    std::optional<std::unique_ptr<class ASTNode>> third;
};

struct NodeVarDecl {
    ASTNodeType type{ASTNodeType::VarDecl};
    Xvr_Literal identifier;
    Xvr_Literal typeLiteral;
    std::unique_ptr<class ASTNode> expression;
    int line{0};
};

struct NodeFnCollection {
    ASTNodeType type{ASTNodeType::FnCollection};
    std::vector<std::unique_ptr<class ASTNode>> nodes;
};

struct NodeFnDecl {
    ASTNodeType type{ASTNodeType::FnDecl};
    Xvr_Literal identifier;
    std::unique_ptr<class ASTNode> arguments;
    std::unique_ptr<class ASTNode> returns;
    std::unique_ptr<class ASTNode> block;
    int line{0};
};

struct NodeFnCall {
    ASTNodeType type{ASTNodeType::FnCall};
    std::unique_ptr<class ASTNode> arguments;
    int argumentCount{0};
};

struct NodeFnReturn {
    ASTNodeType type{ASTNodeType::FnReturn};
    std::unique_ptr<class ASTNode> returns;
};

struct NodeIf {
    ASTNodeType type{ASTNodeType::If};
    std::unique_ptr<class ASTNode> condition;
    std::unique_ptr<class ASTNode> thenPath;
    std::unique_ptr<class ASTNode> elsePath;
    Xvr_LiteralType returnType{XVR_LITERAL_TYPE};
    bool isExpression{false};
};

struct NodeWhile {
    ASTNodeType type{ASTNodeType::While};
    std::unique_ptr<class ASTNode> condition;
    std::unique_ptr<class ASTNode> thenPath;
};

struct NodeFor {
    ASTNodeType type{ASTNodeType::For};
    std::unique_ptr<class ASTNode> preClause;
    std::unique_ptr<class ASTNode> condition;
    std::unique_ptr<class ASTNode> postClause;
    std::unique_ptr<class ASTNode> thenPath;
};

struct NodeBreak {
    ASTNodeType type{ASTNodeType::Break};
};

struct NodeContinue {
    ASTNodeType type{ASTNodeType::Continue};
};

struct NodePrefixIncrement {
    ASTNodeType type{ASTNodeType::PrefixIncrement};
    Xvr_Literal identifier;
};

struct NodePrefixDecrement {
    ASTNodeType type{ASTNodeType::PrefixDecrement};
    Xvr_Literal identifier;
};

struct NodePostfixIncrement {
    ASTNodeType type{ASTNodeType::PostfixIncrement};
    Xvr_Literal identifier;
};

struct NodePostfixDecrement {
    ASTNodeType type{ASTNodeType::PostfixDecrement};
    Xvr_Literal identifier;
};

struct NodeCast {
    ASTNodeType type{ASTNodeType::Cast};
    Xvr_Literal targetType;
    std::unique_ptr<class ASTNode> expression;
};

struct NodeImport {
    ASTNodeType type{ASTNodeType::Import};
    Xvr_Literal identifier;
    Xvr_Literal alias;
};

using ASTNodeVariant = std::variant<
    NodeLiteral,
    NodeUnary,
    NodeBinary,
    NodeTernary,
    NodeGrouping,
    NodeBlock,
    NodeCompound,
    NodePair,
    NodeIndex,
    NodeVarDecl,
    NodeFnCollection,
    NodeFnDecl,
    NodeFnCall,
    NodeFnReturn,
    NodeIf,
    NodeWhile,
    NodeFor,
    NodeBreak,
    NodeContinue,
    NodePrefixIncrement,
    NodePostfixIncrement,
    NodePrefixDecrement,
    NodePostfixDecrement,
    NodeCast,
    NodeImport
>;

class ASTNode {
public:
    ASTNodeVariant variant;

    explicit ASTNode(ASTNodeVariant v) : variant(std::move(v)) {}

    ASTNodeType type() const {
        return std::visit([](const auto& n) -> ASTNodeType { return n.type; }, variant);
    }

    static std::unique_ptr<ASTNode> create(NodeLiteral&& n);
    static std::unique_ptr<ASTNode> create(NodeUnary&& n);
    static std::unique_ptr<ASTNode> create(NodeBinary&& n);
    static std::unique_ptr<ASTNode> create(NodeTernary&& n);
    static std::unique_ptr<ASTNode> create(NodeGrouping&& n);
    static std::unique_ptr<ASTNode> create(NodeBlock&& n);
    static std::unique_ptr<ASTNode> create(NodeCompound&& n);
    static std::unique_ptr<ASTNode> create(NodePair&& n);
    static std::unique_ptr<ASTNode> create(NodeIndex&& n);
    static std::unique_ptr<ASTNode> create(NodeVarDecl&& n);
    static std::unique_ptr<ASTNode> create(NodeFnCollection&& n);
    static std::unique_ptr<ASTNode> create(NodeFnDecl&& n);
    static std::unique_ptr<ASTNode> create(NodeFnCall&& n);
    static std::unique_ptr<ASTNode> create(NodeFnReturn&& n);
    static std::unique_ptr<ASTNode> create(NodeIf&& n);
    static std::unique_ptr<ASTNode> create(NodeWhile&& n);
    static std::unique_ptr<ASTNode> create(NodeFor&& n);
    static std::unique_ptr<ASTNode> create(NodeBreak&& n);
    static std::unique_ptr<ASTNode> create(NodeContinue&& n);
    static std::unique_ptr<ASTNode> create(NodePrefixIncrement&& n);
    static std::unique_ptr<ASTNode> create(NodePostfixIncrement&& n);
    static std::unique_ptr<ASTNode> create(NodePrefixDecrement&& n);
    static std::unique_ptr<ASTNode> create(NodePostfixDecrement&& n);
    static std::unique_ptr<ASTNode> create(NodeCast&& n);
    static std::unique_ptr<ASTNode> create(NodeImport&& n);

    void free();
};

}  // namespace xvr

#endif  // XVR_AST_NODE_HPP