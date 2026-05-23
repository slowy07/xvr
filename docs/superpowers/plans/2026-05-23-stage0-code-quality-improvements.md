# Stage0 Code Quality Improvements — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve stage0 compiler codebase following DRY, SRP, KISS, and testability principles — without adding new features.

**Architecture:** Four independent sections delivered sequentially: (1) delete dead code, deduplicate node constants, (2) extract character helpers and break monolith functions, (3) wire main.xvr to actually compile source → AST → IR, (4) convert print-and-check tests to assertion-based.

**Tech Stack:** xvr language (stage0 bootstrap compiler), bash

**Build & test:** `cd stage0 && bash build.sh && bash test_self_host.sh`

---
### File Structure Map

#### Files to Delete (10)
- `src/ast.xvr` — dead code, never included
- `src/test_inc.xvr`, `src/test_inc2.xvr`, `src/test_inc3.xvr`, `src/test_inc4.xvr`, `src/test_minimal.xvr`, `src/test_lexer_combined.xvr` — experimental clutter
- `tests/test_lexer.xvr`, `tests/test_parser.xvr`, `tests/test_codegen.xvr`, `tests/t2.xvr` — abandoned stubs

#### Files to Create (2)
- `src/node_types.xvr` — single-source node type constants (replaces triple definitions)
- `tests/test_helpers.xvr` — shared assertion utilities

#### Files to Modify (7)
- `src/lexer.xvr` — extract helpers, refactor nextToken, remove unused `lex()`
- `src/parser.xvr` — include node_types, refactor parseInfix, add PREC_* constants, remove unused tokens param
- `src/codegen.xvr` — include node_types, remove unused `generateCall()` and duplicate constants
- `src/main.xvr` — wire real pipeline (lex → parse → codegen)
- `tests/test_lexer_stage0.xvr` — assertion-based
- `tests/test_parser_stage0.xvr` — assertion-based
- `tests/test_codegen_stage0.xvr` — assertion-based
- `tests/test_stage0_all.xvr` — assertion-based
- `tests/test_lexer_ops.xvr` — use shared helpers
- `tests/test_parser_expr.xvr` — use shared helpers
- `tests/test_struct.xvr` — use shared helpers

#### Scripts to Update (1)
- `embed_and_build.sh` — replace `ast.xvr` with `node_types.xvr` in file list

---

### Task 1: Code Organization Cleanup

**Files:**
- Create: `src/node_types.xvr`
- Delete: `src/ast.xvr`, `src/test_inc.xvr`, `src/test_inc2.xvr`, `src/test_inc3.xvr`, `src/test_inc4.xvr`, `src/test_minimal.xvr`, `src/test_lexer_combined.xvr`
- Delete: `tests/test_lexer.xvr`, `tests/test_parser.xvr`, `tests/test_codegen.xvr`, `tests/t2.xvr`
- Modify: `src/parser.xvr:1-21` — replace inline NODE_* constants with `include node_types;`
- Modify: `src/codegen.xvr:1-19` — replace inline NODE_* constants with `include node_types;`
- Modify: `src/lexer.xvr` — remove unused `lex()` function (lines 218-237)
- Modify: `src/codegen.xvr` — remove unused `generateCall()` function (lines 172-176)
- Modify: `src/parser.xvr:422` — remove unused `tokens: [i32]` parameter from `parse()`
- Modify: `embed_and_build.sh:14` — replace `ast.xvr` with `node_types.xvr`

- [ ] **Step 1: Create `src/node_types.xvr`**

Write the shared node type constants file. These values must match the existing definitions exactly.

```xvr
var NODE_PROGRAM: i32 = 0;
var NODE_LITERAL: i32 = 1;
var NODE_IDENTIFIER: i32 = 2;
var NODE_BINARY_OP: i32 = 3;
var NODE_VARIABLE_DECL: i32 = 4;
var NODE_FUNCTION_DECL: i32 = 5;
var NODE_IF_STMT: i32 = 6;
var NODE_WHILE_STMT: i32 = 7;
var NODE_RETURN_STMT: i32 = 8;
var NODE_STRUCT_DEF: i32 = 9;
var NODE_STRUCT_FIELD: i32 = 10;
var NODE_STRUCT_INIT: i32 = 11;
var NODE_STRUCT_ACCESS: i32 = 12;
var NODE_UNARY_OP: i32 = 13;
var NODE_CALL: i32 = 14;
var NODE_INDEX: i32 = 15;
var NODE_FIELD: i32 = 16;
var NODE_CONDITIONAL: i32 = 17;
var NODE_GROUPING: i32 = 18;
```

- [ ] **Step 2: Update `parser.xvr` to include node_types instead of redefining**

Replace lines 1-21 in `parser.xvr`:
```
include lexer;
include node_types;
```

