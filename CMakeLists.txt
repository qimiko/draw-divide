cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(draw-divide VERSION 1.0.0)

add_library(draw-divide SHARED src/main.cpp src/nodes.cpp)

target_compile_definitions(draw-divide PRIVATE CC_DLL= NOMINMAX)

target_include_directories(draw-divide PRIVATE
  libraries/minhook/include
  libraries/Mega-Hack-Extensions/extensions1/
  libraries/gd.h
  libraries/gd.h/include
)

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)
add_subdirectory(libraries/mat-dash)

target_link_libraries(draw-divide minhook cocos2d mat-dash)

