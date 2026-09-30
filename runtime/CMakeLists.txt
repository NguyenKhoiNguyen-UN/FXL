# Runtime library CMakeLists.txt

# Runtime backend (Win32 + OpenGL)
add_library(fxl_runtime_backend STATIC
    src/backend_win32.cpp
)

target_include_directories(fxl_runtime_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Windows libraries
target_link_libraries(fxl_runtime_backend PUBLIC
    opengl32
    gdi32
    user32
)

# Runtime interface library (header-only)
add_library(fxl_runtime INTERFACE)
target_include_directories(fxl_runtime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fxl_runtime INTERFACE fxl_runtime_backend)

# Install
install(DIRECTORY include/ DESTINATION include/fxl/runtime)
install(TARGETS fxl_runtime_backend DESTINATION lib)