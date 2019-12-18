#include <thread>
#include <atomic>
#include "SC_Lock.h"
#include "SC_PlugIn.h"

#include "MAX30105.h"
#include "heartRate.h"
#include "Wire.h"

// written with reference to the chapter "Writing Unit Generator Plug-ins" in The SuperCollider Book
// and also http://doc.sccode.org/Guides/WritingUGens.html accessed March 2, 2015
//

const byte RATE_SIZE = 4;

struct Max30102Thread;

struct Max30102Sample {
    uint32_t red, ir;
    bool isBeat;
    float tempC;
};

struct Max30102BPM {
    byte rates[RATE_SIZE]; //Array of heart rates
    byte rateSpot = 0;
    long lastBeat = 0; //Time at which the last beat occurred

    float beatsPerMinute;
    int beatAvg;
};

struct Max30102 : public Unit {
    static const int IR_AC = 0;
    static const int IR_BEAT = 1;
    static const int IR_RAW = 2;
    static const int RED_RAW = 3;
    static const int IR_BPM = 4;
    static const int HZ = 5;
    static const int TEMP_C = 6;
    static const int IR_AVG_DC_EST = 7;

    int channel = IR_AC;
    unsigned char ledBrightness = 0x1F;
    unsigned char sampleAverage = 1;
    unsigned char ledMode = 2;
    int sampleRate = 400;
    int outputs = 1;
    Max30102Thread* thread;
    Max30102Sample sample;
    Max30102BPM bpm;
    unsigned long samplesTaken = 0;
};

unsigned long startTime;
WireLinux wireLinux;
MAX30105 sensor;
//byte ledBrightness = 0xFF; // too bright for the sensor -> caps at 262143
//uint32_t irStartup = 0; //Average at power up
int32_t irOldDCAvgEst;

// PLUGIN INTERFACE

extern "C" {
    void Max30102_Ctor(Max30102 *unit);
    void Max30102_Dtor(Max30102 *unit);
    void Max30102_next_k(Max30102 *unit, int numSamples);
}

struct Max30102Thread {
    std::thread* thread;
    bool running;


    void join() {
        running = false;
        thread->join();
        delete thread;
    }

    void run(Max30102* unit) {
        running = true;
        thread = new std::thread(&Max30102Thread::loop, this, unit);
    }

    void readSample(Max30102 *unit) {
        uint32_t ir = sensor.getFIFOIR();
//                uint32_t irDelta = ir - irStartup;
//                if (irDelta > 50000) {
        if (ir > (uint32_t) 80000) {
            unit->sample.ir = ir;
            unit->sample.red = sensor.getFIFORed();
        }
        else {
            unit->sample.ir = 0;
            unit->sample.red = 0;
        }
        unit->samplesTaken++;
    }

    void checkBeat(Max30102 *unit) {
        bool isBeat = checkForBeat(unit->sample.ir);
        if (isBeat && unit->channel == Max30102::IR_BPM) {
            long delta = millis() - unit->bpm.lastBeat;
            unit->bpm.lastBeat = millis();

            unit->bpm.beatsPerMinute = 60 / (delta / 1000.0);

            if (unit->bpm.beatsPerMinute < 255 && unit->bpm.beatsPerMinute > 20)
            {
                unit->bpm.rates[unit->bpm.rateSpot++] = (byte) unit->bpm.beatsPerMinute; //Store this reading in the array
                unit->bpm.rateSpot %= RATE_SIZE; //Wrap variable

                //Take average of readings
                unit->bpm.beatAvg = 0;
                for (byte x = 0 ; x < RATE_SIZE ; x++)
                    unit->bpm.beatAvg += unit->bpm.rates[x];
                unit->bpm.beatAvg /= RATE_SIZE;
            }
        }
        unit->sample.isBeat = isBeat;
    }

    void loop(Max30102* unit) {
        while (running) {
            sensor.check();
            while (sensor.available()) {
                readSample(unit);
                checkBeat(unit);
                sensor.nextSample();
            }
            //std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
};

static InterfaceTable *ft;

// PLUGIN IMPLEMENTATION

void Max30102_next(Max30102 *unit, int numSamples) {

    float value[3] = {0,0,0};
    int outputs = unit->outputs;

    int16_t irAvgDcEst = averageDCEstimator(&irOldDCAvgEst, unit->sample.ir);
    switch (unit->channel) {
        case Max30102::RED_RAW:
            value[0] = unit->sample.red;
            break;
        case Max30102::TEMP_C:
            value[0] = unit->sample.tempC;
            break;
        case Max30102::IR_RAW:
            value[0] = unit->sample.ir;
            break;
        case Max30102::IR_BPM:
            value[0] = unit->bpm.beatAvg;
            break;
        case Max30102::HZ:
            value[0] = (float)unit->samplesTaken / ((millis() - startTime) / 1000.0);
            break;
        case Max30102::IR_BEAT:
            value[0] = unit->sample.isBeat ? 1.0 : 0.0;
            break;
        case Max30102::IR_AVG_DC_EST:
            value[0] = irAvgDcEst;
            break;
        case Max30102::IR_AC:
        default:
            value[0] = lowPassFIRFilter(unit->sample.ir - irAvgDcEst);
            break;
    }

    for (int i = 0; i < numSamples ; i++) {
        for (int o = 0; o < outputs; o++) {
            OUT(o)[i] = value[o];
            //OUT(0) = 0;
            //OUT(1) = 0;
            //OUT(2) = 0;
        }
    }
}

void Max30102_Ctor(Max30102 *unit) {
    unit->channel = static_cast<int>(IN0(0));
    unit->ledBrightness = static_cast<unsigned char>(IN0(1));
    unit->sampleAverage = static_cast<unsigned char>(IN0(2));
    unit->ledMode = static_cast<unsigned char>(IN0(3));
    unit->sampleRate = static_cast<int>(IN0(4));
    unit->outputs = 1;

    sensor.begin(wireLinux);
    sensor.setup(unit->ledBrightness, unit->sampleAverage, unit->ledMode, unit->sampleRate);
    sensor.setPulseAmplitudeGreen(0); //Turn off Green LED

    //Take an average of IR readings at power up
//    irStartup = 0;
//    for (byte x = 0 ; x < 32 ; x++) {
//        irStartup += sensor.getIR();
//    }
//    irStartup /= 32;
    startTime = millis();

    unit->thread = new Max30102Thread();
    unit->thread->run(unit);

    SETCALC(Max30102_next);
    Max30102_next(unit, 1);
}

void Max30102_Dtor(Max30102 *unit) {
    unit->thread->join();

}

PluginLoad(Max30102)
{
    ft = inTable;
    DefineDtorUnit(Max30102);
}
