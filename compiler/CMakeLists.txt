set(COMPILER_SOURCES
    main_compiler.c
    compiler_tools.c
)

set(COMPILER_HEADERS
    compiler_tools.h
)

add_executable(xvr ${COMPILER_SOURCES} ${COMPILER_HEADERS})

target_include_directories(xvr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/backend
    ${CMAKE_SOURCE_DIR}/src/adapters/llvm
    ${CMAKE_SOURCE_DIR}/src/optimizer
)

target_compile_options(xvr PRIVATE
    ${XVR_COMPILE_FLAGS}
    -DXVR_IMPORT
    -DXVR_EXPORT_LLVM
)

target_link_libraries(xvr PRIVATE xvr_objects xvr_llvm_libs m ${XVR_LINK_FLAGS})

set_target_properties(xvr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${XVR_OUTPUT_DIR}
)

install(TARGETS xvr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
