# sc-max3010x

Supercollider plugin to read data from MAX30102 and MAX30105 over i2c.

Based on https://github.com/jpburstrom/sc-mpu9250

This project will build:

 * MPU: a SuperCollider UGen plugin
 * debug: an executable that reads the data from MAX3010x over i2c and prints it

### To build:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DSC_PATH=/path/to/supercollider/source ../
    make 

### To run:

    sudo ./build/apps/MAX30105_debug/MAX30105_debug

### To build a debug version:

    mkdir debug
    cd debug
    cmake -DCMAKE_BUILD_TYPE=Debug -DSC_PATH=/path/to/supercollider/source ../
    make 