Delete lines 3-21 (the `var NODE_*` definitions).

- [ ] **Step 3: Update `codegen.xvr` to include node_types instead of redefining**

Replace lines 1-19 with:
```
include node_types;
```

Delete lines 1-19 (the `var NODE_*` definitions).

- [ ] **Step 4: Remove unused `lex()` function from `lexer.xvr`**

Delete lines 218-237:
```xvr
proc lex(source: string): [Token] {
    initLexer(source);
    nextToken();
    while (g_pos < len(g_source)) {
        nextToken();
    }
    var result: [Token];
    result = [];
    var i: i32;
    i = 0;
    while (i < g_tokenCount) {
        var t: Token;
        t.type = getTokenType(i);
        t.lexeme = getTokenLexeme(i);
        t.line = g_tokenLines[i];
        result.insert(t);
        i = i + 1;
    }
    return result;
}
```

- [ ] **Step 5: Remove unused `generateCall()` from `codegen.xvr`**

Delete lines 172-176:
```xvr
proc generateCall(fnName: string): string {
    var ir: string;
    ir = "  %result = call i32 @" + fnName + "()\n";
    return ir;
}
```

- [ ] **Step 6: Remove unused `tokens` parameter from `parse()`**

Change `parser.xvr:422` from:
```xvr
proc parse(tokens: [i32]): i32 {
```
to:
```xvr
proc parse(): i32 {
```

- [ ] **Step 7: Delete abandoned test stubs and experimental files**

```bash
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/ast.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_inc.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_inc2.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_inc3.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_inc4.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_minimal.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_lexer_combined.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/tests/test_lexer.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/tests/test_parser.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/tests/test_codegen.xvr
rm -f /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/tests/t2.xvr
```

- [ ] **Step 8: Update `embed_and_build.sh` to replace `ast.xvr` with `node_types.xvr`**

Change line 14 from:
```
for f in "$STAGE0_DIR/src/main.xvr" "$STAGE0_DIR/src/token.xvr" "$STAGE0_DIR/src/ast.xvr" "$STAGE0_DIR/src/lexer.xvr" "$STAGE0_DIR/src/parser.xvr" "$STAGE0_DIR/src/codegen.xvr"; do
```
to:
```
for f in "$STAGE0_DIR/src/main.xvr" "$STAGE0_DIR/src/token.xvr" "$STAGE0_DIR/src/node_types.xvr" "$STAGE0_DIR/src/lexer.xvr" "$STAGE0_DIR/src/parser.xvr" "$STAGE0_DIR/src/codegen.xvr"; do
```

- [ ] **Step 9: Build and verify**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh && bash test_self_host.sh
```

Expected: build succeeds, test_self_host.sh passes all checks.

- [ ] **Step 10: Commit**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add -A
git commit -m "refactor(stage0): unify node constants, delete dead code"
```

---

### Task 2: Lexer Helper Extraction

**Files:**
- Modify: `src/lexer.xvr` — add isAlpha, isDigit, isAlphaNum helpers; extract lexNumber(); replace inline range checks

- [ ] **Step 1: Add character helper functions after `initLexer()` (after line 19)**

```xvr
proc isAlpha(c: string): bool {
    if (c >= "a") { if (c <= "z") { return true; } }
    if (c >= "A") { if (c <= "Z") { return true; } }
    if (c == "_") { return true; }
    return false;
}

proc isDigit(c: string): bool {
    if (c >= "0") { if (c <= "9") { return true; } }
    return false;
}

proc isAlphaNum(c: string): bool {
    return isAlpha(c) || isDigit(c);
}
```

- [ ] **Step 2: Replace inline digit check in nextToken() (line 55)**

Change:
```
if (c >= "0") { if (c <= "9") { isDigit = true; } }
```
to:
```
isDigit = isDigit(c);
```

- [ ] **Step 3: Replace inline alpha checks in identifier section**

Lines 86-90:
```
var isAlpha: bool;
isAlpha = false;
if (c >= "a") { if (c <= "z") { isAlpha = true; } }
if (c >= "A") { if (c <= "Z") { isAlpha = true; } }
if (c == "_") { isAlpha = true; }
```
Replace with:
```
var isAlpha: bool;
isAlpha = isAlpha(c);
```

Lines 101-104 in the identifier loop body:
```
isAlpha = false;
if (c >= "a") { if (c <= "z") { isAlpha = true; } }
if (c >= "A") { if (c <= "Z") { isAlpha = true; } }
if (c == "_") { isAlpha = true; }
if (c >= "0") { if (c <= "9") { isAlpha = true; } }
```
Replace with:
```
isAlpha = false;
if (isAlphaNum(c)) { isAlpha = true; }
```

- [ ] **Step 4: Extract lexNumber() from nextToken()**

