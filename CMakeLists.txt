cmake_minimum_required(VERSION 3.20)
project(FXLCompiler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/compiler/include)

# Compiler library sources (without main.cpp)
set(COMPILER_LIB_SOURCES
    compiler/src/diagnostic.cpp
    compiler/src/token.cpp
    compiler/src/lexer.cpp
    compiler/src/ast.cpp
    compiler/src/parser.cpp
    compiler/src/semantic.cpp
    compiler/src/auto_resolver.cpp
    compiler/src/codegen.cpp
)

# Compiler library
add_library(fxl_compiler STATIC ${COMPILER_LIB_SOURCES})
target_include_directories(fxl_compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/compiler/include)

# Main executable
add_executable(fxlc compiler/src/main.cpp)
target_link_libraries(fxlc fxl_compiler)

# Install targets
install(TARGETS fxlc DESTINATION bin)
install(TARGETS fxl_compiler DESTINATION lib)
install(DIRECTORY compiler/include/ DESTINATION include/fxl)

# Runtime library
add_subdirectory(runtime)

# Enable testing
enable_testing()

# Add tests directory
add_subdirectory(tests)