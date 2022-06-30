cmake_minimum_required(VERSION 3.1)
project(Jerboa)

set(BUILD_SHARED_LIBRARY FALSE)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O3")

set(SFML_STATIC_LIBRARIES TRUE)
message("SFML",${SFML_DIR})
set(GLEW_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libGLEW.a")

set(FREETYPE_STATIC_LIBRARIES TRUE)
set(PNG_STATIC_LIBRARIES TRUE)
set(ZLIB_STATIC_LIBRARIES TRUE)

set(FREETYPE_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include/freetype")
set(FREETYPE_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libfreetype.a")

set(ZLIB_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libz.a")
set(PNG_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libpng16.a")

find_package(OpenGL REQUIRED)
find_package(SFML 2.5.1 COMPONENTS system window graphics audio REQUIRED)
include_directories(include ${OPENGL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})

file(GLOB SOURCES "src/*.cpp")

add_executable(Jerboa ${SOURCES})

target_link_libraries(Jerboa sfml-system sfml-window sfml-graphics sfml-audio X11 ${FREETYPE_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