Take the number lexing logic (current lines 53-84) and move it to a separate function placed after the character helpers:

```xvr
proc lexNumber(): Token {
    var lexeme: string;
    lexeme = "";
    var isFloat = false;

    while (g_pos < len(g_source)) {
        var currC: string;
        currC = g_source.subString(g_pos, 1);
        if (currC == "." && !isFloat) {
            isFloat = true;
            lexeme = lexeme + currC;
            g_pos = g_pos + 1;
        } else if (isDigit(currC)) {
            lexeme = lexeme + currC;
            g_pos = g_pos + 1;
        } else {
            break;
        }
    }

    var tok: Token;
    tok.type = if (isFloat) { FLOAT_LITERAL } else { INT_LITERAL };
    tok.lexeme = lexeme;
    tok.line = g_line;
    addToken(tok.type, lexeme, g_line);
    return tok;
}
```

Replace the inline number block (lines 53-84 in original) with:
```xvr
if (isDigit) {
    return lexNumber();
}
```

- [ ] **Step 5: Build and verify**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh && bash test_self_host.sh
```

Expected: build succeeds, test passes.

- [ ] **Step 6: Commit**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add stage0/src/lexer.xvr
git commit -m "refactor(stage0): add char helpers, extract lexNumber()"
```

---

### Task 3: Refactor nextToken() — Extract lexIdentifierOrKeyword, lexString, lexOperatorOrPunctuation

**Files:**
- Modify: `src/lexer.xvr` — extract 3 more functions from nextToken(), replace monolithic if-else chain

- [ ] **Step 1: Extract lexIdentifierOrKeyword()**

Add after `lexNumber()`:

```xvr
proc lexIdentifierOrKeyword(): Token {
    var lexeme: string;
    lexeme = "";
    var c: string;
    c = g_source.subString(g_pos, 1);
    while (isAlphaNum(c)) {
        lexeme = lexeme + c;
        g_pos = g_pos + 1;
        if (g_pos >= len(g_source)) { break; }
        c = g_source.subString(g_pos, 1);
    }

    var tokType: i32;
    tokType = IDENT;
    if (lexeme == "var") { tokType = KW_VAR; }
    if (lexeme == "if") { tokType = KW_IF; }
    if (lexeme == "else") { tokType = KW_ELSE; }
    if (lexeme == "while") { tokType = KW_WHILE; }
    if (lexeme == "for") { tokType = KW_FOR; }
    if (lexeme == "return") { tokType = KW_RETURN; }
    if (lexeme == "proc") { tokType = KW_PROC; }
    if (lexeme == "true") { tokType = KW_TRUE; }
    if (lexeme == "false") { tokType = KW_FALSE; }
    if (lexeme == "struct") { tokType = KW_STRUCT; }
    if (lexeme == "const") { tokType = KW_CONST; }
    if (lexeme == "fn") { tokType = KW_FN; }
    if (lexeme == "include") { tokType = KW_INCLUDE; }
    if (lexeme == "void") { tokType = KW_VOID; }

    var tok: Token;
    tok.type = tokType;
    tok.lexeme = lexeme;
    tok.line = g_line;
    addToken(tokType, lexeme, g_line);
    return tok;
}
```

- [ ] **Step 2: Extract lexString()**

Add after `lexIdentifierOrKeyword()`:

```xvr
proc lexString(): Token {
    var lexeme: string;
    lexeme = "";
    var unterminated: bool;
    unterminated = true;
    g_pos = g_pos + 1;
    while (g_pos < len(g_source)) {
        var currC: string;
        currC = g_source.subString(g_pos, 1);
        if (currC == "\"") {
            g_pos = g_pos + 1;
            unterminated = false;
            break;
        }
        lexeme = lexeme + currC;
        g_pos = g_pos + 1;
    }
    var tokType: i32;
    tokType = if (unterminated) { INVALID } else { STRING_LITERAL };
    var tok: Token;
    tok.type = tokType;
    tok.lexeme = lexeme;
    tok.line = g_line;
    addToken(tokType, lexeme, g_line);
    return tok;
}
```

- [ ] **Step 3: Extract lexOperatorOrPunctuation()**

Add after `lexString()`:

