cmake_minimum_required(VERSION 3.16)
project(XVR VERSION 0.5.13 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS ON)

include(GNUInstallDirs)
include(CMakeDependentOption)
include(CheckCCompilerFlag)

set(XVR_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(XVR_SRC_DIR ${XVR_ROOT_DIR}/src)
set(XVR_BACKEND_DIR ${XVR_SRC_DIR}/backend)
set(XVR_COMPILER_DIR ${XVR_ROOT_DIR}/compiler)
set(XVR_TEST_DIR ${XVR_ROOT_DIR}/test)
set(XVR_DOCS_DIR ${XVR_ROOT_DIR}/docs)

set(XVR_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(XVR_OBJ_DIR ${CMAKE_BINARY_DIR}/obj)

option(XVR_BUILD_TESTS "Build test suite" OFF)
option(XVR_BUILD_SHARED "Build shared library" OFF)
option(XVR_BUILD_STATIC "Build static library" ON)
option(XVR_ENABLE_INSTALL "Enable installation" ON)

set(XVR_BUILD_TYPE "debug" CACHE STRING "Build type: debug, release, minsizerel, relwithdebinfo")
set_property(CACHE XVR_BUILD_TYPE PROPERTY STRINGS "debug" "release" "minsizerel" "relwithdebinfo")

set(XVR_DEBUG "OFF" CACHE BOOL "Enable debug mode")
set(XVR_SANITIZE "" CACHE STRING "Sanitizer: address, undefined, thread, memory, all")
set(XVR_PROFILE "OFF" CACHE BOOL "Enable profiling support")

if(WIN32)
    set(DEFAULT_COMPILER "clang" CACHE STRING "C compiler")
    set(DEFAULT_CXX_COMPILER "clang++" CACHE STRING "C++ compiler")
else()
    set(DEFAULT_COMPILER "cc" CACHE STRING "C compiler")
    set(DEFAULT_CXX_COMPILER "c++" CACHE STRING "C++ compiler")
endif()

set(CMAKE_C_COMPILER ${DEFAULT_COMPILER} CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${DEFAULT_CXX_COMPILER} CACHE FILEPATH "C++ compiler")


message(STATUS "  XVR Compiler Build System")

message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER_ID} (${CMAKE_C_COMPILER})")
message(STATUS "  Build Type: ${XVR_BUILD_TYPE}")
message(STATUS "  Debug: ${XVR_DEBUG}")
message(STATUS "  Sanitizer: ${XVR_SANITIZE}")
message(STATUS "  Profile: ${XVR_PROFILE}")


if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(XVR_IMPORT)
    
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_compile_definitions(_WIN64)
    endif()
    
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(COMPILER_FLAGS_BASE "-Wall -Wextra -Wpedantic -Wno-unused-parameter")
        set(COMPILER_FLAGS_DEBUG "-g -O0")
        set(COMPILER_FLAGS_RELEASE "-O2")
    else()
        set(COMPILER_FLAGS_BASE "/W4")
        set(COMPILER_FLAGS_DEBUG "/WX-")
        set(COMPILER_FLAGS_RELEASE "/O2")
    endif()
else()
    set(COMPILER_FLAGS_BASE "-Wall -Wextra -Wpedantic -Wno-unused-parameter")
    set(COMPILER_FLAGS_DEBUG "-g -O0")
    set(COMPILER_FLAGS_RELEASE "-O2")
    set(COMPILER_FLAGS_RELWITHDEBINFO "-O2 -g")
    set(COMPILER_FLAGS_MINSIZEREL "-Os")
endif()

set(CMAKE_C_FLAGS_DEBUG "${COMPILER_FLAGS_BASE} ${COMPILER_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "${COMPILER_FLAGS_BASE} ${COMPILER_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${COMPILER_FLAGS_BASE} ${COMPILER_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_MINSIZEREL "${COMPILER_FLAGS_BASE} ${COMPILER_FLAGS_MINSIZEREL}")

if(XVR_DEBUG)
    add_compile_definitions(DEBUG=1 DXVR_DEBUG=1)
    add_compile_definitions(DEBUG_ASSERTIONS=1)
    message(STATUS "Debug mode enabled")
endif()

if(XVR_PROFILE)
    if(WIN32)
        message(WARNING "Profiling not fully supported on Windows")
    else()
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
    message(STATUS "Profiling enabled")
endif()

if(XVR_SANITIZE)
    if(WIN32)
        message(WARNING "Sanitizers not supported on Windows with MSVC")
    else()
        if(XVR_SANITIZE STREQUAL "address")
            add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
            add_link_options(-fsanitize=address)
            message(STATUS "AddressSanitizer enabled")
        elseif(XVR_SANITIZE STREQUAL "undefined")
            add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
            add_link_options(-fsanitize=undefined)
            message(STATUS "UndefinedBehaviorSanitizer enabled")
        elseif(XVR_SANITIZE STREQUAL "thread")
            add_compile_options(-fsanitize=thread)
            add_link_options(-fsanitize=thread)
            message(STATUS "ThreadSanitizer enabled")
        elseif(XVR_SANITIZE STREQUAL "memory")
            add_compile_options(-fsanitize=memory -fno-omit-frame-pointer)
            add_link_options(-fsanitize=memory)
            message(STATUS "MemorySanitizer enabled")
        elseif(XVR_SANITIZE STREQUAL "all")
            add_compile_options(-fsanitize=address,undefined,thread -fno-omit-frame-pointer)
            add_link_options(-fsanitize=address,undefined,thread)
            message(STATUS "All sanitizers enabled")
        else()
            message(WARNING "Unknown sanitizer: ${XVR_SANITIZE}")
        endif()
    endif()
endif()

set(LLVM_FOUND FALSE)
set(LLVM_LIBS "")
set(LLVM_CFLAGS "")

if(WIN32)
    set(LLVM_SEARCH_PATHS 
        "C:/LLVM"
        "C:/Program Files/LLVM"
        "$ENV{LLVM_DIR}"
        "$ENV{LLVM_ROOT_DIR}"
        "C:/Program Files (x86)/LLVM"
    )
    
    foreach(SEARCH_PATH ${LLVM_SEARCH_PATHS})
        if(EXISTS "${SEARCH_PATH}/lib/cmake/llvm/LLVMConfig.cmake")
            set(LLVM_DIR "${SEARCH_PATH}/lib/cmake/llvm")
            break()
        endif()
        if(EXISTS "${SEARCH_PATH}/cmake/llvm/LLVMConfig.cmake")
            set(LLVM_DIR "${SEARCH_PATH}/cmake/llvm")
            break()
        endif()
    endforeach()
    
    if(LLVM_DIR)
        find_package(LLVM CONFIG PATHS "${LLVM_DIR}")
        if(LLVM_FOUND)
            include_directories(${LLVM_INCLUDE_DIRS})
            link_directories(${LLVM_LIBRARY_DIRS})
            list(APPEND LLVM_LIBS ${LLVM_AVAILABLE_LIBS})
            message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} via CMake")
        endif()
    else()
        set(LLVM_COMPONENTS Core BitWriter Support)
        foreach(COMPONENT ${LLVM_COMPONENTS})
            find_library(LLVM_${COMPONENT}_LIB NAMES "LLVM${COMPONENT}" PATHS 
                "C:/LLVM/lib" 
                "C:/Program Files/LLVM/lib"
                NO_DEFAULT_PATH
            )
            if(LLVM_${COMPONENT}_LIB)
                list(APPEND LLVM_LIBS ${LLVM_${COMPONENT}_LIB})
            endif()
        endforeach()
        if(LLVM_LIBS)
            set(LLVM_FOUND TRUE)
            include_directories("C:/LLVM/include" "C:/Program Files/LLVM/include")
            message(STATUS "Found LLVM libraries directly")
        endif()
    endif()
else()
    find_program(LLVM_CONFIG_PROGRAM 
        NAMES llvm-config llvm-config-22 llvm-config-21 llvm-config-20 llvm-config-19 
               llvm-config-18 llvm-config-17 llvm-config-16 llvm-config-15
        DOC "Path to llvm-config"
    )
    
    if(LLVM_CONFIG_PROGRAM)
        message(STATUS "Using llvm-config: ${LLVM_CONFIG_PROGRAM}")
        
        execute_process(
            COMMAND ${LLVM_CONFIG_PROGRAM} --cflags
            OUTPUT_VARIABLE LLVM_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LLVM_CONFIG_RESULT
        )
        
        execute_process(
            COMMAND ${LLVM_CONFIG_PROGRAM} --includedir
            OUTPUT_VARIABLE LLVM_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LLVM_CONFIG_RESULT
        )
        
        execute_process(
            COMMAND ${LLVM_CONFIG_PROGRAM} --libs core bitwriter
            OUTPUT_VARIABLE LLVM_CONFIG_LIBS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LLVM_CONFIG_RESULT
        )
        
        execute_process(
            COMMAND ${LLVM_CONFIG_PROGRAM} --system-libs
            OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LLVM_CONFIG_RESULT
        )
        
        if(LLVM_CONFIG_RESULT EQUAL 0 AND LLVM_CFLAGS)
            set(LLVM_FOUND TRUE)
            add_compile_options(${LLVM_CFLAGS})
            if(LLVM_INCLUDE_DIR)
                include_directories(${LLVM_INCLUDE_DIR})
                message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIR}")
            endif()
            separate_arguments(LLVM_LIBS UNIX_COMMAND "${LLVM_CONFIG_LIBS}")
            separate_arguments(LLVM_SYSTEM_LIBS UNIX_COMMAND "${LLVM_SYSTEM_LIBS}")
            list(APPEND LLVM_LIBS ${LLVM_SYSTEM_LIBS})
            message(STATUS "LLVM configuration successful")
            message(STATUS "LLVM CFLAGS: ${LLVM_CFLAGS}")
        endif()
    endif()
    
    if(NOT LLVM_FOUND)
        find_package(LLVM QUIET)
        if(LLVM_FOUND)
            include_directories(${LLVM_INCLUDE_DIRS})
            link_directories(${LLVM_LIBRARY_DIRS})
            list(APPEND LLVM_LIBS ${LLVM_AVAILABLE_LIBS})
            message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} via CMake")
            message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
        endif()
    endif()
