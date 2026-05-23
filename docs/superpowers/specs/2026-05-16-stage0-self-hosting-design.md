# Stage0 Bootstrap Compiler: Self-Hosting Improvements

**Date**: 2026-05-16
**Status**: Design (approved)
**Goal**: Make stage0 bootstrap compiler capable of compiling its own source code

## Overview

Stage0 is a bootstrap compiler written in XVR. Currently it can tokenize and
parse basic statements, but lacks expression parsing, multi-character operators,
and runtime I/O. This design adds these capabilities in 5 incremental layers to
achieve self-hosting — where stage0 can compile its own source files.

## Approach

**Expression-First (Approach A1 from brainstorming)**

Focus on expression parsing first (the biggest blocker), then lexer operators,
CLI, codegen, and finally self-compilation testing. Keep parallel arrays for
AST storage (defer `ast.xvr` refactoring).

---

## Layer 1: Pratt Expression Parser

### Precedence Levels

| Level | Name | Operators |
|-------|------|-----------|
| 0 | DEFAULT | (top-level expression start) |
| 1 | ASSIGN | `=` |
| 2 | LOGICAL | `\|\|` |
| 3 | AND | `&&` |
| 4 | EQUALITY | `==`, `!=` |
| 5 | COMPARE | `<`, `>`, `<=`, `>=` |
| 6 | TERM | `+`, `-` |
| 7 | FACTOR | `*`, `/`, `%` |
| 8 | UNARY | `!`, `-` (prefix) |
| 9 | CALL | `()`, `[]`, `.` |

### Implementation (in `parser.xvr`)

| Procedure | Role |
|-----------|------|
| `parseExpression(prec: i32): i32` | Main entry — parse prefix, then infix while next prec >= prec |
| `parsePrefix(): i32` | Handle IDENT, INT_LITERAL, STRING_LITERAL, `(expr)`, `-expr`, `!expr` |
| `parseInfix(left: i32, minPrec: i32): i32` | Handle `+`, `-`, `*`, `/`, `fn(args)`, `arr[i]`, `obj.field` |

### Precedence Lookup

| Token | Precedence | Associativity |
|-------|------------|---------------|
| `OP_PLUS`, `OP_MINUS` | 6 | Left |
| `OP_STAR`, `OP_SLASH`, `OP_PERCENT` | 7 | Left |
| `OP_EQ`, `OP_NEQ` | 4 | Left |
| `OP_LT`, `OP_GT`, `OP_LTE`, `OP_GTE` | 5 | Left |
| `OP_AND` | 3 | Left |
| `OP_OR` | 2 | Left |
| `OP_ASSIGN` | 1 | Right |

### New AST Node Kinds

| Node Kind | Extra field | Meaning |
|-----------|-------------|---------|
| `NODE_BINARY_OP` | operator token ID | e.g., `OP_PLUS` for `a + b` |
| `NODE_UNARY_OP` | operator token ID | e.g., `OP_NOT` for `!x` |
| `NODE_CALL` | arg count | function call `fn(a, b)` |
| `NODE_INDEX` | - | array access `arr[i]` |
| `NODE_FIELD` | - | struct field `obj.field` |
| `NODE_CONDITIONAL` | else branch index | ternary `c ? t : f` (if-expression) |

### Integration

Current `parseExpression()` → replaced by `parseExpression(0)`.
Current `parseStatement()` unchanged — continues using keyword dispatch,
calls `parseExpression(0)` for expression statements.

---

## Layer 2: Multi-Character Operators

### Peek-based lexer extension (in `lexer.xvr`)

Add `peek()`:
```
proc peek(): string {
    if (g_pos + 1 < len(g_source)) {
        return g_source.subString(g_pos + 1, 1);
    }
    return "";
}
```

### Operator Character Dispatch

| Current char | peek() | Result token |
|-------------|--------|--------------|
| `=` | `=` | `OP_EQ` |
| `!` | `=` | `OP_NEQ` |
| `<` | `=` | `OP_LTE` |
| `>` | `=` | `OP_GTE` |
| `&` | `&` | `OP_AND` |
| `\|` | `\|` | `OP_OR` |
| `-` | `>` | `ARROW` |
| `.` | `.` | `RANGE` |

### New Token Constants (in `token.xvr`)

```
var ARROW: i32 = 57;   // ->
var RANGE: i32 = 58;   // ..
```

---

## Layer 3: Pipe/CLI Interface (Two-Stage)

### Stage 3a: Build-Script Source Embedding

The main XVR compiler does not support preprocessor `-D` flags. Instead,
a build script generates a temporary `main_*.xvr` with the source embedded
directly as a string constant, then compiles it:

```bash
#!/bin/bash
# stage0/embed_and_build.sh
SOURCE_FILE="$1"
EMBEDDED=$(cat "$SOURCE_FILE" | sed 's/"/\\"/g')  # escape quotes
cat > /tmp/_stage0_main.xvr << ENDXVR
include lexer;
include parser;
include codegen;

const SOURCE = "$EMBEDDED";

proc run(): void {
    initLexer(SOURCE);
    initParser();
    var ast = parse([]);
    initCodegen();
    generate();
    std::print(getIR());
}
run();
ENDXVR
../build/xvr /tmp/_stage0_main.xvr -o /tmp/stage0_bin
```

This avoids needing runtime file I/O for the first self-hosting milestone.
The stage0 binary has the source code baked in at compile time, processes it,
and outputs LLVM IR to stdout.