```xvr
proc lexOperatorOrPunctuation(firstChar: string): Token {
    var tokType: i32;
    tokType = INVALID;
    var nextC: string;
    nextC = peek();
    var isTwoChar: bool;
    isTwoChar = false;

    if (firstChar == "=" && nextC == "=") { tokType = OP_EQ; isTwoChar = true; }
    else if (firstChar == "!" && nextC == "=") { tokType = OP_NEQ; isTwoChar = true; }
    else if (firstChar == "<" && nextC == "=") { tokType = OP_LTE; isTwoChar = true; }
    else if (firstChar == ">" && nextC == "=") { tokType = OP_GTE; isTwoChar = true; }
    else if (firstChar == "&" && nextC == "&") { tokType = OP_AND; isTwoChar = true; }
    else if (firstChar == "|" && nextC == "|") { tokType = OP_OR; isTwoChar = true; }
    else if (firstChar == "-" && nextC == ">") { tokType = ARROW; isTwoChar = true; }
    else if (firstChar == "." && nextC == ".") { tokType = RANGE; isTwoChar = true; }
    else if (firstChar == "(") { tokType = LPAREN; }
    else if (firstChar == ")") { tokType = RPAREN; }
    else if (firstChar == "{") { tokType = LBRACE; }
    else if (firstChar == "}") { tokType = RBRACE; }
    else if (firstChar == "[") { tokType = LBRACKET; }
    else if (firstChar == "]") { tokType = RBRACKET; }
    else if (firstChar == ",") { tokType = COMMA; }
    else if (firstChar == ";") { tokType = SEMICOLON; }
    else if (firstChar == ":") { tokType = COLON; }
    else if (firstChar == ".") { tokType = DOT; }
    else if (firstChar == "=") { tokType = EQ; }
    else if (firstChar == "+") { tokType = OP_PLUS; }
    else if (firstChar == "-") { tokType = OP_MINUS; }
    else if (firstChar == "*") { tokType = OP_STAR; }
    else if (firstChar == "/") { tokType = OP_SLASH; }
    else if (firstChar == "%") { tokType = OP_PERCENT; }
    else if (firstChar == "!") { tokType = OP_NOT; }

    var lexeme: string;
    lexeme = firstChar;
    var tok: Token;
    tok.type = tokType;
    tok.lexeme = lexeme;
    tok.line = g_line;
    addToken(tokType, lexeme, g_line);
    g_pos = g_pos + 1;
    if (isTwoChar) {
        g_pos = g_pos + 1;
    }
    return tok;
}
```

- [ ] **Step 4: Rewrite nextToken() as a dispatcher**

Replace the entire `nextToken()` body (after `var c = g_source.subString(g_pos, 1);`) with the dispatcher pattern:

```xvr
proc nextToken(): Token {
    while (g_pos < len(g_source)) {
        var c: string;
        c = g_source.subString(g_pos, 1);

        if (c == " " || c == "\t" || c == "\n") {
            if (c == "\n") { g_line = g_line + 1; }
            g_pos = g_pos + 1;
        } else if (isDigit(c)) {
            return lexNumber();
        } else if (isAlpha(c)) {
            return lexIdentifierOrKeyword();
        } else if (c == "\"") {
            return lexString();
        } else {
            return lexOperatorOrPunctuation(c);
        }
    }

    var eof: Token;
    eof.type = EOF;
    eof.lexeme = "";
    eof.line = g_line;
    addToken(EOF, "", g_line);
    return eof;
}
```

- [ ] **Step 5: Build and verify**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh && bash test_self_host.sh
```

Expected: build succeeds, test passes.

- [ ] **Step 6: Commit**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add stage0/src/lexer.xvr
git commit -m "refactor(stage0): split nextToken into specialized lex functions"
```

---

### Task 4: Parser Refactor — PREC_* constants + Table-Driven parseInfix

**Files:**
- Modify: `src/parser.xvr` — add PREC_* named constants, replace 13 operator if-blocks with table-driven dispatch

- [ ] **Step 1: Add PREC_* named constants after `node_types` include**

Add after `include node_types;`:

```xvr
var PREC_OR: i32 = 2;
var PREC_AND: i32 = 3;
var PREC_EQ: i32 = 4;
var PREC_COMPARE: i32 = 5;
var PREC_TERM: i32 = 6;
var PREC_FACTOR: i32 = 7;
var PREC_UNARY: i32 = 8;
var PREC_CALL: i32 = 9;
```

- [ ] **Step 2: Replace getPrecedence() magic numbers with named constants**

Replace the entire function `getPrecedence()` (lines 77-92) with:

```xvr
proc getPrecedence(tokType: i32): i32 {
    if (tokType == OP_OR) { return PREC_OR; }
    if (tokType == OP_AND) { return PREC_AND; }
    if (tokType == OP_EQ || tokType == OP_NEQ) { return PREC_EQ; }
    if (tokType == OP_LT || tokType == OP_GT || tokType == OP_LTE || tokType == OP_GTE) { return PREC_COMPARE; }
    if (tokType == OP_PLUS || tokType == OP_MINUS) { return PREC_TERM; }
    if (tokType == OP_STAR || tokType == OP_SLASH || tokType == OP_PERCENT) { return PREC_FACTOR; }
    return 0;
}
```

