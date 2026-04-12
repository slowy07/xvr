#ifndef XVR_SCOPE_HPP
#define XVR_SCOPE_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "xvr_literal.h"
#include "xvr_type.hpp"

namespace xvr {

struct Variable {
    Xvr_Literal identifier;
    TypePtr type;
    int depth;
    bool isMutable;
    
    Variable(Xvr_Literal id, TypePtr t, int d, bool mut = true)
        : identifier(id), type(std::move(t)), depth(d), isMutable(mut) {}
};

class Scope {
public:
    Scope() = default;
    explicit Scope(int depth) : depth_(depth) {}
    
    void define(const std::string& name, Variable var);
    std::optional<std::reference_wrapper<Variable>> lookup(const std::string& name);
    std::optional<std::reference_wrapper<Variable>> lookupLocal(const std::string& name) const;
    
    int depth() const { return depth_; }
    void setDepth(int d) { depth_ = d; }
    
    bool contains(const std::string& name) const;
    void clear();
    
private:
    std::unordered_map<std::string, Variable> values_;
    int depth_{0};
};

class ScopeStack {
public:
    ScopeStack() = default;
    
    void pushScope();
    void popScope();
    void define(const std::string& name, Variable var);
    std::optional<std::reference_wrapper<Variable>> lookup(const std::string& name);
    int currentDepth() const;
    
    void enterBlock();
    void exitBlock();
    
private:
    std::vector<Scope> scopes_;
    int blockDepth_{0};
};

}  // namespace xvr

#endif  // XVR_SCOPE_HPP