endif()

if(NOT LLVM_FOUND)

    message(WARNING "LLVM not found! Compilation will likely fail.")
    message(WARNING "Install LLVM:")
    message(WARNING "  Windows: https://releases.llvm.org/")
    message(WARNING "  Linux: sudo apt install llvm-dev (Debian)")
    message(WARNING "  Linux: sudo dnf install llvm-devel (Fedora)")
    message(WARNING "  macOS: brew install llvm")
endif()

set(XVR_SOURCES
    ${XVR_SRC_DIR}/xvr_ast_node.c
    ${XVR_SRC_DIR}/xvr_cast_emit.c
    ${XVR_SRC_DIR}/xvr_common.c
    ${XVR_SRC_DIR}/xvr_format_string.c
    ${XVR_SRC_DIR}/xvr_keyword_types.c
    ${XVR_SRC_DIR}/xvr_lexer.c
    ${XVR_SRC_DIR}/xvr_literal.c
    ${XVR_SRC_DIR}/xvr_literal_array.c
    ${XVR_SRC_DIR}/xvr_literal_dictionary.c
    ${XVR_SRC_DIR}/xvr_memory.c
    ${XVR_SRC_DIR}/xvr_parser.c
    ${XVR_SRC_DIR}/xvr_print_handler.c
    ${XVR_SRC_DIR}/xvr_refstring.c
    ${XVR_SRC_DIR}/xvr_runtime.c
    ${XVR_SRC_DIR}/xvr_scope.c
    ${XVR_SRC_DIR}/xvr_semantic.c
    ${XVR_SRC_DIR}/xvr_type.c
    ${XVR_SRC_DIR}/xvr_unused.c
    ${XVR_BACKEND_DIR}/xvr_llvm_codegen.c
    ${XVR_BACKEND_DIR}/xvr_llvm_context.c
    ${XVR_BACKEND_DIR}/xvr_llvm_control_flow.c
    ${XVR_BACKEND_DIR}/xvr_llvm_expression_emitter.c
    ${XVR_BACKEND_DIR}/xvr_llvm_function_emitter.c
    ${XVR_BACKEND_DIR}/xvr_llvm_ir_builder.c
    ${XVR_BACKEND_DIR}/xvr_llvm_module_manager.c
    ${XVR_BACKEND_DIR}/xvr_llvm_optimizer.c
    ${XVR_BACKEND_DIR}/xvr_llvm_target.c
    ${XVR_BACKEND_DIR}/xvr_llvm_type_mapper.c
)

