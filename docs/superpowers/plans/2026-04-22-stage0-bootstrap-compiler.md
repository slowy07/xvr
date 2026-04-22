# Stage0 Bootstrap Compiler Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create stage0 bootstrap compiler in XVR that emits LLVM IR, with TDD testing and fuzzer integration.

**Architecture:** Modular compiler with lexer → parser → AST → codegen pipeline. Each component has single responsibility (SOC) and minimal interfaces (LOD).

**Tech Stack:** XVR language, LLVM IR, Catch2 for testing, libFuzzer for fuzzing

---

## File Structure

```
stage0/
├── src/
│   ├── token.xvr          # Token types (enum-like)
│   ├── lexer.xvr          # Lexer implementation
│   ├── ast.xvr            # AST node definitions
│   ├── parser.xvr         # Parser implementation
│   ├── codegen.xvr        # LLVM IR code generation
│   └── main.xvr           # CLI entry point
├── tests/
│   ├── test_lexer.xvr     # Lexer TDD tests
│   ├── test_parser.xvr    # Parser TDD tests
│   └── test_codegen.xvr   # Codegen TDD tests
├── fuzzer/
│   ├── fuzz_lexer.c       # Lexer fuzzer harness
│   ├── fuzz_parser.c      # Parser fuzzer harness
│   └── corpus/            # Fuzz test corpus
└── build.sh               # Build script
```

---

### Task 1: Project Setup & Token Types

**Files:**
- Create: `stage0/build.sh`
- Create: `stage0/src/token.xvr`

- [ ] **Step 1: Create build.sh**

```bash
#!/bin/bash
set -e

XVR="${XVR:-./build/xvr}"
STAGE0_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Building stage0..."
$XVR "$STAGE0_DIR/src/main.xvr" -o "$STAGE0_DIR/stage0" || exit 1

echo "Stage0 binary created: $STAGE0_DIR/stage0"
```

- [ ] **Step 2: Create token.xvr**

```xvr
namespace Token {

enum Type {
    EOF,
    INVALID,
    IDENT,
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,

    KW_VAR,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_RETURN,
    KW_PROC,
    KW_TRUE,
    KW_FALSE,
    KW_INCLUDE,
    KW_VOID,

    OP_ASSIGN,
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_PERCENT,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_AND,
    OP_OR,
    OP_NOT,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COMMA,
    SEMICOLON,
    COLON,
}

struct Token {
    type: Type,
    lexeme: string,
    line: int,
}

}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p stage0/src stage0/tests stage0/fuzzer/corpus
git add stage0/
git commit -m "feat: add stage0 project structure and token types"
```

---

### Task 2: Lexer Implementation (SOC - Single Responsibility)

**Files:**
- Create: `stage0/src/lexer.xvr`

- [ ] **Step 1: Write failing test in tests/test_lexer.xvr**

```xvr
include std;

proc test_lexer_int_literal() {
    var result = Lexer.lex("42");
    std::println("Token count: {}", result.length);
}

test_lexer_int_literal();
```

- [ ] **Step 2: Run test to verify it fails**

```bash
./build/xvr stage0/tests/test_lexer.xvr 2>&1
```

Expected: FAIL - "Lexer not defined"

- [ ] **Step 3: Write minimal lexer.xvr**

