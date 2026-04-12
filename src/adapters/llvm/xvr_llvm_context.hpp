#ifndef XVR_LLVM_CONTEXT_HPP
#define XVR_LLVM_CONTEXT_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include "xvr_type.hpp"

namespace xvr {

class LLVMContext {
public:
    LLVMContext();
    ~LLVMContext();
    
    llvm::LLVMContext& context() { return *context_; }
    llvm::Module* module() { return module_.get(); }
    llvm::IRBuilder<>& builder() { return builder_; }
    
    void setSourceFile(const std::string& name);
    void setTargetTriple(const std::string& triple);
    
    llvm::Type* getLLVMType(TypePtr type);
    llvm::Value* getOrCreateGlobal(const std::string& name, llvm::Type* type);
    
    bool verify() const;
    void print(llvm::raw_ostream& os) const;
    
private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<> builder_;
    
    std::unordered_map<std::string, llvm::Type*> typeCache_;
    std::unordered_map<std::string, llvm::Value*> globals_;
    
    llvm::Type* mapPrimitiveType(TypeKind kind);
};

}  // namespace xvr

#endif  // XVR_LLVM_CONTEXT_HPP