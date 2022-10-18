set(TARGET_NAME jackal_ctrl)

# Define source files
define_source_files ()

define_resource_dirs (GLOB_PATTERNS ${CMAKE_SOURCE_DIR}/deploy/Data ${CMAKE_SOURCE_DIR}/deploy/CoreData)

# Setup target with resource copying
setup_main_executable ()