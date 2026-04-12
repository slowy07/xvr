# C++ Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate the entire XVR compiler codebase from C to C++20 with modern C++ patterns while maintaining backward compatibility and not breaking the system.

**Architecture:** Phase-based migration starting from core data structures (AST, Literal, Types) → low-level utilities (Memory, RefString, Common) → mid-level components (Lexer, Parser, Semantic) → high-level (LLVM Backend) → tests and build system. Each phase replaces C code with C++ equivalents, maintaining API compatibility through `extern "C"` wrappers where needed.

**Tech Stack:** C++20 (with C++17 compatibility), CMake, LLVM C++ API

---

## Phase 1: Core Data Structures (AST, Literal, Types)

### Task 1.1: Update CMakeLists.txt for C++ Support

**Files:**
- Modify: `CMakeLists.txt:1-6`
- Modify: `CMakeLists.txt:39-47`

- [ ] **Step 1: Modify root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(xvr LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS ON)
```

- [ ] **Step 2: Update compile flags for C++**

Replace lines 39-47 with:
```cmake
set(XVR_COMPILE_FLAGS
    -std=c++20
    -pedantic
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-unused-variable
)
```

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add C++20 support to CMake"
```

---

### Task 1.2: Create C++ AST Node Header

**Files:**
- Create: `src/core/ast/xvr_ast_node.hpp`
- Modify: `src/core/ast/xvr_ast_node.h` (add include guard for C++)

- [ ] **Step 1: Create modern C++ AST node header**

```cpp
#ifndef XVR_AST_NODE_HPP
#define XVR_AST_NODE_HPP

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "xvr_literal.h"  // Keep C literal for now
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
    // ... other create methods

    void free();
};

}  // namespace xvr

#endif  // XVR_AST_NODE_HPP
```

- [ ] **Step 2: Update C header to include C++ guard**

```c
#ifdef __cplusplus
extern "C" {
#endif

// ... existing C code ...

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 3: Commit**

```bash
git add src/core/ast/xvr_ast_node.hpp src/core/ast/xvr_ast_node.h
git commit -m "feat(ast): add C++ AST node header with std::variant"
```

---

### Task 1.3: Create C++ Type System Header

**Files:**
- Create: `src/core/types/xvr_type.hpp`
- Modify: `src/core/types/xvr_type.h`

- [ ] **Step 1: Create modern C++ type header**

```cpp
#ifndef XVR_TYPE_HPP
#define XVR_TYPE_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "xvr_literal.h"

namespace xvr {

enum class TypeKind {
    Null,
    Void,
    Boolean,
    Integer,
    Float,
    String,
    Array,
    Dictionary,
    Function,
    Identifier,
    Type,
    Opaque,
    Any,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float16,
    Float32,
    Float64
};

class Type {
public:
    TypeKind kind;
    std::vector<std::shared_ptr<Type>> subtypes;
    bool isConstant{false};
    int bitWidth{0};

    Type() : kind(TypeKind::Null) {}
    explicit Type(TypeKind k) : kind(k) {}

    bool isPrimitive() const;
    bool isNumeric() const;
    bool isInteger() const;
    bool isFloat() const;
    bool isPointer() const;
    bool isArray() const;
    bool isDictionary() const;
    bool isFunction() const;

    static std::shared_ptr<Type> fromLiteralType(Xvr_LiteralType literalType);
    Xvr_LiteralType toLiteralType() const;

    std::string toString() const;
    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const { return !(*this == other); }
};

using TypePtr = std::shared_ptr<Type>;
using TypeMap = std::unordered_map<std::string, TypePtr>;

class TypeEnvironment {
public:
    void define(const std::string& name, TypePtr type);
    std::optional<TypePtr> lookup(const std::string& name) const;
    void enterScope();
    void exitScope();

private:
    std::vector<TypeMap> scopes_;
};

}  // namespace xvr

#endif  // XVR_TYPE_HPP
```

- [ ] **Step 2: Update C header for C++ compatibility**

Add `extern "C"` guards to `src/core/types/xvr_type.h`

- [ ] **Step 3: Commit**

```bash
git add src/core/types/xvr_type.hpp src/core/types/xvr_type.h
git commit -m "feat(types): add C++ type system with classes"
```

---

## Phase 2: Low-Level Components

### Task 2.1: Create C++ Memory Management Header

**Files:**
- Create: `src/xvr_memory.hpp`
- Modify: `src/xvr_memory.h`

- [ ] **Step 1: Create C++ memory header**

```cpp
#ifndef XVR_MEMORY_HPP
#define XVR_MEMORY_HPP

