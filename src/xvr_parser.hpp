#ifndef XVR_PARSER_HPP
#define XVR_PARSER_HPP

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/ast/xvr_ast_node.hpp"
#include "xvr_lexer.hpp"
#include "xvr_token_types.h"

namespace xvr {

class Parser {
public:
    explicit Parser(std::string_view source);
    std::unique_ptr<ASTNode> parse();
    std::vector<std::unique_ptr<ASTNode>> parseAll();
    
    bool hasError() const { return hasError_; }
    const std::string& errorMessage() const { return errorMessage_; }
    void reset();

private:
    std::unique_ptr<Lexer> lexer_;
    std::optional<Token> current_;
    std::optional<Token> previous_;
    bool hasError_{false};
    std::string errorMessage_;
    
    void advance();
    void consume(TokenType type, const char* message);
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    void synchroniz();
    
    std::unique_ptr<ASTNode> declaration();
    std::unique_ptr<ASTNode> statement();
    std::unique_ptr<ASTNode> expressionStatement();
    std::unique_ptr<ASTNode> block();
    std::unique_ptr<ASTNode> varDeclaration();
    std::unique_ptr<ASTNode> functionDeclaration();
    std::unique_ptr<ASTNode> ifStatement();
    std::unique_ptr<ASTNode> whileStatement();
    std::unique_ptr<ASTNode> forStatement();
    std::unique_ptr<ASTNode> returnStatement();
    std::unique_ptr<ASTNode> breakStatement();
    std::unique_ptr<ASTNode> continueStatement();
    
    std::unique_ptr<ASTNode> expression();
    std::unique_ptr<ASTNode> assignment();
    std::unique_ptr<ASTNode> orExpression();
    std::unique_ptr<ASTNode> andExpression();
    std::unique_ptr<ASTNode> equality();
    std::unique_ptr<ASTNode> comparison();
    std::unique_ptr<ASTNode> term();
    std::unique_ptr<ASTNode> factor();
    std::unique_ptr<ASTNode> unary();
    std::unique_ptr<ASTNode> call();
    std::unique_ptr<ASTNode> primary();
    
    void error(const char* message);
    void errorAtCurrent(const char* message);
};

}  // namespace xvr

#endif  // XVR_PARSER_HPP