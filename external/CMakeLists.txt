add_subdirectory (glfw)

set (GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extern/glfw/include)

set (GLEW_USE_STATIC_LIBS TRUE)
set (GLEW_VERBOSE TRUE)
add_subdirectory (glew-cmake/build/cmake)

set (GLEW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extern/glew-cmake/include)

add_subdirectory (glm)

set (GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extern/glm/include)
