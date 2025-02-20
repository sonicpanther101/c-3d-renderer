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

file(GLOB SOURCES "src/*.cpp" "src/*.h")

find_package(OpenGL REQUIRED)

# GLFW
add_subdirectory("vendor/glfw")

# GLEW
add_subdirectory("vendor/glew-cmake/build/cmake")
include_directories("vendor/glew-cmake/include")

add_executable(${PROJECT_NAME} ${SOURCES})

# Link the engine
target_link_libraries(${PROJECT_NAME} ${OPEN_GL_LIBRARIES} glew glfw)

# print_all_variables()
