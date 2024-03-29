cmake_minimum_required(VERSION 3.6)   # CMake version check
project(compute)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 11)            # Enable c++11 standard

#add source folder so the program can be compiled at build folder
include_directories(${CMAKE_SOURCE_DIR})

#Headers
file(GLOB HEADERS
  *.h
  *.cpp
  )

#Main
set(SOURCE_FILES main.cpp)
add_executable(compute ${SOURCE_FILES} ${HEADERS})

#Shaders
configure_file(raymarch.comp raymarch.comp COPYONLY)

#OpenGL
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
target_link_libraries(compute ${OPENGL_gl_LIBRARY})

#GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})
target_link_libraries(compute ${GLFW_LIBRARIES})

#GLEW
find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries(compute ${GLEW_LIBRARIES})
endif()



