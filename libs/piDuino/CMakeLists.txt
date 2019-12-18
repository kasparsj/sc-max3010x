project(piDuino)
find_package (Threads)
add_library(piDuino Core.cpp Serial.cpp Wire.cpp SPI.cpp WString.cpp piDuino.h)

include_directories(../piDuino)
target_link_libraries(piDuino ${CMAKE_THREAD_LIBS_INIT})
