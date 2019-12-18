project(MAX30105_debug)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIR})
include_directories(../../libs/piDuino ../../libs/MAX30105)

add_executable(MAX30105_debug main.cpp)

target_link_libraries(MAX30105_debug ${CURSES_LIBRARIES})
target_link_libraries(MAX30105_debug MAX30105)