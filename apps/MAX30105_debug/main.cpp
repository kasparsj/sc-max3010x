#include <signal.h>
#include <unistd.h>
#include <iostream>

#include <stdio.h>
#include <ncurses.h>

#include "MAX30105.h"
#include "heartRate.h"
#include "Wire.h"

struct Sample {
    int red, ir, green, temp;
};

bool running = true;
MAX30105 sensor;
long startTime;
long samplesTaken = 0;
byte ledMode = 2;
byte powerLevel = 0x1F;
byte sampleAverage = 1;
int sampleRate = 400;
bool showTemp = false;
bool showHz = true;
bool showBeat = false;
bool showBpm = true;
bool showAvgDcEst = false;
bool showAcCurr = false;
bool fifoMode = true;
bool showPlot = false;
Sample sample;
WireLinux wireLinux;
int32_t irOldDCAvgEst = 0;
int32_t irOldDCAvgWeighted = 0;

const byte RATE_SIZE = 4;
struct BPM {
    byte rates[RATE_SIZE]; //Array of heart rates
    byte rateSpot = 0;
    long lastBeat = 0; //Time at which the last beat occurred

    float beatsPerMinute;
    int beatAvg;
};
BPM bpm;

void kill_handler(int s) {
    running = false;
}

int kbhit()
{
    int ch;

    // turn off getch() blocking and echo
    nodelay(stdscr, TRUE);
    noecho();

    // check for input
    ch = getch();

    // restore block and echo
    echo();
    nodelay(stdscr, FALSE);
    return ch;
}

void keyPressed() {
    int c = kbhit();
    if (c != ERR) {
        switch (c) {
            case '1':
            case '2':
            case '3':
                ledMode = (c - '1' + 1);
                sensor.setup(powerLevel, sampleAverage, ledMode, sampleRate);
                break;
            case '4':
                showTemp = !showTemp;
                break;
            case '5':
                showHz = !showHz;
                break;
            case '6':
                showBeat = !showBeat;
                break;
            case '7':
                showBpm = !showBpm;
                break;
            case '8':
                showAvgDcEst = !showAvgDcEst;
                break;
            case '9':
                showAcCurr = !showAcCurr;
                break;
            case 'f':
                fifoMode = !fifoMode;
                break;
            case 'p':
                showPlot = !showPlot;
//                if (showPlot) {
//                    endwin();
//                }
//                else {
//                    initscr();
//                    scrollok(stdscr, true);
//                }
                break;
            case 's':
                usleep(5 * 1000 * 1000);
                break;
        }
        samplesTaken = 0;
        startTime = millis();
    }
}

void readSample() {
    sample.red = sensor.getRed();
    if (ledMode > 1) {
        sample.ir = sensor.getIR();
    }
    if (ledMode > 2) {
        sample.green = sensor.getGreen();
    }
    if (showTemp) {
        sample.temp = sensor.readTemperature();
    }
}

void readSampleFifo() {
    sample.red = sensor.getFIFORed();
    if (ledMode > 1) {
        sample.ir = sensor.getFIFOIR();
    }
    if (ledMode > 2) {
        sample.green = sensor.getFIFOGreen();
    }
    if (showTemp) {
        sample.temp = sensor.readTemperature();
    }
}

void checkBeat() {
    bool isBeat = checkForBeat(sample.ir);
    if (isBeat) {
        long delta = millis() - bpm.lastBeat;
        bpm.lastBeat = millis();

        bpm.beatsPerMinute = 60 / (delta / 1000.0);

        if (bpm.beatsPerMinute < 255 && bpm.beatsPerMinute > 20)
        {
            bpm.rates[bpm.rateSpot++] = (byte) bpm.beatsPerMinute; //Store this reading in the array
            bpm.rateSpot %= RATE_SIZE; //Wrap variable

            //Take average of readings
            bpm.beatAvg = 0;
            for (byte x = 0 ; x < RATE_SIZE ; x++)
                bpm.beatAvg += bpm.rates[x];
            bpm.beatAvg /= RATE_SIZE;
        }

        if (showBeat) {
            addstr("--- XXX ---\n");
        }
    }
}

int16_t averageDCWeighted(int32_t *p, uint16_t x, int weight = 100)
{
    *p = (x + *p * (weight - 1)) / weight;
    return *p;
}

void printSample() {
    printw("] R[ %d", sample.red);
    if (ledMode > 1) {
        printw("] IR[ %d", sample.ir);
    }
    if (ledMode > 2) {
        printw("] G[ %d", sample.green);
    }
    if (showTemp) {
        printw("] TempC[ %f", sample.temp);
    }
    if (showHz) {
        printw("] Hz[ %f", (float)samplesTaken / ((millis() - startTime) / 1000.0));
    }

    if (showBpm) {
        printw("] BPM[ %d", bpm.beatAvg);
    }

    int32_t irAvgDcEst = averageDCEstimator(&irOldDCAvgEst, sample.ir);
    int32_t irAvgDcWeighted = averageDCWeighted(&irOldDCAvgWeighted, sample.ir);
    if (showAvgDcEst) {
        printw("] IRAvgDcEst[ %d", irAvgDcEst);
        printw("] IRAvgDcWeighted[ %d", irAvgDcWeighted);
    }
    if (showAcCurr) {
        printw("] IRAcEst[ %d", lowPassFIRFilter(sample.ir - irAvgDcEst));
        printw("] IRAcWeighted[ %d", lowPassFIRFilter(sample.ir - irAvgDcWeighted));
    }
    addstr("]\n");
}

int main(int argc, char** argv) {

    initscr();
    scrollok(stdscr, true);

    // setup ctrl-c handler
    // code from http://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event-c
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = kill_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    addstr("MAX30105 Basic Readings Example \n");

    // Initialize sensor
    if (sensor.begin(wireLinux) == false) {
        addstr("MAX30105 was not found. Please check wiring/power. \n");
        return 1;
    }
    else {
        printw("Part id: %d \n", sensor.readPartID());
        refresh();
        usleep(2 * 1000 * 1000);
    }

    sensor.setup(powerLevel, sampleAverage, ledMode, sampleRate); //Configure sensor. Use 6.4mA for LED drive

    startTime = millis();

    long unsigned int x = 0;
    while (running) {
        if (fifoMode) {
            sensor.check();
            while (sensor.available()) {
                samplesTaken++;
                readSampleFifo();
                checkBeat();
                sensor.nextSample();
            }
        }
        else {
            samplesTaken++;
            readSample();
            checkBeat();
        }
        printSample();
        keyPressed();
    }
//    printSample();
//    refresh();
//    usleep(5 * 1000 * 1000);
    endwin();
}
