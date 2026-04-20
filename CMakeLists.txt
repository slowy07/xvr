cmake_minimum_required(VERSION 3.20)
project(xvr LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(XVR_BUILD_TESTS "Build test suite" ON)
option(XVR_BUILD_SHARED "Build shared library" ON)
option(XVR_BUILD_STATIC "Build static library" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(XVR_DEBUG ON)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND XVR_COMPILE_FLAGS -O2)
endif()

set(XVR_OUTPUT_DIR ${CMAKE_BINARY_DIR})
set(XVR_LIB_DIR ${XVR_OUTPUT_DIR})
set(XVR_BIN_DIR ${XVR_OUTPUT_DIR})

include(GNUInstallDirs)

find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

set(XVR_COMPILE_FLAGS
    -std=c++20
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-missing-field-initializers
)

if(XVR_DEBUG)
    list(APPEND XVR_COMPILE_FLAGS -g -DDEBUG -DXVR_DEBUG=1)
    list(APPEND XVR_COMPILE_FLAGS -fno-omit-frame-pointer)
endif()

if(XVR_SANITIZE)
    if(XVR_SANITIZE STREQUAL "address")
        list(APPEND XVR_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
        list(APPEND XVR_LINK_FLAGS -fsanitize=address)
    elseif(XVR_SANITIZE STREQUAL "undefined")
        list(APPEND XVR_COMPILE_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
        list(APPEND XVR_LINK_FLAGS -fsanitize=undefined)
    elseif(XVR_SANITIZE STREQUAL "thread")
        list(APPEND XVR_COMPILE_FLAGS -fsanitize=thread)
        list(APPEND XVR_LINK_FLAGS -fsanitize=thread)
    elseif(XVR_SANITIZE STREQUAL "memory")
        list(APPEND XVR_COMPILE_FLAGS -fsanitize=memory -fno-omit-frame-pointer)
        list(APPEND XVR_LINK_FLAGS -fsanitize=memory)
    elseif(XVR_SANITIZE STREQUAL "all")
        list(APPEND XVR_COMPILE_FLAGS -fsanitize=address,undefined,thread -fno-omit-frame-pointer)
        list(APPEND XVR_LINK_FLAGS -fsanitize=address,undefined,thread)
    endif()
endif()

add_subdirectory(src)
add_subdirectory(compiler)

if(XVR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

install(DIRECTORY ${XVR_BIN_DIR}/ DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${XVR_LIB_DIR}/ DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)

message(STATUS "")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "XVR Debug: ${XVR_DEBUG}")
message(STATUS "Output directory: ${XVR_OUTPUT_DIR}")
message(STATUS "")