```xvr
namespace Lexer {

struct LexerState {
    source: string,
    pos: int,
    line: int,
}

proc init(source: string) -> LexerState {
    var state: LexerState;
    state.source = source;
    state.pos = 0;
    state.line = 1;
    return state;
}

proc lex(source: string) -> [Token::Token] {
    var state = init(source);
    var tokens: [Token::Token];

    while (state.pos < state.source.length) {
        var tok = nextToken(state);
        tokens = tokens + [tok];
    }

    var eof: Token::Token;
    eof.type = Token::Type.EOF;
    eof.lexeme = "";
    eof.line = state.line;
    tokens = tokens + [eof];

    return tokens;
}

proc nextToken(state: LexerState) -> Token::Token {
    var tok: Token::Token;
    tok.line = state.line;

    if (isDigit(state.source[state.pos])) {
        tok = readNumber(state);
    } else if (isAlpha(state.source[state.pos])) {
        tok = readIdentifier(state);
    } else {
        tok.type = Token::Type.INVALID;
        tok.lexeme = state.source[state.pos];
        state.pos = state.pos + 1;
    }

    return tok;
}

proc readNumber(state: LexerState) -> Token::Token {
    var tok: Token::Token;
    var start = state.pos;

    while (state.pos < state.source.length && isDigit(state.source[state.pos])) {
        state.pos = state.pos + 1;
    }

    tok.type = Token::Type.INT_LITERAL;
    tok.lexeme = state.source.subString(start, state.pos - start);
    tok.line = state.line;
    return tok;
}

proc readIdentifier(state: LexerState) -> Token::Token {
    var tok: Token::Token;
    var start = state.pos;

    while (state.pos < state.source.length && isAlphaNum(state.source[state.pos])) {
        state.pos = state.pos + 1;
    }

    tok.lexeme = state.source.subString(start, state.pos - start);
    tok.type = Token::Type.IDENT;
    tok.line = state.line;

    if (tok.lexeme == "var") {
        tok.type = Token::Type.KW_VAR;
    } else if (tok.lexeme == "if") {
        tok.type = Token::Type.KW_IF;
    } else if (tok.lexeme == "else") {
        tok.type = Token::Type.KW_ELSE;
    } else if (tok.lexeme == "while") {
        tok.type = Token::Type.KW_WHILE;
    } else if (tok.lexeme == "for") {
        tok.type = Token::Type.KW_FOR;
    } else if (tok.lexeme == "return") {
        tok.type = Token::Type.KW_RETURN;
    } else if (tok.lexeme == "proc") {
        tok.type = Token::Type.KW_PROC;
    } else if (tok.lexeme == "true") {
        tok.type = Token::Type.KW_TRUE;
    } else if (tok.lexeme == "false") {
        tok.type = Token::Type.KW_FALSE;
    }

    return tok;
}

proc isDigit(c: string) -> bool {
    return c >= "0" && c <= "9";
}

proc isAlpha(c: string) -> bool {
    return (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || c == "_";
}

proc isAlphaNum(c: string) -> bool {
    return isDigit(c) || isAlpha(c);
}

}
```

- [ ] **Step 4: Run test to verify it passes**

```bash
./build/xvr stage0/tests/test_lexer.xvr 2>&1
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add stage0/src/lexer.xvr stage0/tests/test_lexer.xvr
git commit -m "feat: add lexer implementation"
```

---

### Task 3: Parser Implementation (SOC - Single Responsibility)

**Files:**
- Create: `stage0/src/ast.xvr`
- Create: `stage0/src/parser.xvr`

- [ ] **Step 1: Write failing test in tests/test_parser.xvr**

```xvr
include std;

proc test_parser_simple_expr() {
    var tokens = Lexer.lex("42 + 5");
    var ast = Parser.parse(tokens);
    std::println("AST nodes: {}", ast.length);
}

test_parser_simple_expr();
```

- [ ] **Step 2: Run test to verify it fails**

```bash
./build/xvr stage0/tests/test_parser.xvr 2>&1
```

Expected: FAIL - "Parser not defined"

- [ ] **Step 3: Write ast.xvr**

```xvr
namespace AST {

enum NodeKind {
    PROGRAM,
    EXPR_STMT,
    BINARY_EXPR,
    UNARY_EXPR,
    LITERAL,
    IDENTIFIER,
    VARIABLE_DECL,
    FUNCTION_DECL,
    BLOCK,
    IF_STMT,
    WHILE_STMT,
    RETURN_STMT,
}

struct Node {
    kind: NodeKind,
    value: string,
    left: *Node,
    right: *Node,
    children: [*Node],
}

proc node(kind: NodeKind) -> Node {
    var n: Node;
    n.kind = kind;
    n.value = "";
    return n;
}

}
```

- [ ] **Step 4: Write parser.xvr**

