project(MAX30105)
add_library(MAX30105 MAX30105.cpp heartRate.cpp spo2_algorithm.cpp)

include_directories(../piDuino)
target_link_libraries(MAX30105 piDuino)
