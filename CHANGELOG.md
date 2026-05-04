# Changelog

All notable changes to the XVR Programming Language project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed
- **Parser**: Fixed parseRules array ordering bug where `IMPORT`, `INCLUDE`, `IN`, `OF`, `PRINT` tokens were misaligned with `Xvr_TokenType` enum, causing function calls to fail with "unexpected token" errors
- **LLVM Codegen**: Fixed void function default - functions without explicit return type now default to `void` (instead of `int`), preventing invalid IR generation
- **LLVM Codegen**: Added terminator instructions (`ret void`/`ret 0`) when LLVM IR has errors, preventing crashes in optimization passes (BranchProbabilityInfo)
- **Module Resolver**: Added `Xvr_ModuleResolverAddSearchPath()` function for dynamic search path registration
- **Tests**: Fixed `get_xvr_path()` in test infrastructure to check for regular files (not directories), resolving test failures (94/94 now passing)
- **Compiler**: Fixed `Xvr_LLVMCodegenSetIncludeDir()` to use correct API signature

### Added
- **API**: `Xvr_ModuleResolverAddSearchPath()` - allows adding search paths to module resolver at runtime
- **API**: `Xvr_LLVMCodegenSetIncludeDir()` - sets include directory for source-relative module resolution

### Changed
- **Stage0**: Verified stage0 bootstrap compiler builds successfully with current lexer implementation

## [0.6.16] - 2026-05-02

### Added
- **std::print**: Support for `{}` format placeholders with auto-detected argument types
- **Module Resolution**: Multi-path module discovery (`XVR_MAX_SEARCH_PATHS=16`)
- **User main**: Renamed user-defined `main` to `_xvr_main` to avoid LLVM entry point collision

### Fixed
- **LLVMBuildCall2**: Fixed crash by using `LLVMGlobalGetValueType` for correct function type
- **Null args**: Fixed crash on zero-argument function calls
- **std:: dispatch**: Fixed routing bug to correctly call `emit_printf`, `emit_max`, etc.
- **Unary operators**: Added support for `-`, `!` in function call arguments
- **Float promotion**: Fixed float-to-double promotion for variadic function calls (printf `%f`)
- **Module resolver**: Removed hardcoded developer paths from `main_compiler.c`
- **Double free**: Fixed double free on inline source in `main_compiler.c`
- **Stage0 lexer**: Fixed `tok.type = INVALID` → `tok.type = tokType`
- **Stage0 generate**: Fixed missing return statement
- **Bounds checking**: Added bounds checking to `search_paths` array

### Changed
- **Function keyword**: Single function declaration keyword `proc` (removed `fn`)
- **Parser**: `TOKEN_PRINT` now uses `identifierOrKeyword` prefix rule with `fnCall` infix rule

## [0.6.0] - 2024-04-25

### Added
- Initial LLVM AOT compilation support
- Standard library (`std::`) with print, max, min, sizeof, len functions
- Module system with `include` keyword
- Optimization pipeline with constant folding and dead code elimination
- Test suite with Catch2 integration
- Stage0 bootstrap compiler

[Unreleased]: https://github.com/anomalyco/xvr/compare/v0.6.16...HEAD
[0.6.16]: https://github.com/anomalyco/xvr/releases/tag/v0.6.16
[0.6.0]: https://github.com/anomalyco/xvr/releases/tag/v0.6.0
