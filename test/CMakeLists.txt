include(FetchContent)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(catch2)

set(TEST_SOURCES
    xvr_test_main.cpp
    fixtures/xvr_test_fixture.cpp
    test_literal.cpp
    test_scope.c
    test_lexer.c
    test_parser.c
    test_memory.c
    test_compiler.c
    test_llvm_backend.c
    test_std.c
    test_ast_node.c
)

add_executable(xvr_test_all ${TEST_SOURCES})

target_include_directories(xvr_test_all PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/backend
    ${CMAKE_SOURCE_DIR}/src/adapters/llvm
)

target_link_libraries(xvr_test_all PRIVATE
    Catch2::Catch2
    xvr_objects
)

if(XVR_SANITIZER)
    target_link_libraries(xvr_test_all PRIVATE -fsanitize=${XVR_SANITIZER})
endif()

if(XVR_BUILD_STATIC)
    target_link_libraries(xvr_test_all PRIVATE xvr_static)
else()
    target_link_libraries(xvr_test_all PRIVATE xvr_shared)
endif()

target_link_libraries(xvr_test_all PRIVATE xvr_llvm_libs m)

set_target_properties(xvr_test_all PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${XVR_OUTPUT_DIR}
)

add_test(NAME xvr_test_all COMMAND xvr_test_all)

install(TARGETS xvr_test_all RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
