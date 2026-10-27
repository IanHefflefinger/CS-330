cmake_minimum_required(VERSION 3.15)
project(Scene3D CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(scene Source.cpp camera.h)
target_link_libraries(scene PRIVATE OpenGL::GL glfw GLEW::GLEW glm::glm)

if(APPLE)
  # macOS deprecated OpenGL but it still works; hide the warnings
  target_compile_definitions(scene PRIVATE GL_SILENCE_DEPRECATION)
endif()

# Put the executable next to Source.cpp so the relative texture path
# (../resources/textures/...) resolves when you run it from this folder.
set_target_properties(scene PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")