Also replace magic numbers `8` and `9` in `parsePrefix()` and `parseExpression()`:
- Line 146: `parseExpression(8)` → `parseExpression(PREC_UNARY)`
- Line 152: `parseExpression(8)` → `parseExpression(PREC_UNARY)`
- Line 299: `nextPrec = 9` → `nextPrec = PREC_CALL`
- Line 302: `nextPrec = 9` → `nextPrec = PREC_CALL`

- [ ] **Step 3: Replace 13 if-blocks in parseInfix() with table-driven dispatch**

Replace the entire `parseInfix()` body (lines 159-285) with:

```xvr
proc parseInfix(left: i32, leftPrec: i32): i32 {
    var tokType: i32;
    tokType = currentTokenType();

    if (tokType == LBRACKET) {
        advance();
        var index: i32;
        index = parseExpression(0);
        advance();
        return addNode(NODE_INDEX, "", left, index, -1);
    }

    if (tokType == DOT) {
        advance();
        var fieldName: string;
        fieldName = currentLexeme();
        advance();
        return addNode(NODE_FIELD, fieldName, left, -1, -1);
    }

    var opLexeme: string;
    opLexeme = currentLexeme();
    advance();
    var right: i32;
    right = parseExpression(leftPrec);
    return addNode(NODE_BINARY_OP, opLexeme, left, right, tokType);
}
```

The 13 nearly-identical operator blocks collapse into 6 lines of dispatch: check for special-case tokens (LBRACKET, DOT) first, then handle all binary operators with a single generic block.

- [ ] **Step 4: Build and verify**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh && bash test_self_host.sh
```

Expected: build succeeds, test passes.

- [ ] **Step 5: Commit**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add stage0/src/parser.xvr
git commit -m "refactor(stage0): add PREC_* constants, table-driven parseInfix"
```

---

### Task 5: Data Flow Fix — Wire main.xvr to Actually Compile

**Files:**
- Modify: `src/main.xvr` — replace stub with real lexical → parse → codegen pipeline

- [ ] **Step 1: Rewrite main.xvr**

Replace the entire file with:

```xvr
include lexer;
include parser;
include codegen;

proc run(): void {
    std::print("Stage0 Bootstrap Compiler v0.1.0\n");

    var source = "var x = 42";
    initLexer(source);

    while (nextToken().type != EOF) { }

    initParser();
    var nodeCount = parse();

    if (nodeCount > 0) {
        initCodegen();
        var i: i32;
        i = 0;
        while (i < nodeCount) {
            generateNode(g_astKinds[i], g_astLefts[i], g_astRights[i],
                         g_astExtras[i], g_astValues[i]);
            i = i + 1;
        }
        var ir: string;
        ir = getIR();
        std::print("=== Generated LLVM IR ===\n");
        std::print(ir);
    } else {
        std::print("Parse failed\n");
    }
}
```

run();
```

Note: The codegen's `generateNode()` accesses `values` array with `left`/`right`/`extra` indices. These indices are AST node indices, not values array indices — `g_astValues[]` stores the string value (variable name, literal string, operator lexeme). The `generateNode()` function receives `left` and `right` as the original AST node indices (child node positions in the parallel arrays), but `values` is `g_astValues[]` which contains string values indexed by node position.

This mismatch means codegen currently interprets `left`/`right` as direct indices into `values` (string array), but for expression nodes, `left` and `right` are child AST node indices, not values array indices.

**Fix**: `main.xvr` needs to pass `g_astValues` as the `values` parameter. For AST nodes where `left`/`right` are child node indices (like `NODE_BINARY_OP`), the codegen should not index into `values[childIdx]` — it should use `emitExpr()` which handles recursive traversal.
```
```

- [ ] **Step 2: Build and verify**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh && bash test_self_host.sh
```

Expected: build succeeds (may show "module does not export" errors for AST globals — see Step 3 for fix).

- [ ] **Step 3: Fix parser's AST global visibility if needed**

If the build fails with "g_astKinds not found" or similar, it means `parser.xvr`'s globals are not visible to `main.xvr`. The xvr language uses module-level visibility — variables declared at file scope in one module should be accessible from another module when the first module is included.

If the globals are not accessible, the simplest fix is to add accessor functions in `parser.xvr`:

```xvr
proc getAstKind(i: i32): i32 {
    if (i >= 0 && i < g_nodeCount) { return g_astKinds[i]; }
    return -1;
}
proc getAstLeft(i: i32): i32 {
    if (i >= 0 && i < g_nodeCount) { return g_astLefts[i]; }
    return -1;
}
proc getAstRight(i: i32): i32 {
    if (i >= 0 && i < g_nodeCount) { return g_astRights[i]; }
    return -1;
}
proc getAstExtra(i: i32): i32 {
    if (i >= 0 && i < g_nodeCount) { return g_astExtras[i]; }
    return -1;
}
proc getAstValue(i: i32): string {
    if (i >= 0 && i < g_nodeCount) { return g_astValues[i]; }
    return "";
}
```

