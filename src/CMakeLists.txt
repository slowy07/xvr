set(XVR_SOURCES
    xvr_ast_node.c
    xvr_cast_emit.c
    xvr_common.c
    xvr_format_string.c
    xvr_keyword_types.c
    xvr_lexer.c
    xvr_literal.c
    xvr_literal_array.c
    xvr_literal_dictionary.c
    xvr_memory.c
    xvr_parser.c
    xvr_print_handler.c
    xvr_refstring.c
    xvr_runtime.c
    xvr_scope.c
    xvr_semantic.c
    xvr_type.c
    xvr_unused.c
    backend/xvr_llvm_codegen.c
    backend/xvr_llvm_context.c
    backend/xvr_llvm_control_flow.c
    backend/xvr_llvm_expression_emitter.c
    backend/xvr_llvm_function_emitter.c
    backend/xvr_llvm_ir_builder.c
    backend/xvr_llvm_module_manager.c
    backend/xvr_llvm_optimizer.c
    backend/xvr_llvm_target.c
    backend/xvr_llvm_type_mapper.c
)

set(XVR_HEADERS
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
    xvr_scope.h
    xvr_semantic.h
    xvr_token_types.h
    xvr_type.h
    xvr_unused.h
    backend/xvr_llvm_backend.h
    backend/xvr_llvm_codegen.h
    backend/xvr_llvm_context.h
    backend/xvr_llvm_control_flow.h
    backend/xvr_llvm_expression_emitter.h
    backend/xvr_llvm_function_emitter.h
    backend/xvr_llvm_ir_builder.h
    backend/xvr_llvm_module_manager.h
    backend/xvr_llvm_optimizer.h
    backend/xvr_llvm_target.h
    backend/xvr_llvm_type_mapper.h
)

add_library(xvr_objects OBJECT ${XVR_SOURCES} ${XVR_HEADERS})

target_include_directories(xvr_objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(xvr_objects PRIVATE ${XVR_COMPILE_FLAGS})

if(XVR_SANITIZER)
    target_compile_options(xvr_objects PRIVATE -fno-omit-frame-pointer)
endif()

set(LLVM_LIBS -lLLVM-22)

add_library(xvr_llvm_libs INTERFACE)
target_link_libraries(xvr_llvm_libs INTERFACE ${LLVM_LIBS})

if(XVR_BUILD_STATIC)
    add_library(xvr_static STATIC $<TARGET_OBJECTS:xvr_objects>)
    target_compile_options(xvr_static PRIVATE ${XVR_COMPILE_FLAGS})
    target_link_libraries(xvr_static PRIVATE xvr_llvm_libs)
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
    target_link_libraries(xvr_shared PRIVATE xvr_llvm_libs)
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
