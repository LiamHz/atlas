cmake_minimum_required(VERSION 3.13)  # CMake version check
project(Atlas)               # create project "Atlas"
set(CMAKE_CXX_STANDARD 14)            # enable c++14 standard

# add source files
set(SOURCE_FILES lib/glad.c src/main.cpp)

# add executable target with source files listed in SOURCE_FILES variable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# find packages
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

# link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC glfw ${OPENGL_LIBRARIES})

# include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})