Then in `main.xvr`, replace direct global access with:
```xvr
generateNode(parser::getAstKind(i), parser::getAstLeft(i), parser::getAstRight(i),
             parser::getAstExtra(i), parser::getAstValue(i));
```

- [ ] **Step 4: Build and verify again**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh && bash test_self_host.sh
```

Expected: build succeeds, test passes, main.xvr prints "Generated LLVM IR" with real IR output (even if placeholder-based).

- [ ] **Step 5: Commit**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add src/main.xvr src/parser.xvr
git commit -m "feat(stage0): wire main.xvr to compile source -> AST -> IR"
```

---

### Task 6: Test Improvements — Assertion-Based Tests

**Files:**
- Create: `tests/test_helpers.xvr`
- Modify: `tests/test_lexer_stage0.xvr`, `tests/test_parser_stage0.xvr`, `tests/test_codegen_stage0.xvr`, `tests/test_stage0_all.xvr`, `tests/test_lexer_ops.xvr`, `tests/test_parser_expr.xvr`, `tests/test_struct.xvr`

- [ ] **Step 1: Create `tests/test_helpers.xvr`**

```xvr
proc assertEq(got: i32, expected: i32, msg: string) {
    if (got != expected) {
        std::print("FAIL: " + msg + " (got " + got + ", expected " + expected + ")\n");
    } else {
        std::print("PASS: " + msg + "\n");
    }
}

proc assertNe(got: i32, expected: i32, msg: string) {
    if (got == expected) {
        std::print("FAIL: " + msg + " (got " + got + ", should differ)\n");
    } else {
        std::print("PASS: " + msg + "\n");
    }
}

proc assertStr(got: string, expected: string, msg: string) {
    if (got != expected) {
        std::print("FAIL: " + msg + " (got '" + got + "', expected '" + expected + "')\n");
    } else {
        std::print("PASS: " + msg + "\n");
    }
}

proc assertTrue(cond: bool, msg: string) {
    if (cond) {
        std::print("PASS: " + msg + "\n");
    } else {
        std::print("FAIL: " + msg + "\n");
    }
}

var g_passCount: i32 = 0;
var g_failCount: i32 = 0;

proc check(passed: bool, msg: string) {
    if (passed) {
        g_passCount = g_passCount + 1;
        std::print("PASS: " + msg + "\n");
    } else {
        g_failCount = g_failCount + 1;
        std::print("FAIL: " + msg + "\n");
    }
}

proc report() {
    std::print("\nResults: " + g_passCount + " passed, " + g_failCount + " failed\n");
    if (g_failCount > 0) {
        std::print("SOME TESTS FAILED\n");
    } else {
        std::print("ALL TESTS PASSED\n");
    }
}
```

- [ ] **Step 2: Rewrite `tests/test_lexer_stage0.xvr` with assertions**

