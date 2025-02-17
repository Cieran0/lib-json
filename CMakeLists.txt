cmake_minimum_required(VERSION 3.10)

# Project name and version
project(lib-json VERSION 1.0 LANGUAGES C)

# Specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Directories for headers and sources
include_directories(include)
include_directories(include/internal)
file(GLOB_RECURSE SOURCES src/*.c)

# Output library name
set(LIBRARY_NAME json)

# Add library
add_library(${LIBRARY_NAME} STATIC ${SOURCES})

# Directory to clone and build lib-tokenise
set(LIB_TOKENISE_DIR ${CMAKE_BINARY_DIR}/lib-tokenise)

# Step 1: Clone the repository if it doesn't exist
if(NOT EXISTS ${LIB_TOKENISE_DIR})
    message(STATUS "Cloning lib-tokenise...")
    execute_process(
        COMMAND git clone https://github.com/cieran0/lib-tokenise.git ${LIB_TOKENISE_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Step 2: Build lib-tokenise using its Makefile
add_custom_target(build_lib_tokenise ALL
    COMMAND make
    WORKING_DIRECTORY ${LIB_TOKENISE_DIR}
)

# Path to the built lib-tokenise library
set(LIB_TOKENISE_LIB ${LIB_TOKENISE_DIR}/libtokenise.a)

# Ensure the library is built before linking
add_dependencies(${LIBRARY_NAME} build_lib_tokenise)

# Link lib-tokenise to the library
target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIB_TOKENISE_LIB})

# Include lib-tokenise headers
include_directories(${LIB_TOKENISE_DIR}/include)

# Install step (optional)
install(TARGETS ${LIBRARY_NAME} DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
