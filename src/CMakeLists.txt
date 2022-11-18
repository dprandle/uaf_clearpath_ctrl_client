set(TARGET_NAME jackal_ctrl)

# Define source files
define_source_files ()

define_resource_dirs (GLOB_PATTERNS ${CMAKE_SOURCE_DIR}/deploy/* HTML_SHELL ${CMAKE_SOURCE_DIR}/deploy/shell.html)

# Setup target with resource copying
setup_main_executable ()

if (DEFINED EMSCRIPTEN)
    target_link_libraries(${TARGET_NAME} websocket.js)
endif()

# if (${EMSCRIPTEN})
# set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY
#     LINK_FLAGS_DEBUG " -s SOCKET_DEBUG=1")
# endif()
