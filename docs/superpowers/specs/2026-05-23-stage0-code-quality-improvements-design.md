# Stage0 Code Quality Improvements — Design

## Goal

Improve the stage0 bootstrap compiler codebase following software engineering principles: DRY, SRP, KISS, and testability. The result should be easier to maintain, extend, and eventually self-host.

## Scope

Four independent sections, deliverable in any order:

1. Code Organization Cleanup
2. Helper Extraction & Monolith Breaking
3. Data Flow Fix
4. Test Improvements

No new features. No behavioral changes. The compiler still accepts the same input and produces the same output.

---

## Section 1: Code Organization Cleanup

### Problem

- `ast.xvr` defines node types as an enum — but is never included by any file (dead code)
- `parser.xvr` defines 19 `NODE_*` constants independently (lines 3-21)
- `codegen.xvr` defines the same 19 constants independently (lines 1-19) — triple duplication
- 4 abandoned test stubs in `tests/`: `test_lexer.xvr`, `test_parser.xvr`, `test_codegen.xvr`, `t2.xvr`
- 5 experimental files in `src/`: `test_inc*.xvr`, `test_minimal.xvr`, `test_lexer_combined.xvr`
- Unused functions: `lex()` in lexer.xvr (218-237), `generateCall()` in codegen.xvr (172-176), unused `tokens` param in `parse()` (422)

### Design

1. Delete `ast.xvr` (dead code)
2. Create `src/node_types.xvr` with all 19 `NODE_*` constants — single source of truth
3. `parser.xvr` includes `node_types.xvr` instead of defining constants inline
4. `codegen.xvr` includes `node_types.xvr` instead of defining constants inline
5. Delete abandoned test stubs: `tests/test_lexer.xvr`, `tests/test_parser.xvr`, `tests/test_codegen.xvr`, `tests/t2.xvr`
6. Delete experimental test files from `src/`: `test_inc.xvr`, `test_inc2.xvr`, `test_inc3.xvr`, `test_inc4.xvr`, `test_minimal.xvr`, `test_lexer_combined.xvr`
7. Delete unused `lex()` function from lexer.xvr (now dead since Section 3 introduces its own approach)
8. Delete unused `generateCall()` from codegen.xvr
9. Remove unused `tokens` parameter from `parse()`

### Impact

- Removes ~10 files and ~80 lines of dead code
- Eliminates the critical triple-maintenance problem for node constants
- Each `.xvr` source file is now only what's needed for the build

---

## Section 2: Helper Extraction & Monolith Breaking

### Problem

- `nextToken()` (181 lines) is a monolith handling 5+ token types in one function
- `parseInfix()` (127 lines) has 13 near-identical operator blocks — 108 lines of copy-paste
- Character class checks (`c >= "a" && c <= "z"`) repeated 6+ times
- Keyword detection via 11 sequential `if` statements
- Operator precedence values 2-7 are magic numbers with no named constants

### Design

**Character helpers** in lexer.xvr:
```
proc isAlpha(c: string): bool {
    return (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || c == "_";
}
proc isDigit(c: string): bool { return c >= "0" && c <= "9"; }
proc isAlphaNum(c: string): bool { return isAlpha(c) || isDigit(c); }
```

**nextToken() refactor**: Replace the 181-line function with a dispatcher:
- `nextToken()` (driver) — skips whitespace, dispatches to specialized lexers
- `lexNumber()` — integer and float literals (lines 56-98)
- `lexIdentifierOrKeyword()` — identifiers and keywords, with a lookup table (lines 99-127)
- `lexString()` — string literals with unterminated detection (lines 131-158)
- `lexOperatorOrPunctuation()` — single and multi-char operators (lines 159-215)

**parseInfix() refactor**: Replace the 13 operator if-blocks with a data table:
```
var OP_TABLE: [i32];  // flat array: tokType, leftPrec, rightPrec
```
Binary operators dispatch by indexing into the table instead of 13 if-statements.

**Precedence named constants**:
```
var PREC_OR: i32 = 2;
var PREC_AND: i32 = 3;
var PREC_EQ: i32 = 4;
var PREC_COMPARE: i32 = 5;
var PREC_TERM: i32 = 6;
var PREC_FACTOR: i32 = 7;
var PREC_UNARY: i32 = 8;
var PREC_CALL: i32 = 9;
```

### Impact

- `nextToken()` shrinks from 181 to ~40 lines (driver)
- `parseInfix()` shrinks from 127 to ~30 lines (table-driven)
- Character helpers eliminate 6+ inline range checks
- Keyword table eliminates 11 fragile if-statements
- Precedence names eliminate magic numbers

---

## Section 3: Data Flow Fix

### Problem

`main.xvr` currently:
1. Lexes 5 tokens from `"var x = 42"` and prints "Got token" each time — ignores the token values
2. Calls `initParser()` — but parser reads from lexer's global token arrays (which ARE populated correctly)
3. Calls `generate()` — which produces hardcoded IR (`"add i32 0, 0"`) completely ignoring parser state
4. The lexer output is never consumed; the parser output is never consumed

