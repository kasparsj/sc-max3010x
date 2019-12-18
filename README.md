# sc-max3010x

SuperCollider plugin to read data from MAX30102 and MAX30105 over i2c.

Based on https://github.com/jpburstrom/sc-mpu9250

This project will build:

 * MPU: a SuperCollider UGen plugin
 * debug: an executable that reads the data from MAX3010x over i2c and prints it

### To build:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DSC_PATH=/path/to/supercollider/source ../
    make 

### To test / debug:

    ./build/apps/MAX30105_debug/MAX30105_debug
    
### To install SuperCollider extension

    cp plugin/MAX30105.so ~/.local/share/SuperCollider/Extensions/MAX30105/plugins/
    cp plugin/MAX30105.sc ~/.local/share/SuperCollider/Extensions/MAX30105/classes/
    
### In SuperCollider

    { Max30102.kr(0) * WhiteNoise.ar!2 * 0.1 }.play; // white noise modulated by pulse