### Stage 3b: Runtime `readFile` Builtin (Future)

Add external function declaration to enable runtime file reading:

```xvr
# In std/io.xvr:
proc readFile(path: string): string
    return ""
```

The main compiler's LLVM codegen maps it to `fopen` + `fread` + `fclose`.
This enables: `./stage0 lexer.xvr > lexer.ll`

---

## Layer 4: Codegen Extensions

### Expression IR Generation

In `codegen.xvr`, add `emitExpr(node: i32): string` that returns LLVM IR for
a subexpression as a string (including SSA register assignment):

| Node Kind + Extra | LLVM IR Output |
|-------------------|----------------|
| `NODE_LITERAL` | `i32 <value>` |
| `NODE_IDENTIFIER` | `i32 %<value>` (from `g_vars`) |
| `NODE_BINARY_OP` + `OP_PLUS` | `%t1 = add i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_MINUS` | `%t1 = sub i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_STAR` | `%t1 = mul i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_SLASH` | `%t1 = sdiv i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_EQ` | `%t1 = icmp eq i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_NEQ` | `%t1 = icmp ne i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_LT` | `%t1 = icmp slt i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_LTE` | `%t1 = icmp sle i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_GT` | `%t1 = icmp sgt i32 %left, %right` |
| `NODE_BINARY_OP` + `OP_GTE` | `%t1 = icmp sge i32 %left, %right` |
| `NODE_CALL` | `%t1 = call i32 @fn(type %arg0, type %arg1, ...)` |
| `NODE_CONDITIONAL` | `br i1 %cond, label %t, label %f` + phi |

### Register Allocation

Simple counter-based SSA:
```
var g_regCounter: i32 = 0;
proc nextReg(): string {
    var r = "%t" + g_regCounter;
    g_regCounter = g_regCounter + 1;
    return r;
}
```

---

## Layer 5: Self-Compilation Testing

### Incremental Test Plan

| Test | Source | What it proves |
|------|--------|----------------|
| T1 | `stage0/src/token.xvr` (50 lines, `var` decls only) | Basic variable decl parsing + codegen |
| T2 | `stage0/src/lexer.xvr` (231 lines, functions, loops, if/else, strings) | Full expression parsing + codegen |
| T3 | `stage0/src/codegen.xvr` (221 lines, arrays, struct ops) | Array/struct codegen |
| T4 | Full stage0 (`main.xvr` + 5 includes) | End-to-end self-compilation |

### Test Script (`stage0/tests/test_self_host.sh`)

Uses the build-script embedding approach for each test:

```bash
#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STAGE0_SRC="$SCRIPT_DIR/../src"
XVR="$SCRIPT_DIR/../../build/xvr"
PASS_T1=0; PASS_T2=0; PASS_T3=0

# T1: Embed token.xvr, compile, run, verify LLVM IR
bash $SCRIPT_DIR/../embed_and_build.sh "$STAGE0_SRC/token.xvr"
/tmp/stage0_bin > /tmp/token_out.ll 2>&1
if llc /tmp/token_out.ll -o /dev/null 2>/dev/null; then
    echo "PASS: token.xvr → valid LLVM IR"
    PASS_T1=1
fi

# T2: Embed lexer.xvr
bash $SCRIPT_DIR/../embed_and_build.sh "$STAGE0_SRC/lexer.xvr"
/tmp/stage0_bin > /tmp/lexer_out.ll 2>&1
if llc /tmp/lexer_out.ll -o /dev/null 2>/dev/null; then
    echo "PASS: lexer.xvr → valid LLVM IR"
    PASS_T2=1
fi

# T3: Embed codegen.xvr
bash $SCRIPT_DIR/../embed_and_build.sh "$STAGE0_SRC/codegen.xvr"
/tmp/stage0_bin > /tmp/codegen_out.ll 2>&1
if llc /tmp/codegen_out.ll -o /dev/null 2>/dev/null; then
    echo "PASS: codegen.xvr → valid LLVM IR"
    PASS_T3=1
fi

echo "T1=$PASS_T1 T2=$PASS_T2 T3=$PASS_T3"
```

---

## Files Modified

| File | Layer | Changes |
|------|-------|---------|
| `stage0/src/parser.xvr` | L1 | Replace `parseExpression()` with Pratt parser, add `parsePrefix()`/`parseInfix()`, add NODE_* kinds |
| `stage0/src/lexer.xvr` | L2 | Add `peek()`, multi-char operator handling for `==`/`!=`/`<=`/`>=`/`&&`/`\|\|`/`->`/`..` |
| `stage0/src/token.xvr` | L2 | Add `ARROW=57`, `RANGE=58` |
| `stage0/src/main.xvr` | L3 | Replace hardcoded source with `const SOURCE`, add CLI argument handling |
| `stage0/src/codegen.xvr` | L4 | Add `emitExpr()`, `nextReg()`, expression IR generation for all operator types |
| `stage0/tests/test_self_host.sh` | L5 | Self-compilation test script |

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Pratt parser too complex for stage0's limited language | Start with only needed precedence levels (5-7). Use simple `while` loop, not recursion |
| Parallel arrays hit limits for expression ASTs | Each expression constructs a sub-tree. Add `NODE_TEMP` for intermediate results if needed |
| LLVM IR from stage0 won't pass `llc` verification | Test IR after each layer with `llc -o /dev/null` |
| Compile-time embedding creates slow iteration cycle | Test parsing separately first (dump AST), only compile to IR when parser is validated |