### Design

Replace the stub `main.xvr` with a real compiler pipeline:

```
source = "var x = 42";
initLexer(source);
// Lex all tokens
while (nextToken().type != EOF) { }

initParser();
var nodeCount = parse();  // returns number of AST nodes (or -1 on failure)

if (nodeCount > 0) {
    initCodegen();
    var i: i32 = 0;
    while (i < nodeCount) {
        generateNode(g_astKinds[i], g_astLefts[i], g_astRights[i],
                     g_astExtras[i], g_astValues[i]);
        i = i + 1;
    }
    var ir = getIR();
    std::print(ir);
} else {
    std::print("Parse failed\n");
}
```

This requires `parse()` to populate lexer's token arrays (which it already does). The codegen iterates the flat AST arrays.

**Prerequisite**: `parser.xvr`'s `parse()` function already populates `g_astKinds[]`, `g_astLefts[]`, etc. in the parser's globals (parser.xvr lines 23-30). These globals need to be accessible to `main.xvr` — currently they're local to the parser. The fix: make `parse()` return `g_nodeCount` and expose the AST globals.

Alternatively, `parser.xvr` already exposes its AST arrays as `g_` globals visible from the caller.

### Impact

- `main.xvr` goes from ~40 lines of stub code to ~25 lines of actual pipeline
- Stage0 finally compiles source → AST → IR (even if the IR is still incomplete)
- The "Stage0 ready - lexer works, parser/codegen need implementation" message is replaced with real IR output

---

## Section 4: Test Improvements

### Problem

Only 1 of 12 test files has actual assertion logic (`test_lexer_ops.xvr`). The rest print values to stdout that require manual verification. No negative tests exist.

### Design

Convert each test file from print-and-check to assertion-based:

```
proc assert_eq(got: i32, expected: i32, msg: string) {
    if (got != expected) {
        std::print("FAIL: " + msg + " (got " + got + ", expected " + expected + ")\n");
    } else {
        std::print("PASS: " + msg + "\n");
    }
}
```

Add a shared `test_helpers.xvr` with assertion utilities. Each test file includes it.

**Specific test conversions**:

| Test File | Current | Target |
|-----------|---------|--------|
| `test_lexer_stage0.xvr` | Prints token types manually | Assert token count = 5 for "var x = 42", assert type values |
| `test_parser_stage0.xvr` | Prints node count | Assert node count > 0 for valid input, assert parsed 1 var decl |
| `test_codegen_stage0.xvr` | Prints IR length | Assert IR contains "define", "i32", "@main" |
| `test_stage0_all.xvr` | Prints "OK" after each section | Assert-based with pass/fail counters |
| `test_parser_expr.xvr` | Mixed (already has some asserts) | Full assertion coverage |

**New edge case tests**:
- `test_lexer_empty.xvr` — empty source produces just EOF
- `test_lexer_unterminated.xvr` — unterminated string produces INVALID token
- `test_lexer_max_tokens.xvr` — near-10K token input doesn't crash (optional, low priority)

### Impact

- Tests shift from manual-inspection to automated
- Regression detection becomes possible
- Error handling paths get tested

---

## Files Changed

### Deleted
- `src/ast.xvr` — replaced by `node_types.xvr`
- `src/test_inc.xvr`, `src/test_inc2.xvr`, `src/test_inc3.xvr`, `src/test_inc4.xvr`, `src/test_minimal.xvr`, `src/test_lexer_combined.xvr` — experimental clutter
- `tests/test_lexer.xvr`, `tests/test_parser.xvr`, `tests/test_codegen.xvr`, `tests/t2.xvr` — abandoned stubs

### Created
- `src/node_types.xvr` — single-source node type constants
- `tests/test_helpers.xvr` — assertion utilities

### Modified
- `src/parser.xvr` — include node_types, refactor parseInfix, add PREC_* constants, remove unused tokens param
- `src/codegen.xvr` — include node_types, remove unused generateCall()
- `src/lexer.xvr` — extract helpers, refactor nextToken, remove unused lex()
- `src/main.xvr` — wire real pipeline
- `tests/test_lexer_stage0.xvr` — assertion-based
- `tests/test_parser_stage0.xvr` — assertion-based
- `tests/test_codegen_stage0.xvr` — assertion-based
- `tests/test_stage0_all.xvr` — assertion-based
- `tests/test_lexer_ops.xvr` — use shared helpers
- `tests/test_parser_expr.xvr` — use shared helpers
- `tests/test_struct.xvr` — use shared helpers
- `run_tests.sh` — no changes needed (already references correct test files)

---

## Non-Goals

- No new features (no comments, no escape sequences, no file I/O)
- No changes to token.xvr (token types stay as-is)
- No changes to the LLVM backend codegen (placeholders remain)
- No fixing the pre-existing LLVM codegen segfault (out of scope)
- No build system changes (build.sh stays the same)