```xvr
namespace Parser {

struct ParserState {
    tokens: [Token::Token],
    pos: int,
}

proc init(tokens: [Token::Token]) -> ParserState {
    var state: ParserState;
    state.tokens = tokens;
    state.pos = 0;
    return state;
}

proc parse(tokens: [Token::Token]) -> [AST::Node] {
    var state = init(tokens);
    var nodes: [AST::Node];

    while (state.pos < state.tokens.length) {
        var node = parseExpression(state);
        nodes = nodes + [node];
    }

    return nodes;
}

proc parseExpression(state: ParserState) -> AST::Node {
    var tok = state.tokens[state.pos];

    if (tok.type == Token::Type.INT_LITERAL) {
        state.pos = state.pos + 1;
        var n = AST::node(AST::NodeKind.LITERAL);
        n.value = tok.lexeme;
        return n;
    }

    if (tok.type == Token::Type.IDENT) {
        state.pos = state.pos + 1;
        var n = AST::node(AST::NodeKind.IDENTIFIER);
        n.value = tok.lexeme;
        return n;
    }

    var n = AST::node(AST::NodeKind.EXPR_STMT);
    n.value = "unknown";
    return n;
}

}
```

- [ ] **Step 5: Run test to verify it passes**

```bash
./build/xvr stage0/tests/test_parser.xvr 2>&1
```

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add stage0/src/ast.xvr stage0/src/parser.xvr stage0/tests/test_parser.xvr
git commit -m "feat: add parser implementation"
```

---

### Task 4: LLVM Codegen Implementation (SOC - Single Responsibility)

**Files:**
- Create: `stage0/src/codegen.xvr`

- [ ] **Step 1: Write failing test in tests/test_codegen.xvr**

```xvr
include std;

proc test_codegen_int() {
    var tokens = Lexer.lex("42");
    var ast = Parser.parse(tokens);
    var ir = CodeGen.generate(ast);
    std::println("IR: {}", ir);
}

test_codegen_int();
```

- [ ] **Step 2: Run test to verify it fails**

```bash
./build/xvr stage0/tests/test_codegen.xvr 2>&1
```

Expected: FAIL - "CodeGen not defined"

- [ ] **Step 3: Write codegen.xvr**

```xvr
namespace CodeGen {

proc generate(ast: [AST::Node]) -> string {
    var ir = "; Module ID 'stage0'\n";
    ir = ir + "source_filename = \"stage0\"\n\n";

    ir = ir + "define i32 @main() {\n";
    ir = ir + "entry:\n";

    var i = 0;
    while (i < ast.length) {
        ir = ir + generateNode(ast[i]);
        i = i + 1;
    }

    ir = ir + "  ret i32 0\n";
    ir = ir + "}\n";

    return ir;
}

proc generateNode(node: AST::Node) -> string {
    var code = "";

    if (node.kind == AST::NodeKind.LITERAL) {
        code = "  ; literal: " + node.value + "\n";
    } else if (node.kind == AST::NodeKind.IDENTIFIER) {
        code = "  ; identifier: " + node.value + "\n";
    } else {
        code = "  ; unknown node\n";
    }

    return code;
}

}
```

- [ ] **Step 4: Run test to verify it passes**

```bash
./build/xvr stage0/tests/test_codegen.xvr 2>&1
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add stage0/src/codegen.xvr stage0/tests/test_codegen.xvr
git commit -m "feat: add codegen implementation"
```

---

### Task 5: Main Entry Point & CLI

**Files:**
- Create: `stage0/src/main.xvr`

- [ ] **Step 1: Write main.xvr**

```xvr
include std;

