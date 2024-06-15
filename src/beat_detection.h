#ifndef BEAT_DETECTION_H
#define BEAT_DETECTION_H

#include "driver/i2s.h"

#define SAMPLING_FREQUENCY_HZ 48000
#define FFT_BUFFER_LENGTH 1024

extern unsigned long lastBeatTime_ms;
extern bool isBeatDetected;

void computeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);
void detectBeat();

#endif // BEAT_DETECTION_H
