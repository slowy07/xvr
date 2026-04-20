set(XVR_SOURCES
    core/types/xvr_type.cpp
    xvr_cast_emit.cpp
    xvr_common.cpp
    xvr_format_string.cpp
    xvr_keyword_types.cpp
    xvr_lexer.cpp
    xvr_literal.cpp
    xvr_literal_array.cpp
    xvr_literal_dictionary.cpp
    xvr_memory.cpp
    xvr_parser.cpp
    xvr_print_handler.cpp
    xvr_refstring.cpp
    xvr_runtime.cpp
    xvr_scope.cpp
    core/semantic/xvr_semantic.cpp
    xvr_string_utils.cpp
    xvr_unused.cpp
    sema/xvr_builtin.cpp
    optimizer/xvr_ast_optimizer.cpp
    adapters/llvm/xvr_asm_config.cpp
    adapters/llvm/xvr_llvm_codegen.cpp
    adapters/llvm/xvr_llvm_context.cpp
    adapters/llvm/xvr_llvm_control_flow.cpp
    adapters/llvm/xvr_llvm_expression_emitter.cpp
    adapters/llvm/xvr_llvm_function_emitter.cpp
    adapters/llvm/xvr_llvm_ir_builder.cpp
    adapters/llvm/xvr_llvm_module_manager.cpp
    adapters/llvm/xvr_llvm_optimizer.cpp
    adapters/llvm/xvr_llvm_target.cpp
    adapters/llvm/xvr_llvm_type_mapper.cpp
    core/ir/xvr_ir.cpp
    core/ir/xvr_ir_generator.cpp
    core/ast/xvr_ast_node.cpp
)

set(XVR_HEADERS
    core/types/xvr_type.h
    xvr_ast_node.h
    xvr_cast_emit.h
    xvr_common.h
    xvr_console_colors.h
    xvr_debug.h
    xvr_format_string.h
    xvr_keyword_types.h
    xvr_lexer.h
    xvr_literal.h
    xvr_literal_array.h
    xvr_literal_dictionary.h
    xvr_memory.h
    xvr_opcodes.h
    xvr_parser.h
    xvr_print_handler.h
    xvr_refstring.h
    xvr_refstring.hpp
    xvr_scope.h
    xvr_semantic.h
    xvr_string_utils.h
    xvr_token_types.h
    core/types/xvr_type.h
    xvr_unused.h
    sema/xvr_builtin.h
    optimizer/xvr_ast_optimizer.h
    adapters/llvm/xvr_asm_config.h
    adapters/llvm/xvr_llvm_backend.h
    adapters/llvm/xvr_llvm_codegen.h
    adapters/llvm/xvr_llvm_context.h
    adapters/llvm/xvr_llvm_control_flow.h
    adapters/llvm/xvr_llvm_expression_emitter.h
    adapters/llvm/xvr_llvm_function_emitter.h
    adapters/llvm/xvr_llvm_ir_builder.h
    adapters/llvm/xvr_llvm_module_manager.h
    adapters/llvm/xvr_llvm_optimizer.h
    adapters/llvm/xvr_llvm_target.h
    adapters/llvm/xvr_llvm_type_mapper.h
    core/ir/xvr_ir.h
    core/ir/xvr_ir_generator.h
    core/ast/xvr_ast_node.h
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(xvr_objects OBJECT ${XVR_SOURCES} ${XVR_HEADERS})

target_include_directories(xvr_objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(xvr_objects PRIVATE ${XVR_COMPILE_FLAGS})

if(XVR_SANITIZER)
    target_compile_options(xvr_objects PRIVATE -fno-omit-frame-pointer)
endif()

add_library(xvr_llvm_libs INTERFACE)

if(APPLE)
    target_link_libraries(xvr_llvm_libs INTERFACE -lLLVM)
elseif(UNIX)
    target_link_libraries(xvr_llvm_libs INTERFACE -lLLVM)
endif()

if(XVR_BUILD_STATIC)
    add_library(xvr_static STATIC $<TARGET_OBJECTS:xvr_objects>)
    target_compile_options(xvr_static PRIVATE ${XVR_COMPILE_FLAGS})
    target_link_libraries(xvr_static PRIVATE xvr_llvm_libs ${XVR_LINK_FLAGS})
    set_target_properties(xvr_static PROPERTIES
        OUTPUT_NAME xvr
        PREFIX lib
        POSITION_INDEPENDENT_CODE ON
    )
    add_dependencies(xvr_static xvr_objects)
endif()

if(XVR_BUILD_SHARED)
    add_library(xvr_shared SHARED $<TARGET_OBJECTS:xvr_objects>)
    target_compile_options(xvr_shared PRIVATE ${XVR_COMPILE_FLAGS} -fPIC -DXVR_IMPORT)
    target_link_libraries(xvr_shared PRIVATE xvr_llvm_libs ${XVR_LINK_FLAGS})
    set_target_properties(xvr_shared PROPERTIES
        OUTPUT_NAME xvr
        PREFIX lib
        VERSION 1.0.0
        SOVERSION 1
    )
    add_dependencies(xvr_shared xvr_objects)
endif()

set_target_properties(xvr_objects PROPERTIES
    CLEAN_DIRECT_OUTPUT 1
    POSITION_INDEPENDENT_CODE ON
)
