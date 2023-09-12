cmake_minimum_required (VERSION 3.11)

project (myProject)

find_package (OpenGL REQUIRED)

add_executable (main main.cpp)

add_subdirectory (external)

target_include_directories (main PRIVATE ${GLFW_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${GLM_INCLUDE_DIR})

target_link_libraries (main glfw glew_s glm ${OPENGL_LIBRARIES})
