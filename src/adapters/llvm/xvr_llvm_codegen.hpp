#ifndef XVR_LLVM_CODEGEN_HPP
#define XVR_LLVM_CODEGEN_HPP

#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/Value.h>

#include "core/ast/xvr_ast_node.hpp"
#include "xvr_llvm_context.hpp"
#include "xvr_scope.hpp"

namespace xvr {

class LLVMCodegen {
public:
    explicit LLVMCodegen(LLVMContext& ctx);
    ~LLVMCodegen() = default;
    
    bool generate(ASTNode& ast);
    llvm::Function* generateFunction(ASTNode& node);
    llvm::Value* generateExpression(ASTNode& node);
    
    void setOptimize(bool opt) { optimize_ = opt; }
    bool hasError() const { return hasError_; }
    const std::string& errorMessage() const { return errorMessage_; }
    
private:
    LLVMContext& ctx_;
    ScopeStack scopeStack_;
    std::unordered_map<std::string, llvm::AllocaInst*> locals_;
    std::stack<llvm::BasicBlock*> breakStack_;
    std::stack<llvm::BasicBlock*> continueStack_;
    bool optimize_{false};
    bool hasError_{false};
    std::string errorMessage_;
    
    llvm::Value* generateLiteral(NodeLiteral& node);
    llvm::Value* generateBinary(NodeBinary& node);
    llvm::Value* generateUnary(NodeUnary& node);
    llvm::Value* generateCall(NodeFnCall& node);
    llvm::Value* generateIf(NodeIf& node);
    llvm::Value* generateWhile(NodeWhile& node);
    llvm::Value* generateFor(NodeFor& node);
    llvm::Value* generateBlock(NodeBlock& node);
    llvm::Value* generateVarDecl(NodeVarDecl& node);
    llvm::Value* generateReturn(NodeFnReturn& node);
    
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);
    void setLocal(const std::string& name, llvm::Value* value);
    std::optional<llvm::Value*> getLocal(const std::string& name);
    
    llvm::Type* mapType(NodeVarDecl& node);
    llvm::Value* createCast(llvm::Value* value, llvm::Type* destType);
};

}  // namespace xvr

#endif  // XVR_LLVM_CODEGEN_HPP