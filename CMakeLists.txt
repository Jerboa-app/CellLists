cmake_minimum_required(VERSION 3.1)
project(Jerboa)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O3")

set(SFML_DIR "include/SFML-2.5.1/build")

file(GLOB SOURCES "src/*.cpp")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)
find_package(SFML 2.5.1 COMPONENTS system window graphics audio REQUIRED)
find_package(MPI REQUIRED)

include_directories(include ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${MPI_INCLUDE_DIRS})

add_executable(Jerboa ${SOURCES})

target_link_libraries(Jerboa sfml-system sfml-window sfml-graphics sfml-audio X11 ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${FREETYPE_LIBRARIES} ${MPI_LIBRARIES})
