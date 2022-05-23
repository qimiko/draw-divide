
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(draw-divide VERSION 1.0.0)

find_file(WINDOWS_HEADER windows.h)
if(NOT WINDOWS_HEADER)
  message(FATAL_ERROR "Can't find windows.h!")
endif()

add_library(draw-divide SHARED 
  src/game_hooks.cpp
  src/dllmain.cpp
)

target_include_directories(draw-divide PRIVATE
  libraries/minhook/include
  libraries/Mega-Hack-Extensions/extensions1/
)

add_subdirectory(libraries/minhook)
target_link_libraries(draw-divide minhook)

add_subdirectory(libraries/cocos-headers)
target_link_libraries(draw-divide cocos2d)

set_target_properties(draw-divide PROPERTIES PREFIX "")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