set(COMPILER_SOURCES
    ${XVR_COMPILER_DIR}/main_compiler.c
    ${XVR_COMPILER_DIR}/compiler_tools.c
)

set(XVR_PUBLIC_INCLUDES
    ${XVR_SRC_DIR}
    ${XVR_BACKEND_DIR}
)

if(XVR_BUILD_STATIC)
    add_library(xvr STATIC ${XVR_SOURCES})
    target_include_directories(xvr PUBLIC ${XVR_PUBLIC_INCLUDES})
    target_compile_definitions(xvr PUBLIC XVR_EXPORT_LLVM)
    set_target_properties(xvr PROPERTIES
        OUTPUT_NAME "xvr"
        POSITION_INDEPENDENT_CODE ON
    )
    message(STATUS "Static library: ON")
endif()

if(XVR_BUILD_SHARED)
    add_library(xvr_shared SHARED ${XVR_SOURCES})
    target_include_directories(xvr_shared PUBLIC ${XVR_PUBLIC_INCLUDES})
    target_compile_definitions(xvr_shared PUBLIC XVR_EXPORT_LLVM)
    if(WIN32)
        target_compile_definitions(xvr_shared PUBLIC XVR_EXPORTS)
        set_target_properties(xvr_shared PROPERTIES
            OUTPUT_NAME "xvr"
            SUFFIX ".dll"
        )
    else()
        set_target_properties(xvr_shared PROPERTIES
            OUTPUT_NAME "xvr"
            SUFFIX ".so"
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    endif()
    message(STATUS "Shared library: ON")
endif()

set(XVR_COMPILER_SOURCES
    ${XVR_COMPILER_DIR}/main_compiler.c
    ${XVR_COMPILER_DIR}/compiler_tools.c
)

add_executable(xvr_compiler ${XVR_COMPILER_SOURCES})

if(XVR_BUILD_STATIC)
    target_link_libraries(xvr_compiler PRIVATE xvr)
endif()

if(XVR_BUILD_SHARED)
    target_link_libraries(xvr_compiler PRIVATE xvr_shared)
endif()

target_link_libraries(xvr_compiler PRIVATE ${LLVM_LIBS})
target_include_directories(xvr_compiler PRIVATE 
    ${XVR_SRC_DIR}
    ${XVR_BACKEND_DIR}
    ${XVR_COMPILER_DIR}
)
target_compile_definitions(xvr_compiler PRIVATE XVR_EXPORT_LLVM)

if(WIN32)
    target_compile_definitions(xvr_compiler PRIVATE XVR_IMPORT)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
        set_target_properties(xvr_compiler PROPERTIES 
            LINK_FLAGS "-static-libgcc -static-libstdc++"
        )
    endif()
endif()

set_target_properties(xvr_compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${XVR_OUTPUT_DIR}"
    OUTPUT_NAME "xvr"
)

if(XVR_BUILD_TESTS)
    enable_testing()
    
    set(TEST_SOURCES
        ${XVR_TEST_DIR}/test_all.c
        ${XVR_TEST_DIR}/test_ast_node.c
        ${XVR_TEST_DIR}/test_lexer.c
        ${XVR_TEST_DIR}/test_parser.c
        ${XVR_TEST_DIR}/test_compiler.c
        ${XVR_TEST_DIR}/test_literal.c
        ${XVR_TEST_DIR}/test_memory.c
        ${XVR_TEST_DIR}/test_scope.c
        ${XVR_TEST_DIR}/test_llvm_backend.c
        ${XVR_TEST_DIR}/test_std.c
    )
    
    add_executable(test_all ${TEST_SOURCES})
    target_include_directories(test_all PRIVATE 
        ${XVR_TEST_DIR}
        ${XVR_SRC_DIR}
        ${XVR_BACKEND_DIR}
    )
    target_compile_definitions(test_all PRIVATE XVR_EXPORT_LLVM)
    target_link_libraries(test_all PRIVATE xvr ${LLVM_LIBS})
    
    set_target_properties(test_all PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${XVR_OUTPUT_DIR}"
        OUTPUT_NAME "test_all"
    )
    
    if(WIN32)
        set(TEST_FLAGS /W4 /WX-)
    else()
        set(TEST_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    endif()
    target_compile_options(test_all PRIVATE ${TEST_FLAGS})
    
    add_test(NAME xvr_tests COMMAND test_all)
    message(STATUS "Test suite: ON")
endif()

if(XVR_ENABLE_INSTALL)
    install(TARGETS xvr_compiler
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
    
    if(XVR_BUILD_STATIC)
        install(TARGETS xvr
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT Library
        )
    endif()
    
    if(XVR_BUILD_SHARED)
        install(TARGETS xvr_shared
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT Library
        )
    endif()
    
    message(STATUS "Installation: ON")
endif()
message(STATUS "  Build Configuration Complete")

message(STATUS "")
message(STATUS "To build:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  cmake --build .")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  -DXVR_BUILD_TESTS=ON     Build test suite")
message(STATUS "  -DXVR_BUILD_SHARED=ON    Build shared library")
message(STATUS "  -DXVR_DEBUG=ON           Enable debug mode")
message(STATUS "  -DXVR_SANITIZE=address  Enable sanitizer")
message(STATUS "  -DXVR_PROFILE=ON         Enable profiling")
