# C++ Implementation Migration Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert C implementation files (.c) to C++ (.cpp) for priority components with existing C++ headers, starting from low-level utilities upward, with testing after each task to ensure system stability.

**Architecture:** Convert each C implementation file to C++ by: 1) renaming .c to .cpp, 2) updating includes, 3) adding namespace wrappers where appropriate, 4) updating CMake, 5) testing. Maintain C API compatibility through extern "C" for the library interface.

**Tech Stack:** C++20, CMake, LLVM C++ API

---

## Phase 1: Low-Level Components (Memory, RefString, Common, Type)

### Task 1.1: Convert xvr_memory.c to C++

**Files:**
- Rename: `src/xvr_memory.c` → `src/xvr_memory.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/xvr_memory.c src/xvr_memory.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `xvr_memory.c` with `xvr_memory.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/xvr_memory.cpp src/CMakeLists.txt
git commit -m "refactor(memory): convert to C++"
```

---

### Task 1.2: Convert xvr_refstring.c to C++ (already done)

Note: This task was already completed in previous phase. Verify it works.

**Files:**
- Verify: `src/xvr_refstring.cpp` exists
- Test: Run existing tests

- [ ] **Step 1: Verify file exists and builds**

```bash
ls -la src/xvr_refstring.cpp && cmake --build build 2>&1 | tail -5
```

- [ ] **Step 2: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 3: Commit (if changes needed)**

```bash
git add -A && git commit -m "fix(refstring): ensure C++ build"
```

---

### Task 1.3: Convert xvr_common.c to C++

**Files:**
- Rename: `src/xvr_common.c` → `src/xvr_common.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/xvr_common.c src/xvr_common.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `xvr_common.c` with `xvr_common.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/xvr_common.cpp src/CMakeLists.txt
git commit -m "refactor(common): convert to C++"
```

---

### Task 1.4: Convert xvr_type.c (core/types) to C++

**Files:**
- Rename: `src/core/types/xvr_type.c` → `src/core/types/xvr_type.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/core/types/xvr_type.c src/core/types/xvr_type.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `core/types/xvr_type.c` with `core/types/xvr_type.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/core/types/xvr_type.cpp src/CMakeLists.txt
git commit -m "refactor(types): convert to C++"
```

---

## Phase 2: Mid-Level Components (Lexer, Parser, Scope)

### Task 2.1: Convert xvr_lexer.c to C++

**Files:**
- Rename: `src/xvr_lexer.c` → `src/xvr_lexer.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/xvr_lexer.c src/xvr_lexer.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `xvr_lexer.c` with `xvr_lexer.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/xvr_lexer.cpp src/CMakeLists.txt
git commit -m "refactor(lexer): convert to C++"
```

---

### Task 2.2: Convert xvr_parser.c to C++

**Files:**
- Rename: `src/xvr_parser.c` → `src/xvr_parser.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/xvr_parser.c src/xvr_parser.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `xvr_parser.c` with `xvr_parser.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/xvr_parser.cpp src/CMakeLists.txt
git commit -m "refactor(parser): convert to C++"
```

---

### Task 2.3: Convert xvr_scope.c to C++

**Files:**
- Rename: `src/xvr_scope.c` → `src/xvr_scope.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/xvr_scope.c src/xvr_scope.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `xvr_scope.c` with `xvr_scope.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/xvr_scope.cpp src/CMakeLists.txt
git commit -m "refactor(scope): convert to C++"
```

---

## Phase 3: Core Components (AST, Semantic, IR)

### Task 3.1: Convert xvr_ast_node.c (core/ast) to C++

**Files:**
- Rename: `src/core/ast/xvr_ast_node.c` → `src/core/ast/xvr_ast_node.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/core/ast/xvr_ast_node.c src/core/ast/xvr_ast_node.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `core/ast/xvr_ast_node.c` with `core/ast/xvr_ast_node.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/core/ast/xvr_ast_node.cpp src/CMakeLists.txt
git commit -m "refactor(ast): convert to C++"
```

---

### Task 3.2: Convert xvr_semantic.c (core/semantic) to C++

**Files:**
- Rename: `src/core/semantic/xvr_semantic.c` → `src/core/semantic/xvr_semantic.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/core/semantic/xvr_semantic.c src/core/semantic/xvr_semantic.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `core/semantic/xvr_semantic.c` with `core/semantic/xvr_semantic.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/core/semantic/xvr_semantic.cpp src/CMakeLists.txt
git commit -m "refactor(semantic): convert to C++"
```

---

### Task 3.3: Convert xvr_ir.c (core/ir) to C++

**Files:**
- Rename: `src/core/ir/xvr_ir.c` → `src/core/ir/xvr_ir.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/core/ir/xvr_ir.c src/core/ir/xvr_ir.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `core/ir/xvr_ir.c` with `core/ir/xvr_ir.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/core/ir/xvr_ir.cpp src/CMakeLists.txt
git commit -m "refactor(ir): convert to C++"
```

---

### Task 3.4: Convert xvr_ir_generator.c to C++

**Files:**
- Rename: `src/core/ir/xvr_ir_generator.c` → `src/core/ir/xvr_ir_generator.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/core/ir/xvr_ir_generator.c src/core/ir/xvr_ir_generator.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `core/ir/xvr_ir_generator.c` with `core/ir/xvr_ir_generator.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/core/ir/xvr_ir_generator.cpp src/CMakeLists.txt
git commit -m "refactor(ir_generator): convert to C++"
```

