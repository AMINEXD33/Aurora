cmake_minimum_required(VERSION 3.23)
enable_testing()
#data source
set(DATAPOINT_SOURCES
    data.c)

#json source
set(JSON_SOURCES
    json.c)

# files source
set(FILES_SOURCES
    files.c)

# add libs , shared 
add_library(datapoint SHARED ${DATAPOINT_SOURCES})
add_library(json SHARED ${JSON_SOURCES})
add_library(files SHARED ${FILES_SOURCES})

set_target_properties(datapoint PROPERTIES
    # Output name: libdatapoint.so (without this, it uses the target name)
    OUTPUT_NAME "datapoint"
    # Library version: creates libdatapoint.so.1.0.0
    VERSION 1.1.0
    # SO version: creates symlink libdatapoint.so.1 -> libdatapoint.so.1.0.0
    # This is for ABI compatibility tracking
    SOVERSION 1
    # Position Independent Code (required for shared libraries)
    POSITION_INDEPENDENT_CODE ON
)
set_target_properties(json PROPERTIES
    OUTPUT_NAME "json"
    VERSION 1.3.0
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
)
set_target_properties(files PROPERTIES
    OUTPUT_NAME "files"
    VERSION 1.0.1
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
)

# include where to find the helpers headers for each lib
target_include_directories(datapoint PUBLIC
    ${PROJECT_SOURCE_DIR}/engine/helpers
)
target_include_directories(json PUBLIC
    ${PROJECT_SOURCE_DIR}/engine/helpers
)
target_include_directories(files PUBLIC
    ${PROJECT_SOURCE_DIR}/engine/helpers
)

# 
# find_package(cJSON REQUIRED)

target_include_directories(json PRIVATE /usr/local/include/cjson)
target_include_directories(files PRIVATE /usr/local/include/cjson)

target_link_libraries(json PRIVATE cjson)
target_link_libraries(files PRIVATE cjson)

# get versions 
get_target_property(DATAPOINT_VER datapoint VERSION)
get_target_property(JSON_VER json VERSION)
get_target_property(FILES_VER files VERSION)

# Add compile definitions (like #define in C)
# PRIVATE: only when compiling this library
# PUBLIC: also for anyone who links to this library
target_compile_definitions(datapoint PUBLIC
    # Define a version macro
    DATAPOINT_VERSION="${DATAPOINT_VER}"
)
target_compile_definitions(json PUBLIC
    # Define a version macro
    JSON_VERSION="${JSON_VER}"
)

target_compile_definitions(files PUBLIC
    # Define a version macro
    FILES_VERSION="${FILES_VER}"
)


# Install the library to /usr/local/lib (or CMAKE_INSTALL_PREFIX/lib)
install(TARGETS datapoint
    # Where to install the .so file
    LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/lib
    
    # Where to install the .a file (if you made a static version)
    ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/lib
    
    # Where to install .dll on Windows
    RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/bin
)
install(TARGETS json
    LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/lib
    ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/lib
    RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/bin
)
install(TARGETS files
    LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/lib
    ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/lib
    RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/compiled/bin
)

add_executable(testwhole test.c)

# libs
target_link_libraries(testwhole PUBLIC
    datapoint
    json      # json already links to cjson
    files     # files already links to cjson
    cjson
)
# headers
target_include_directories(testwhole PRIVATE 
    /usr/local/include/cjson
    ${PROJECT_SOURCE_DIR}/engine/helpers
)
# set some target properties
set_target_properties(testwhole PROPERTIES
    # During build, look in build directory for .so files
    BUILD_RPATH "${CMAKE_BINARY_DIR}"
    
    # After install, look in same directory as executable
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
    
    # Don't use install rpath during build
    BUILD_WITH_INSTALL_RPATH FALSE
)

# Register the test with CTest
add_test(NAME test1 COMMAND testwhole)

message(STATUS "Configured library: datapoint")
message(STATUS "Configured library: json")
message(STATUS "Configured library: files")

message(STATUS "  Sources: ${DATAPOINT_SOURCES}")
message(STATUS "  Sources: ${JSON_SOURCES}")
message(STATUS "  Sources: ${FILES_SOURCES}")