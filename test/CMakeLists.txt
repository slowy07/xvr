set(TEST_SOURCES
    test_all.c
    test_ast_node.c
    test_lexer.c
    test_parser.c
    test_compiler.c
    test_literal.c
    test_memory.c
    test_scope.c
    test_llvm_backend.c
    test_std.c
)

set(TEST_HEADERS
    xvr_test.h
)

add_executable(xvr_test_all ${TEST_SOURCES} ${TEST_HEADERS})

target_include_directories(xvr_test_all PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/backend
)

target_compile_options(xvr_test_all PRIVATE
    ${XVR_COMPILE_FLAGS}
    -DXVR_EXPORT_LLVM
)

if(XVR_SANITIZER)
    target_link_libraries(xvr_test_all PRIVATE xvr_objects -fsanitize=${XVR_SANITIZER} xvr_llvm_libs)
elseif(XVR_BUILD_STATIC)
    target_link_libraries(xvr_test_all PRIVATE xvr_static xvr_llvm_libs)
else()
    target_link_libraries(xvr_test_all PRIVATE xvr_shared xvr_llvm_libs)
endif()

set_target_properties(xvr_test_all PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${XVR_OUTPUT_DIR}
)

add_test(NAME xvr_test_all COMMAND xvr_test_all)

install(TARGETS xvr_test_all RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