#include <cstddef>
#include <memory>
#include <new>

namespace xvr {

using ReallocateFn = void* (*)(void* pointer, size_t oldSize, size_t newSize);

void setMemoryAllocator(ReallocateFn allocator);
ReallocateFn getMemoryAllocator();

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

template<typename T>
T* allocate(size_t count = 1) {
    return static_cast<T*>(reallocate(nullptr, 0, sizeof(T) * count));
}

template<typename T>
void free(T* pointer, size_t count = 1) {
    reallocate(pointer, sizeof(T) * count, 0);
}

template<typename T>
T* grow(T* oldPtr, size_t oldCount, size_t newCount) {
    return static_cast<T*>(reallocate(oldPtr, sizeof(T) * oldCount, sizeof(T) * newCount));
}

template<typename T>
T* shrink(T* oldPtr, size_t oldCount, size_t newCount) {
    return static_cast<T*>(reallocate(oldPtr, sizeof(T) * oldCount, sizeof(T) * newCount));
}

constexpr size_t growCapacity(size_t capacity) {
    return capacity < 8 ? 8 : capacity * 2;
}

constexpr size_t growCapacityFast(size_t capacity) {
    return capacity < 32 ? 32 : capacity * 2;
}

#if defined(XVR_DEBUG) || defined(DEBUG)
void debugPrintMemoryStats();
size_t debugGetAllocCount();
size_t debugGetFreeCount();
size_t debugGetCurrentMemory();
void debugResetMemoryStats();
#endif

template<typename T>
class UniquePtr {
    T* ptr_;
public:
    explicit UniquePtr(T* p = nullptr) : ptr_(p) {}
    ~UniquePtr() { free<T>(ptr_); }
    
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;
    
    UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            free<T>(ptr_);
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
    
    void reset(T* p = nullptr) {
        free<T>(ptr_);
        ptr_ = p;
    }
    
    T* release() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }
};

template<typename T, typename Deleter>
class UniquePtrWithDeleter {
    T* ptr_;
    Deleter deleter_;
public:
    explicit UniquePtrWithDeleter(T* p = nullptr, Deleter d = Deleter{}) 
        : ptr_(p), deleter_(d) {}
    ~UniquePtrWithDeleter() { if (ptr_) deleter_(ptr_); }
    
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    void reset(T* p = nullptr) { 
        if (ptr_) deleter_(ptr_); 
        ptr_ = p; 
    }
};

}  // namespace xvr

#endif  // XVR_MEMORY_HPP
```

- [ ] **Step 2: Update C header with extern "C" guards**

- [ ] **Step 3: Commit**

```bash
git add src/xvr_memory.hpp src/xvr_memory.h
git commit -m "feat(memory): add C++ memory management with smart pointers"
```

---

### Task 2.2: Create C++ RefString Header

**Files:**
- Create: `src/xvr_refstring.hpp`
- Modify: `src/xvr_refstring.h`

- [ ] **Step 1: Create C++ RefString header**

```cpp
#ifndef XVR_REFSTRING_HPP
#define XVR_REFSTRING_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "xvr_memory.h"  // Use C allocator

namespace xvr {

class RefString {
public:
    RefString() = default;
    explicit RefString(const char* str);
    explicit RefString(const char* str, size_t len);
    explicit RefString(std::string_view view);
    
    RefString(const RefString& other);
    RefString& operator=(const RefString& other);
    
    RefString(RefString&& other) noexcept;
    RefString& operator=(RefString&& other) noexcept;
    
    ~RefString();
    
    const char* data() const { return data_; }
    size_t length() const { return length_; }
    size_t refCount() const { return refCount_; }
    size_t capacity() const { return capacity_; }
    
    std::string_view view() const { return std::string_view(data_, length_); }
    std::string toString() const { return std::string(data_, length_); }
    
    void incrementRef();
    void decrementRef();
    
    static RefString* create(const char* str);
    static RefString* create(const char* str, size_t len);
    static RefString* create(std::string_view view);
    static void destroy(RefString* rs);
    
private:
    char* data_{nullptr};
    size_t length_{0};
    size_t refCount_{1};
    size_t capacity_{0};
    
    void allocateAndCopy(const char* str, size_t len);
    void reallocate(size_t newCapacity);
};

using RefStringPtr = std::shared_ptr<RefString>;
using RefStringWeakPtr = std::weak_ptr<RefString>;

inline bool operator==(const RefString& lhs, const RefString& rhs) {
    return lhs.view() == rhs.view();
}

inline bool operator!=(const RefString& lhs, const RefString& rhs) {
    return !(lhs == rhs);
}

}  // namespace xvr

