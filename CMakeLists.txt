cmake_minimum_required(VERSION 3.23)
project(Aurora)
enable_testing()
# Set the C standard to C11 (could be C99, C11, C17, etc.)
set(CMAKE_C_STANDARD 11)

# Make the C standard required (don't fall back to older standards)
set(CMAKE_C_STANDARD_REQUIRED True)

# Export compile commands (useful for IDEs and tools like clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add debuging if needed
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -Wall -Wextra -Wpedantic)
    message(STATUS "Building in Debug mode")
elseif(CMAKE_BUILD_TYPE STREQUAL "Performance")
    add_compile_options(-O3 -Wall -Wextra -Wpedantic)
    message(STATUS "Building in Performace mode")
else()
    add_compile_options(-O2 -Wall -Wextra -Wpedantic)
    message(STATUS "Building in Normal mode")
endif()


# Global include directory
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    /usr
)

# Add subdirectories
add_subdirectory(engine/helpers)
# add_subdirectory(lib)
# add_subdirectory(tests)


# Put all compiled libraries in build/lib/
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Put all executables in build/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Put all archives (.a files) in build/lib/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# install public headers /usr/local/include/aurora/
install(DIRECTORY include/aurora
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)


message(STATUS "")
message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")