proc main(args: [string]) -> int {
    if (args.length < 2) {
        std::println("Usage: stage0 <source.xvr>");
        return 1;
    }

    var sourcePath = args[1];
    std::println("Compiling: {}", sourcePath);

    var tokens = Lexer.lex(sourcePath);
    std::println("Tokens: {}", tokens.length);

    var ast = Parser.parse(tokens);
    std::println("AST nodes: {}", ast.length);

    var ir = CodeGen.generate(ast);
    std::println("Generated LLVM IR:\n{}", ir);

    return 0;
}
```

- [ ] **Step 2: Build and test**

```bash
./build/xvr stage0/src/main.xvr -o stage0/stage0
./stage0/stage0 "var x = 42;"
```

- [ ] **Step 3: Commit**

```bash
git add stage0/src/main.xvr
git commit -m "feat: add main entry point"
```

---

### Task 6: Fuzzer Integration

**Files:**
- Create: `stage0/fuzzer/fuzz_lexer.c`
- Create: `stage0/fuzzer/fuzz_parser.c`
- Create: `stage0/fuzzer/build.sh`

- [ ] **Step 1: Write fuzzer harness for lexer**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This will be called by libFuzzer
int LLVMFuzzerTestOneInput(const char *data, size_t size) {
    char *source = malloc(size + 1);
    memcpy(source, data, size);
    source[size] = '\0';

    // Stage0 lexer expects string input
    // Run lexer through XVR runtime or test harness
    
    free(source);
    return 0;
}
```

- [ ] **Step 2: Write build.sh for fuzzer**

```bash
#!/bin/bash
set -e

FUZZER_DIR="$(cd "$(dirname "$0")" && pwd)"
CC="${CC:-clang}"

echo "Building lexer fuzzer..."
$CC -fsanitize=fuzzer,address \
    "$FUZZER_DIR/fuzz_lexer.c" \
    -o "$FUZZER_DIR/fuzz_lexer" \
    -lm

echo "Fuzzer built: $FUZZER_DIR/fuzz_lexer"
echo "Run with: $FUZZER_DIR/fuzz_lexer"
```

- [ ] **Step 3: Test fuzzer builds**

```bash
cd stage0/fuzzer
./build.sh
```

- [ ] **Step 4: Create initial corpus**

```bash
mkdir -p stage0/fuzzer/corpus
echo -n "1" > stage0/fuzzer/corpus/01_int.txt
echo -n "abc" > stage0/fuzzer/corpus/02_ident.txt
echo -n "1 + 2" > stage0/fuzzer/corpus/03_expr.txt
```

- [ ] **Step 5: Run fuzzer**

```bash
./stage0/fuzzer/fuzz_lexer stage0/fuzzer/corpus/ 2>&1 | head -20
```

- [ ] **Step 6: Commit**

```bash
git add stage0/fuzzer/
git commit -m "feat: add fuzzer integration"
```

---

### Task 7: Extended Types Support

**Files:**
- Modify: `stage0/src/token.xvr`
- Modify: `stage0/src/lexer.xvr`
- Modify: `stage0/src/codegen.xvr`

- [ ] **Step 1: Add float, bool, string to tokens**

```xvr
// In token.xvr - add to enum:
FLOAT_LITERAL,
BOOL_LITERAL,
STRING_LITERAL,
```

- [ ] **Step 2: Add lexer support for floats**

```xvr
// In lexer.xvr - modify readNumber:
proc readNumber(state: LexerState) -> Token::Token {
    var tok: Token::Token;
    var start = state.pos;
    var hasDot = false;

    while (state.pos < state.source.length) {
        var c = state.source[state.pos];
        if (c == ".") {
            if (hasDot) { break; }
            hasDot = true;
        } else if (!isDigit(c)) {
            break;
        }
        state.pos = state.pos + 1;
    }

    tok.type = if (hasDot) { Token::Type.FLOAT_LITERAL } else { Token::Type.INT_LITERAL };
    tok.lexeme = state.source.subString(start, state.pos - start);
    tok.line = state.line;
    return tok;
}
```

- [ ] **Step 3: Add codegen for int/float literals**

```xvr
proc generateNode(node: AST::Node) -> string {
    var code = "";

    if (node.kind == AST::NodeKind.LITERAL) {
        // Check if int or float by presence of '.'
        if (node.value.contains(".")) {
            code = "  %result = fptrunc double " + node.value + " to float\n";
        } else {
            code = "  %result = add i32 " + node.value + ", 0\n";
        }
    }

    return code;
}
```

- [ ] **Step 4: Add tests for new types**

```xvr
proc test_float_literal() {
    var tokens = Lexer.lex("3.14");
    std::println("Float token type: {}", tokens[0].type);
}

test_float_literal();
```