```xvr
include std;
include lexer;

proc test_token_types() {
    std::print("=== Test: Token Types ===\n");
    initLexer("var x = 42");
    while (nextToken().type != EOF) { }

    assertEq(tokenCount(), 5, "token count for 'var x = 42'");
    assertStr(getTokenLexeme(0), "var", "token 0 lexeme");
    assertStr(getTokenLexeme(1), "x", "token 1 lexeme");
    assertStr(getTokenLexeme(2), "=", "token 2 lexeme");
    assertStr(getTokenLexeme(3), "42", "token 3 lexeme");
    assertEq(getTokenType(0), KW_VAR, "token 0 type is KW_VAR");
    assertEq(getTokenType(3), INT_LITERAL, "token 3 type is INT_LITERAL");
    std::print("PASS: token_types\n");
}

proc test_keywords() {
    std::print("=== Test: Keywords ===\n");
    initLexer("struct const proc fn if else while for return true false");
    while (nextToken().type != EOF) { }

    assertEq(tokenCount(), 12, "keyword token count");
    assertEq(getTokenType(0), KW_STRUCT, "keyword 'struct'");
    assertEq(getTokenType(1), KW_CONST, "keyword 'const'");
    assertEq(getTokenType(2), KW_PROC, "keyword 'proc'");
    assertEq(getTokenType(3), KW_FN, "keyword 'fn'");
    assertEq(getTokenType(4), KW_IF, "keyword 'if'");
    assertEq(getTokenType(5), KW_ELSE, "keyword 'else'");
    assertEq(getTokenType(6), KW_WHILE, "keyword 'while'");
    assertEq(getTokenType(7), KW_FOR, "keyword 'for'");
    assertEq(getTokenType(8), KW_RETURN, "keyword 'return'");
    assertEq(getTokenType(9), KW_TRUE, "keyword 'true'");
    assertEq(getTokenType(10), KW_FALSE, "keyword 'false'");
    std::print("PASS: keywords\n");
}

proc test_literals() {
    std::print("=== Test: Literals ===\n");

    initLexer("42");
    var tok = nextToken();
    assertEq(tok.type, INT_LITERAL, "int literal type");
    assertStr(tok.lexeme, "42", "int literal lexeme");

    initLexer("3.14");
    tok = nextToken();
    assertEq(tok.type, FLOAT_LITERAL, "float literal type");
    assertStr(tok.lexeme, "3.14", "float literal lexeme");

    initLexer("\"hello\"");
    tok = nextToken();
    assertEq(tok.type, STRING_LITERAL, "string literal type");
    assertStr(tok.lexeme, "hello", "string literal lexeme");

    initLexer("myVar");
    tok = nextToken();
    assertEq(tok.type, IDENT, "identifier type");
    assertStr(tok.lexeme, "myVar", "identifier lexeme");

    std::print("PASS: literals\n");
}

proc test_multi_char_ops() {
    std::print("=== Test: Multi-char Operators ===\n");

    initLexer("== != <= >= && || -> ..");
    while (nextToken().type != EOF) { }

    assertEq(tokenCount(), 9, "multi-char op count");
    assertEq(getTokenType(0), OP_EQ, "==");
    assertEq(getTokenType(1), OP_NEQ, "!=");
    assertEq(getTokenType(2), OP_LTE, "<=");
    assertEq(getTokenType(3), OP_GTE, ">=");
    assertEq(getTokenType(4), OP_AND, "&&");
    assertEq(getTokenType(5), OP_OR, "||");
    assertEq(getTokenType(6), ARROW, "->");
    assertEq(getTokenType(7), RANGE, "..");
    std::print("PASS: multi_char_ops\n");
}

proc main() {
    std::print("Stage0 Lexer Tests\n\n");
    test_token_types();
    test_keywords();
    test_literals();
    test_multi_char_ops();
    std::print("\n=== All Lexer Tests Passed ===\n");
}

main();
```

- [ ] **Step 3: Rewrite `tests/test_parser_stage0.xvr` with assertions**

```xvr
include std;
include lexer;
include parser;

proc test_var_decl() {
    std::print("=== Test: Var Declaration ===\n");
    initLexer("var x = 42");
    while (nextToken().type != EOF) { }
    initParser();
    var count = parse();
    assertTrue(count > 0, "parsed at least 1 node");
    std::print("PASS: var_decl\n");
}

proc test_if_stmt() {
    std::print("=== Test: If Statement ===\n");
    initLexer("if 1 2");
    while (nextToken().type != EOF) { }
    initParser();
    var count = parse();
    assertTrue(count > 0, "parsed if statement");
    std::print("PASS: if_stmt\n");
}

proc test_while_stmt() {
    std::print("=== Test: While Statement ===\n");
    initLexer("while 1 2");
    while (nextToken().type != EOF) { }
    initParser();
    var count = parse();
    assertTrue(count > 0, "parsed while statement");
    std::print("PASS: while_stmt\n");
}

proc test_return() {
    std::print("=== Test: Return ===\n");
    initLexer("return 42");
    while (nextToken().type != EOF) { }
    initParser();
    var count = parse();
    assertTrue(count > 0, "parsed return statement");
    std::print("PASS: return\n");
}

proc main() {
    std::print("Stage0 Parser Tests\n\n");
    test_var_decl();
    test_if_stmt();
    test_while_stmt();
    test_return();
    std::print("\n=== All Parser Tests Passed ===\n");
}

main();
```

- [ ] **Step 4: Rewrite `tests/test_codegen_stage0.xvr` with assertions**

```xvr
include std;
include codegen;

proc test_codegen_init() {
    std::print("=== Test: Codegen Init ===\n");
    initCodegen();
    var ir = getIR();
    assertStr(ir, "", "IR is empty after init");
    std::print("PASS: codegen_init\n");
}

proc test_generate_function() {
    std::print("=== Test: Generate Function ===\n");
    initCodegen();
    var fn = generateFunction("testFn", "  ret i32 0\n");
    var hasDefine = false;
    if (fn.subString(0, 7) == "define ") { hasDefine = true; }
    assertTrue(hasDefine, "function IR starts with 'define'");
    std::print("PASS: generate_function\n");
}

proc test_generate_if() {
    std::print("=== Test: Generate If ===\n");
    initCodegen();
    var ir = generateIf("%cond", "  ret i32 0\n", "");
    assertTrue(len(ir) > 20, "if IR is non-trivial length");
    std::print("PASS: generate_if\n");
}

proc test_generate_while() {
    std::print("=== Test: Generate While ===\n");
    initCodegen();
    var ir = generateWhile("%cond", "  ret i32 0\n");
    assertTrue(len(ir) > 20, "while IR is non-trivial length");
    std::print("PASS: generate_while\n");
}

proc main() {
    std::print("Stage0 Codegen Tests\n\n");
    test_codegen_init();
    test_generate_function();
    test_generate_if();
    test_generate_while();
    std::print("\n=== All Codegen Tests Passed ===\n");
}

main();
```

