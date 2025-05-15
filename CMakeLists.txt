cmake_minimum_required(VERSION 3.30)

project(C++-renderer)

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

find_package(OpenGL REQUIRED)

# GLFW
add_subdirectory("vendor/glfw")
include_directories("vendor/glfw/include")

# GLEW
add_subdirectory("vendor/glew-cmake/build/cmake")
include_directories("vendor/glew-cmake/include")

# Assimp
add_subdirectory("vendor/assimp")

# ImGui
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui")
# Specify the backend directory
set(IMGUI_BACKEND_DIR "${IMGUI_DIR}/backends")
include_directories(${IMGUI_DIR} ${IMGUI_BACKEND_DIR})


# Collect ImGui source files
file(GLOB IMGUI_SOURCE "${IMGUI_DIR}/*.cpp" "${IMGUI_DIR}/*.h")

# Collect backend include files (if needed, typically not necessary for linking)
file(GLOB IMGUI_BACKENDS_INCLUDE
    "${IMGUI_BACKEND_DIR}/imgui_impl_glfw.h"
    "${IMGUI_BACKEND_DIR}/imgui_impl_opengl3.h"
)

# Collect backend source files
file(GLOB IMGUI_BACKENDS_SOURCE
    "${IMGUI_BACKEND_DIR}/imgui_impl_glfw.cpp"
    "${IMGUI_BACKEND_DIR}/imgui_impl_opengl3.cpp"
)

# Create the ImGui library, including both ImGui and backend source files
add_library(imgui ${IMGUI_SOURCE} ${IMGUI_BACKENDS_SOURCE})

# ImGui File Dialog
set(IMGUI_FILE_DIALOG_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui-file-dialog")
include_directories(${IMGUI_FILE_DIALOG_DIR})
file(GLOB IMGUI_FILE_DIALOG_SOURCE "${IMGUI_FILE_DIALOG_DIR}/*.cpp" "${IMGUI_FILE_DIALOG_DIR}/*.h")

add_library(imguiFileDialog ${IMGUI_FILE_DIALOG_SOURCE})

file(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(${PROJECT_NAME} ${SOURCES})

# Link the engine
target_link_libraries(
    ${PROJECT_NAME} 
    ${OPEN_GL_LIBRARIES} 
    glew 
    glfw 
    assimp 
    imgui 
    imguiFileDialog
)

# Link the headers
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)


# print_all_variables()

# Copy dependencies to output directory (even if I dont need, its still cool)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:glew> $<TARGET_FILE_DIR:${PROJECT_NAME}>
)