cmake_minimum_required(VERSION 3.11.0)

project(build_and_battle)

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS ON)

set(JACKAL_CONTROL_VERSION "1.0.0")
set(JACKAL_CONTROL_BASE_NAME "jackal_control")
set(JACKAL_CONTROL_TARGET_NAME "${JACKAL_CONTROL_BASE_NAME}-v${JACKAL_CONTROL_VERSION}")

set(JACKAL_CONTROL_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(URHO3D_INCLUDE_DIR ${URHO3D_PATH}/include)
set(THIRD_PARTY_INCLUDE_DIR ${URHO3D_INCLUDE_DIR}/Urho3D/ThirdParty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the src files for the project
file(GLOB JACKAL_CONTROL_SRC_FILES
    "${JACKAL_CONTROL_SRC_DIR}/*.cpp"
    "${JACKAL_CONTROL_SRC_DIR}/game/*.cpp"
)

# Set project includes dir
include_directories(
    "${JACKAL_CONTROL_SRC_DIR}"
    "${URHO3D_INCLUDE_DIR}"
    "${THIRD_PARTY_INCLUDE_DIR}"
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_definitions(-DDEBUG_VERSION)
else()
    add_definitions(-DRELEASE_VERSION)
endif()

find_library(URHO3D_LIB
    NAMES Urho3D libUrho3D
    HINTS ${URHO3D_PATH}/lib64
)

add_definitions(-DURHO3D_ANGELSCRIPT)
add_definitions(-DURHO3D_LUA)
add_definitions(-DURHO3D_NETWORK)
add_definitions(-DURHO3D_PHYSICS)

add_executable(${JACKAL_CONTROL_TARGET_NAME} ${JACKAL_CONTROL_SRC_FILES})

# copy all the files in the deploy folder to bin
add_custom_target(deploy_files ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/deploy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    DEPENDS ${JACKAL_CONTROL_TARGET_NAME}
)

# link with opengl (no debug version)
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(ERROR "Error: OpenGL not found on your system - please make sure you have it downloaded.")
endif(NOT OPENGL_FOUND)

# Link in the unix only stuff
if(UNIX)
    target_link_libraries(${JACKAL_CONTROL_TARGET_NAME} pthread)
endif()

# link in the libraries for Urho3d
target_link_libraries(${JACKAL_CONTROL_TARGET_NAME}
    ${URHO3D_LIB}
    ${OPENGL_LIBRARIES}
    spdlog
    fmt
)