---

## Phase 4: LLVM Backend Components

### Task 4.1: Convert xvr_llvm_context.c to C++

**Files:**
- Rename: `src/adapters/llvm/xvr_llvm_context.c` → `src/adapters/llvm/xvr_llvm_context.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/adapters/llvm/xvr_llvm_context.c src/adapters/llvm/xvr_llvm_context.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `adapters/llvm/xvr_llvm_context.c` with `adapters/llvm/xvr_llvm_context.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/adapters/llvm/xvr_llvm_context.cpp src/CMakeLists.txt
git commit -m "refactor(llvm_context): convert to C++"
```

---

### Task 4.2: Convert xvr_llvm_codegen.c to C++

**Files:**
- Rename: `src/adapters/llvm/xvr_llvm_codegen.c` → `src/adapters/llvm/xvr_llvm_codegen.cpp`
- Modify: `src/CMakeLists.txt`
- Test: Run existing tests

- [ ] **Step 1: Rename file**

```bash
git mv src/adapters/llvm/xvr_llvm_codegen.c src/adapters/llvm/xvr_llvm_codegen.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace `adapters/llvm/xvr_llvm_codegen.c` with `adapters/llvm/xvr_llvm_codegen.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/adapters/llvm/xvr_llvm_codegen.cpp src/CMakeLists.txt
git commit -m "refactor(llvm_codegen): convert to C++"
```

---

### Task 4.3: Convert remaining LLVM adapter files

Convert the remaining LLVM adapter files in batches:

**Files to convert (batch):**
- `src/adapters/llvm/xvr_llvm_module_manager.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_control_flow.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_type_mapper.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_expression_emitter.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_ir_builder.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_optimizer.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_target.c` → `.cpp`
- `src/adapters/llvm/xvr_llvm_function_emitter.c` → `.cpp`
- `src/adapters/llvm/xvr_asm_config.c` → `.cpp`

- [ ] **Step 1: Rename all files**

```bash
cd src/adapters/llvm
for f in xvr_llvm_module_manager xvr_llvm_control_flow xvr_llvm_type_mapper xvr_llvm_expression_emitter xvr_llvm_ir_builder xvr_llvm_optimizer xvr_llvm_target xvr_llvm_function_emitter xvr_asm_config; do
    git mv ${f}.c ${f}.cpp
done
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace all `.c` files with `.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/adapters/llvm/*.cpp src/CMakeLists.txt
git commit -m "refactor(llvm_adapters): convert remaining LLVM files to C++"
```

---

## Phase 5: Remaining Components

### Task 5.1: Convert remaining utility files

**Files (batch conversion):**
- `src/xvr_literal.c` → `.cpp`
- `src/xvr_literal_array.c` → `.cpp`
- `src/xvr_literal_dictionary.c` → `.cpp`
- `src/xvr_string_utils.c` → `.cpp`
- `src/xvr_print_handler.c` → `.cpp`
- `src/xvr_format_string.c` → `.cpp`
- `src/xvr_cast_emit.c` → `.cpp`
- `src/xvr_keyword_types.c` → `.cpp`
- `src/xvr_builtin.c` → `.cpp`
- `src/xvr_unused.c` → `.cpp`

- [ ] **Step 1: Rename files**

```bash
cd src
for f in xvr_literal xvr_literal_array xvr_literal_dictionary xvr_string_utils xvr_print_handler xvr_format_string xvr_cast_emit xvr_keyword_types xvr_builtin xvr_unused; do
    git mv ${f}.c ${f}.cpp
done
```

- [ ] **Step 2: Update CMakeLists.txt**

Replace all `.c` files with `.cpp` in the XVR_SOURCES list

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/*.cpp src/CMakeLists.txt
git commit -m "refactor(utils): convert utility files to C++"
```

---

### Task 5.2: Convert optimizer and sema files

**Files:**
- `src/optimizer/xvr_ast_optimizer.c` → `.cpp`
- `src/sema/xvr_builtin.c` → `.cpp`
- `src/xvr_runtime.c` → `.cpp`

- [ ] **Step 1: Rename files**

```bash
git mv src/optimizer/xvr_ast_optimizer.c src/optimizer/xvr_ast_optimizer.cpp
git mv src/sema/xvr_builtin.c src/sema/xvr_builtin.cpp
git mv src/xvr_runtime.c src/xvr_runtime.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

- [ ] **Step 3: Test build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 4: Run tests**

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/optimizer/xvr_ast_optimizer.cpp src/sema/xvr_builtin.cpp src/xvr_runtime.cpp src/CMakeLists.txt
git commit -m "refactor(optimizer): convert optimizer to C++"
```

---

## Summary

This plan converts all priority C implementation files to C++:

1. **Phase 1** - Memory, RefString, Common, Type (4 files)
2. **Phase 2** - Lexer, Parser, Scope (3 files)
3. **Phase 3** - AST, Semantic, IR (4 files)
4. **Phase 4** - LLVM adapters (11 files)
5. **Phase 5** - Utilities, optimizer, runtime (13 files)

Total: ~35 files to convert

Each task includes build verification and test execution to ensure the system remains functional throughout the migration.