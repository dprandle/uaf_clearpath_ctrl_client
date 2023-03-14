
# Set CMake minimum version and CMake policy required by UrhoCommon module
cmake_minimum_required (VERSION 3.2.3)
if (COMMAND cmake_policy)
    # Libraries linked via full path no longer produce linker search paths
    cmake_policy (SET CMP0003 NEW)
    # INTERFACE_LINK_LIBRARIES defines the link interface
    cmake_policy (SET CMP0022 NEW)
    # Disallow use of the LOCATION target property - so we set to OLD as we still need it
    cmake_policy (SET CMP0026 OLD)
    # MACOSX_RPATH is enabled by default
    cmake_policy (SET CMP0042 NEW)
    # Honor the visibility properties for SHARED target types only
    cmake_policy (SET CMP0063 OLD)
endif ()


# Set project name
project (client_ctrl)
set(CMAKE_CXX_STANDARD 20)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_definitions(-DDEBUG_VERSION)
else()
    add_definitions(-DRELEASE_VERSION)
endif()

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${URHO3D_SRC}/cmake/Modules)

# Include UrhoCommon.cmake module after setting project name
include (UrhoCommon)

find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS})

# Define target name
add_subdirectory(src)

set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/../jackal_control_server/src/emscripten)