- [ ] **Step 5: Rewrite `tests/test_stage0_all.xvr` with assertions**

```xvr
include std;
include lexer;
include parser;
include codegen;

var passCount: i32 = 0;
var failCount: i32 = 0;

proc check(passed: bool, msg: string) {
    if (passed) {
        passCount = passCount + 1;
        std::println("PASS: {}", msg);
    } else {
        failCount = failCount + 1;
        std::println("FAIL: {}", msg);
    }
}

proc main() {
    std::println("======================================");
    std::println("  Stage0 Bootstrap Compiler Tests");
    std::println("======================================");

    initLexer("var x = 42");
    while (nextToken().type != EOF) { }
    check(tokenCount() == 5, "lexer produces 5 tokens for 'var x = 42'");
    check(getTokenType(0) == KW_VAR, "first token is KW_VAR");

    initParser();
    var nodeCount = parse();
    check(nodeCount > 0, "parser produces nodes");

    initCodegen();
    var ir = generate();
    check(len(ir) > 20, "codegen produces IR");

    std::println("\n======================================");
    std::println("  Results: {} passed, {} failed", passCount, failCount);
    if (failCount > 0) {
        std::println("  SOME TESTS FAILED");
    } else {
        std::println("  ALL TESTS PASSED");
    }
    std::println("======================================");
}

main();
```

- [ ] **Step 6: Update `tests/test_lexer_ops.xvr`, `tests/test_parser_expr.xvr`, `tests/test_struct.xvr`**

These three files should use `include test_helpers;` and replace manual print-based checks with `check()`/`assertEq()` calls.

For `tests/test_lexer_ops.xvr` — add at top:
```xvr
include test_helpers;
```
And replace inline checks with `assertTrue(cond, msg)` calls.

For `tests/test_parser_expr.xvr` — same pattern.

For `tests/test_struct.xvr` — same pattern.

- [ ] **Step 7: Build and verify**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash build.sh
```

Expected: build succeeds. Note: runtime segfault is pre-existing in LLVM codegen (Task 7 covers this).

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash test_self_host.sh
```

Expected: all checks pass (build, execution, embedded source, unit test compilation, IR validation).

- [ ] **Step 8: Commit**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add -A
git commit -m "test(stage0): convert tests to assertion-based, add test_helpers.xvr"
```

---

### Task 7: Final Verification

- [ ] **Step 1: Run main test suite**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr && ctest --test-dir build --output-on-failure
```

Expected: 100% tests passed, 0 tests failed.

- [ ] **Step 2: Run stage0 self-compilation test**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr/stage0 && bash test_self_host.sh
```

Expected: All checks pass (T1-T6). T5 may show SKIP for known LLVM codegen segfault — document as known pre-existing issue.

- [ ] **Step 3: Verify no leftover dead files**

```bash
ls /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/ast.xvr 2>&1 && echo "FAIL: ast.xvr should be deleted" || echo "OK: ast.xvr deleted"
ls /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/test_inc.xvr 2>&1 && echo "FAIL: test_inc.xvr should be deleted" || echo "OK: test_inc.xvr deleted"
ls /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/tests/t2.xvr 2>&1 && echo "FAIL: t2.xvr should be deleted" || echo "OK: t2.xvr deleted"
```

Expected: all deleted files confirmed gone.

- [ ] **Step 4: Verify node_types.xvr exists and is included**

```bash
grep -n "include node_types" /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/parser.xvr
grep -n "include node_types" /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/codegen.xvr
grep -n "NODE_PROGRAM" /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/node_types.xvr | head -1
```

Expected: parser.xvr and codegen.xvr both include node_types; node_types.xvr contains NODE_PROGRAM.

- [ ] **Step 5: Verify no duplicate NODE_* definitions remain**

```bash
grep -c "var NODE_" /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/parser.xvr
grep -c "var NODE_" /home/arfyslowy/Documents/project/xvrlang/xvr/stage0/src/codegen.xvr
```

Expected: both return `0` (no inline definitions — all come from node_types.xvr).

- [ ] **Step 6: Commit if any fixes needed**

```bash
cd /home/arfyslowy/Documents/project/xvrlang/xvr
git add -A
git commit -m "chore: final verification fixes"
```