#endif  // XVR_REFSTRING_HPP
```

- [ ] **Step 2: Create implementation file**

```cpp
// src/xvr_refstring.cpp
#include "xvr_refstring.hpp"

namespace xvr {

void RefString::allocateAndCopy(const char* str, size_t len) {
    capacity_ = len + 1;
    data_ = static_cast<char*>(xvr::reallocate(nullptr, 0, capacity_));
    length_ = len;
    refCount_ = 1;
    if (str) {
        std::memcpy(data_, str, len);
        data_[len] = '\0';
    }
}

RefString::RefString(const char* str) {
    if (str) {
        allocateAndCopy(str, std::strlen(str));
    }
}

RefString::RefString(const char* str, size_t len) {
    if (str && len > 0) {
        allocateAndCopy(str, len);
    }
}

RefString::RefString(std::string_view view) {
    if (!view.empty()) {
        allocateAndCopy(view.data(), view.size());
    }
}

RefString::RefString(const RefString& other) 
    : data_(other.data_), length_(other.length_), 
      refCount_(other.refCount_), capacity_(other.capacity_) {
    incrementRef();
}

RefString& RefString::operator=(const RefString& other) {
    if (this != &other) {
        decrementRef();
        data_ = other.data_;
        length_ = other.length_;
        refCount_ = other.refCount_;
        capacity_ = other.capacity_;
        incrementRef();
    }
    return *this;
}

RefString::RefString(RefString&& other) noexcept 
    : data_(other.data_), length_(other.length_),
      refCount_(other.refCount_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.length_ = 0;
    other.refCount_ = 0;
    other.capacity_ = 0;
}

RefString& RefString::operator=(RefString&& other) noexcept {
    if (this != &other) {
        decrementRef();
        data_ = other.data_;
        length_ = other.length_;
        refCount_ = other.refCount_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.length_ = 0;
        other.refCount_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

RefString::~RefString() {
    decrementRef();
}

void RefString::incrementRef() {
    if (data_) {
        ++refCount_;
    }
}

void RefString::decrementRef() {
    if (data_ && --refCount_ == 0) {
        xvr::free<char>(data_, capacity_);
        data_ = nullptr;
    }
}

RefString* RefString::create(const char* str) {
    return new RefString(str);
}

RefString* RefString::create(const char* str, size_t len) {
    return new RefString(str, len);
}

RefString* RefString::create(std::string_view view) {
    return new RefString(view);
}

void RefString::destroy(RefString* rs) {
    delete rs;
}

}  // namespace xvr
```

- [ ] **Step 3: Update C header with extern "C" guards**

- [ ] **Step 4: Commit**

```bash
git add src/xvr_refstring.hpp src/xvr_refstring.cpp src/xvr_refstring.h
git commit -m "feat(refstring): add C++ RefString with smart pointers"
```

---

### Task 2.3: Create C++ Common Utilities Header

**Files:**
- Create: `src/xvr_common.hpp`
- Modify: `src/xvr_common.h`

- [ ] **Step 1: Create C++ common utilities**

```cpp
#ifndef XVR_COMMON_HPP
#define XVR_COMMON_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "xvr_memory.h"

namespace xvr {

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 6;
constexpr int VERSION_PATCH = 16;
constexpr std::string_view VERSION_BUILD = __DATE__ " " __TIME__;

enum class Bitness {
    Bits32 = 32,
    Bits64 = 64,
    Unknown = -1
};

#if defined(__linux__)
constexpr Bitness CURRENT_BITNESS = defined(__LP64__) ? Bitness::Bits64 : Bitness::Bits32;
#elif defined(_WIN64)
constexpr Bitness CURRENT_BITNESS = Bitness::Bits64;
#elif defined(__APPLE__)
constexpr Bitness CURRENT_BITNESS = defined(__LP64__) ? Bitness::Bits64 : Bitness::Bits32;
#else
constexpr Bitness CURRENT_BITNESS = Bitness::Unknown;
#endif

class CommandLine {
public:
    bool error{false};
    bool help{false};
    bool version{false};
    std::optional<std::string> sourceFile;
    std::optional<std::string> compileFile;
    std::optional<std::string> outFile;
    std::optional<std::string> source;
    std::optional<std::string> initialfile;
    bool enablePrintNewline{false};
    bool verbose{false};
    bool dumpTokens{false};
    bool dumpAST{false};
    bool dumpLLVM{false};
    bool compileOnly{false};
    bool compileAndRun{false};
    bool showTiming{false};
    std::optional<std::string> emitType;
    std::optional<std::string> asmSyntax;
    int optimizationLevel{0};
    
    static CommandLine& instance();
    void parse(int argc, const char* argv[]);
    void printUsage() const;
    void printHelp() const;
    void printCopyright() const;
};

CommandLine& commandLine();

std::string strdup(std::string_view str);

template<typename... Args>
std::string format(std::string_view fmt, Args... args) {
    // Simple formatting - can be enhanced later
    return std::string(fmt);
}

template<typename T>
constexpr T min(T a, T b) { return a < b ? a : b; }

template<typename T>
constexpr T max(T a, T b) { return a > b ? a : b; }

template<typename T>
constexpr T clamp(T value, T minVal, T maxVal) {
    return value < minVal ? minVal : value > maxVal ? maxVal : value;
}

template<typename Container>
constexpr auto size(const Container& c) -> decltype(c.size()) {
    return c.size();
}

template<typename T, size_t N>
constexpr size_t size(const T (&arr)[N]) {
    return N;
}

}  // namespace xvr

#endif  // XVR_COMMON_HPP
```

- [ ] **Step 2: Update C header with extern "C" guards**

- [ ] **Step 3: Commit**

```bash
git add src/xvr_common.hpp src/xvr_common.h
git commit -m "feat(common): add C++ common utilities"
```

---

## Phase 3: Mid-Level Components

### Task 3.1: Create C++ Lexer Header

**Files:**
- Create: `src/xvr_lexer.hpp`
- Modify: `src/xvr_lexer.h`

- [ ] **Step 1: Create C++ Lexer header**

```cpp
#ifndef XVR_LEXER_HPP
#define XVR_LEXER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "xvr_token_types.h"

namespace xvr {

struct Token {
    TokenType type;
    std::string_view lexeme;
    int line;
    int column;
    
    Token(TokenType t, std::string_view l, int ln, int col)
        : type(t), lexeme(l), line(ln), column(col) {}
};

class Lexer {
public:
    explicit Lexer(std::string_view source);
    std::vector<Token> scanAll();
    std::optional<Token> nextToken();
    bool hasMore() const;
    void reset();
    
private:
    std::string_view source_;
    size_t current_{0};
    size_t start_{0};
    int line_{1};
    int column_{1};
    
    char peek() const;
    char peekAhead(size_t offset) const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    Token scanString();
    Token scanNumber();
    Token scanIdentifier();
    Token makeToken(TokenType type);
    void skipLine();
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
};

}  // namespace xvr

#endif  // XVR_LEXER_HPP
```

- [ ] **Step 2: Update C header with extern "C"**

- [ ] **Step 3: Commit**

```bash
git add src/xvr_lexer.hpp src/xvr_lexer.h
git commit -m "feat(lexer): add C++ Lexer class with std::string_view"
```

---

### Task 3.2: Create C++ Parser Header

**Files:**
- Create: `src/xvr_parser.hpp`
- Modify: `src/xvr_parser.h`

- [ ] **Step 1: Create C++ Parser header**

```cpp
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
    void synchronize();
    
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
```

- [ ] **Step 2: Update C header with extern "C"**

- [ ] **Step 3: Commit**

```bash
git add src/xvr_parser.hpp src/xvr_parser.h
git commit -m "feat(parser): add C++ Parser class with unique_ptr"
```

---

### Task 3.3: Create C++ Scope Header

**Files:**
- Create: `src/xvr_scope.hpp`
- Modify: `src/xvr_scope.h`

- [ ] **Step 1: Create C++ Scope header**

```cpp
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
```

- [ ] **Step 2: Update C header with extern "C"**

- [ ] **Step 3: Commit**

```bash
git add src/xvr_scope.hpp src/xvr_scope.h
git commit -m "feat(scope): add C++ Scope classes"
```

---

## Phase 4: High-Level Components (LLVM Backend)

### Task 4.1: Create C++ LLVM Context Header

**Files:**
- Create: `src/adapters/llvm/xvr_llvm_context.hpp`
- Modify: `src/adapters/llvm/xvr_llvm_context.h`

- [ ] **Step 1: Create C++ LLVM context**

```cpp
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
```

- [ ] **Step 2: Update C header with extern "C"**

- [ ] **Step 3: Commit**

```bash
git add src/adapters/llvm/xvr_llvm_context.hpp src/adapters/llvm/xvr_llvm_context.h
git commit -m "feat(llvm): add C++ LLVM Context wrapper"
```

---

### Task 4.2: Create C++ LLVM Codegen Header

**Files:**
- Create: `src/adapters/llvm/xvr_llvm_codegen.hpp`
- Modify: `src/adapters/llvm/xvr_llvm_codegen.h`

- [ ] **Step 1: Create C++ LLVM codegen**

```cpp
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
```

- [ ] **Step 2: Update C header with extern "C"**

- [ ] **Step 3: Commit**

```bash
git add src/adapters/llvm/xvr_llvm_codegen.hpp src/adapters/llvm/xvr_llvm_codegen.h
git commit -m "feat(llvm): add C++ LLVM codegen wrapper"
```

---

## Phase 5: Tests Migration

### Task 5.1: Update Test Framework to C++

**Files:**
- Modify: `test/xvr_test.h`

- [ ] **Step 1: Create C++ test framework header**

```cpp
#ifndef XVR_TEST_HPP
#define XVR_TEST_HPP

#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#define TEST_CASE(name) void test_##name()
#define ASSERT_TRUE(expr) assert(expr)
#define ASSERT_FALSE(expr) assert(!(expr))
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_NULL(ptr) assert((ptr) == nullptr)
#define ASSERT_NOT_NULL(ptr) assert((ptr) != nullptr)

namespace xvr {
namespace test {

class TestCase {
public:
    using TestFn = void(*)();
    
    TestCase(std::string_view name, TestFn fn) : name_(name), fn_(fn) {}
    
    void run() {
        std::cout << "Running: " << name_ << "... ";
        try {
            fn_();
            std::cout << "PASSED\n";
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            throw;
        }
    }
    
    std::string_view name() const { return name_; }
    
private:
    std::string_view name_;
    TestFn fn_;
};

class TestRegistry {
public:
    static TestRegistry& instance();
    
    void registerTest(std::string_view suite, std::string_view name, TestCase::TestFn fn);
    void runAll();
    void runSuite(std::string_view suite);
    
private:
    TestRegistry() = default;
    
    struct TestEntry {
        std::string suite;
        std::string name;
        TestCase::TestFn fn;
    };
    
    std::vector<TestEntry> tests_;
};

#define TEST_SUITE(suite_name) namespace suite_name {
#define END_SUITE }

#define REGISTER_TEST(suite, name) \
    static bool _test_##suite##_##name = ([]() { \
        xvr::test::TestRegistry::instance().registerTest(#suite, #name, test_##name); \
        return true; \
    })();

}  // namespace test
}  // namespace xvr

#endif  // XVR_TEST_HPP
```

- [ ] **Step 2: Commit**

```bash
git add test/xvr_test.hpp
git commit -m "test: add C++ test framework"
```

---

## Phase 6: Build System Finalization

### Task 6.1: Update CMake for C++ Sources

**Files:**
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Update CMakeLists.txt for mixed C/C++**

```cmake
set(XVR_SOURCES_CXX
    core/types/xvr_type.cpp
    xvr_refstring.cpp
    xvr_lexer.cpp
    xvr_parser.cpp
    xvr_scope.cpp
    adapters/llvm/xvr_llvm_context.cpp
    adapters/llvm/xvr_llvm_codegen.cpp
)

# Keep existing C sources
set(XVR_SOURCES
    core/types/xvr_type.c
    xvr_cast_emit.c
    xvr_common.c
    # ... rest of C sources
)

add_library(xvr_objects OBJECT ${XVR_SOURCES} ${XVR_SOURCES_CXX} ${XVR_HEADERS})

set_target_properties(xvr_objects PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

target_compile_options(xvr_objects PRIVATE 
    $<$<COMPILE_LANGUAGE:C>:-std=c11>
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>
    -pedantic
    -Wall
    -Wextra
)
```

- [ ] **Step 2: Commit**

```bash
git add src/CMakeLists.txt
git commit -m "build: update CMake for C++20 sources"
```

---

## Summary

This migration plan covers:

1. **Phase 1** - Core data structures with `std::variant` and smart pointers
2. **Phase 2** - Low-level memory management with C++ wrappers
3. **Phase 3** - Lexer/Parser with modern C++ idioms
4. **Phase 4** - LLVM backend with C++ wrappers
5. **Phase 5** - Test framework modernization
6. **Phase 6** - Build system updates

Each task is designed to be self-contained and testable. The C code remains functional throughout the migration, with C++ code being added incrementally and wrapped with `extern "C"` for compatibility.