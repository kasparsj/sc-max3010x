cmake_minimum_required(VERSION 2.8)
set(PROJECT "SC-Max30102")
project (${PROJECT})

set(CMAKE_POSITION_INDEPENDENT_CODE True)
message(${CMAKE_CXX_FLAGS_RELEASE})

# hack to get c++11 + warnings
# add_definitions(-std=c++11 -Wall)

add_subdirectory(libs/piDuino)
add_subdirectory(libs/MAX30105)

add_subdirectory(apps/MAX30105_debug)

add_subdirectory(plugin)
