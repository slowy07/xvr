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
)

target_compile_options(xvr PRIVATE
    ${XVR_COMPILE_FLAGS}
    -DXVR_IMPORT
    -DXVR_EXPORT_LLVM
)

if(XVR_SANITIZER)
    target_link_libraries(xvr PRIVATE xvr_objects -fsanitize=${XVR_SANITIZER} xvr_llvm_libs)
elseif(XVR_BUILD_STATIC)
    target_link_libraries(xvr PRIVATE xvr_static xvr_llvm_libs)
else()
    target_link_libraries(xvr PRIVATE xvr_shared xvr_llvm_libs)
endif()

set_target_properties(xvr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${XVR_OUTPUT_DIR}
)

install(TARGETS xvr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
