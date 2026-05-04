# XVR Parser Architecture Guide

This document explains the XVR parser architecture for stage0 developers.

## Overview

The XVR parser is a recursive descent parser with Pratt parsing for expressions. It uses a unified `ParseRule` table that maps token types to parsing functions.

## Key Components

### 1. Token Types (xvr_token_types.h)

Token types are defined in `Xvr_TokenType` enum. **Important**: New tokens must be added at the END of the enum to maintain binary compatibility.

```c
typedef enum Xvr_TokenType {
    XVR_TOKEN_NULL,      // 0
    XVR_TOKEN_VOID,      // 1
    // ... don't insert new tokens in the middle!
    XVR_TOKEN_NEW_TOKEN,  // Always add at end
} Xvr_TokenType;
```

### 2. Parse Rules Table (xvr_parser.cpp)

The `parseRules[]` array maps each token type to parsing behavior:

```c
static ParseRule parseRules[] = {
    // Format: {prefix_fn, infix_fn, precedence}
    {atomic, NULL, PREC_PRIMARY},     // XVR_TOKEN_NULL
    // ...
};
```

- **prefix**: Function called when token appears in prefix position (e.g., `-x`)
- **infix**: Function called when token appears in infix position (e.g., `a + b`)
- **precedence**: Precedence level for infix expressions

**CRITICAL**: The order in `parseRules[]` MUST match `Xvr_TokenType` enum exactly.

> **Common Pitfall**: Misaligned parseRules order causes subtle bugs where tokens are dispatched to wrong handlers.
> 
> **Example**: In a recent fix, `TOKEN_IMPORT`, `TOKEN_INCLUDE`, `TOKEN_IN`, `TOKEN_OF`, `TOKEN_PRINT` were in wrong order in `parseRules[]`, causing `print()` and other keywords to be misidentified. This led to "unexpected token" errors and crashes.
>
> **Rule**: Always verify that `parseRules[index]` corresponds to `Xvr_TokenType` enum value `index`. Add tokens to BOTH places in the same order.

### 3. Namespace Support (xvr_namespace.h/cpp)

The namespace system allows registering namespaces and their members:

```c
// Register a namespace
Xvr_NamespaceRegister("math");

// Add members
Xvr_NamespaceAddMember("math", "sqrt", XVR_NS_MEMBER_FUNCTION, (void*)"sqrt");

// Check if access is valid
bool valid = Xvr_NamespaceIsValidAccess("math", "sqrt");
```

Built-in namespaces (`std`, `math`) are registered automatically via `Xvr_NamespaceRegisterBuiltins()`.

## How to Add a New Language Feature

### Example: Adding a new token

1. **Add token type** to `xvr_token_types.h`:
   ```c
   XVR_TOKEN_MY_NEW_TOKEN,  // Add at END of enum
   ```

2. **Add parse rule** to `parseRules[]` in `xvr_parser.cpp`:
   ```c
   {myNewTokenPrefix, myNewTokenInfix, PREC_MY_LEVEL}, // XVR_TOKEN_MY_NEW_TOKEN
   ```

3. **Implement parsing functions**:
   ```c
   static Xvr_Opcode myNewTokenPrefix(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
       // Parse the token in prefix position
   }
   ```

### Example: Adding a new namespace

1. **Register in `xvr_namespace.cpp`**:
   ```c
   void Xvr_NamespaceRegisterBuiltins(void) {
       Xvr_NamespaceRegister("myns");
       Xvr_NamespaceAddMember("myns", "myfunc", XVR_NS_MEMBER_FUNCTION, (void*)"myfunc");
   }
   ```

2. **Handle in parser** (in `convertFnCallToDot` or similar):
   Check if the namespace is valid using `Xvr_NamespaceIsValidAccess()`.

## AST Node Types

Key AST node types for stage0:

- `XVR_AST_NODE_LITERAL`: Literal values (integers, floats, strings)
- `XVR_AST_NODE_BINARY`: Binary expressions (a + b, a.b)
- `XVR_AST_NODE_FN_CALL`: Function call nodes
- `XVR_AST_NODE_FN_COLLECTION`: Function arguments (new parser structure)

## Common Patterns

### Handling `std::print()`-style calls

The parser now creates a DOT binary structure for namespace access:

```c
// AST structure for std::print(42)
// DOT(std, FN_CALL(print, args))
```

The LLVM emitter checks for this structure in `emit_std_print()`.

### Error Handling

Use `error(parser, token, "message")` to report parse errors. The parser uses a panic mode to recover.

## Stage0 Bootstrap Considerations

When building the stage0 compiler (written in XVR):

1. **Keep it simple**: The stage0 parser should be a simplified version of this one
2. **Maintain compatibility**: Token types and AST structure should be compatible
3. **Test thoroughly**: The bootstrap process is delicate - test each step

## Debugging Tips

- Enable debug output with `-d` flag
- Check `parseRules[]` ordering if new tokens aren't recognized
- Use `Xvr_ASTNodeType` values to debug AST structure
- The `convertFnCallToDot()` function handles legacy `.` operator - prefer `::` for new code

## File Organization

```
src/
├── xvr_token_types.h      # Token type definitions
├── xvr_parser.cpp        # Main parser implementation
├── xvr_namespace.h        # Namespace API
├── xvr_namespace.cpp      # Namespace implementation
├── xvr_ast_node.h       # AST node types
└── adapters/llvm/
    └── xvr_llvm_expression_emitter.cpp  # LLVM code generation
```