- [ ] **Step 5: Run all tests**

```bash
./build/xvr stage0/tests/test_lexer.xvr
./build/xvr stage0/tests/test_codegen.xvr
```

- [ ] **Step 6: Commit**

```bash
git add stage0/
git commit -m "feat: add float type support to stage0"
```

---

### Task 8: Variables & Assignment

**Files:**
- Modify: `stage0/src/ast.xvr`
- Modify: `stage0/src/parser.xvr`
- Modify: `stage0/src/codegen.xvr`

- [ ] **Step 1: Add variable declaration to AST**

```xvr
// In ast.xvr - add to Node struct:
name: string,
type: string,
```

- [ ] **Step 2: Add parser for var decl**

```xvr
proc parseVariableDecl(state: ParserState) -> AST::Node {
    state.pos = state.pos + 1; // skip 'var'
    
    var nameTok = state.tokens[state.pos];
    state.pos = state.pos + 1;
    
    state.pos = state.pos + 1; // skip '='
    
    var value = parseExpression(state);
    
    var n = AST::node(AST::NodeKind.VARIABLE_DECL);
    n.name = nameTok.lexeme;
    n.value = value.value;
    return n;
}
```

- [ ] **Step 3: Add codegen for alloca + store**

```xvr
proc generateNode(node: AST::Node) -> string {
    var code = "";

    if (node.kind == AST::NodeKind.VARIABLE_DECL) {
        code = "  %" + node.name + " = alloca i32\n";
        code = code + "  store i32 " + node.value + ", i32* %" + node.name + "\n";
    }

    return code;
}
```

- [ ] **Step 4: Add test**

```xvr
proc test_var_decl() {
    var tokens = Lexer.lex("var x = 42");
    var ast = Parser.parse(tokens);
    var ir = CodeGen.generate(ast);
    std::println("{}", ir);
}
```

- [ ] **Step 5: Commit**

```bash
git add stage0/
git commit -m "feat: add variable declarations to stage0"
```

---

### Task 9: Functions

**Files:**
- Modify: `stage0/src/ast.xvr`
- Modify: `stage0/src/parser.xvr`
- Modify: `stage0/src/codegen.xvr`

- [ ] **Step 1: Add function declaration to AST**

- [ ] **Step 2: Add parser for proc**

- [ ] **Step 3: Add codegen for function definitions**

- [ ] **Step 4: Add test**

- [ ] **Step 5: Commit**

---

### Task 10: Control Flow (if/while)

**Files:**
- Modify: `stage0/src/ast.xvr`
- Modify: `stage0/src/parser.xvr`
- Modify: `stage0/src/codegen.xvr`

- [ ] **Step 1: Add if/while to AST**

- [ ] **Step 2: Add parser for if/while**

- [ ] **Step 3: Add codegen with br/condbr**

- [ ] **Step 4: Add test**

- [ ] **Step 5: Commit**

---

### Task 11: Full Integration Test

**Files:**
- Create: `stage0/examples/fib.xvr`

- [ ] **Step 1: Create fibonacci example**

```xvr
proc fib(n: int) -> int {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

proc main() -> int {
    var result = fib(10);
    return result;
}
```

- [ ] **Step 2: Compile and run**

```bash
./stage0/stage0 stage0/examples/fib.xvr > fib.ll
llc fib.ll -o fib.s
gcc fib.s -o fib
./fib
echo $?
```

Expected: 55 (10th fibonacci number)

- [ ] **Step 3: Commit**

```bash
git add stage0/examples/
git commit -m "feat: add fibonacci example"
```

---

## Self-Review Checklist

- [ ] All 11 tasks have clear file paths
- [ ] Each task has TDD steps (test → fail → implement → pass)
- [ ] LOD applied: each module has single responsibility
- [ ] SOC applied: lexer, parser, codegen separated
- [ ] Fuzzer integration in Task 6
- [ ] Types (int, float, bool, string) in Tasks 1-7
- [ ] Variables in Task 8
- [ ] Functions in Task 9
- [ ] Control flow in Task 10
- [ ] Full integration in Task 11
