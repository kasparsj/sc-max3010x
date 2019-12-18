Max30102 : UGen {
    /*
    Channels:
    0: IR_AC
    1: IR_BEAT
    2: IR_RAW
    3: RED_RAW
    4: IR_BPM
    5: HZ
    6: TEMP_C
    7: IR_AVG_DC_EST
    */
    *kr {
        arg channel = 1, ledBrightness = 31, sampleAverage = 1, ledMode = 2, sampleRate = 800, mul = 1.0, add = 0;

        ^this.multiNew('control', channel, ledBrightness, sampleAverage, ledMode).madd(mul, add)
    }

}
