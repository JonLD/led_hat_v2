#ifndef BEAT_DETECTION_H
#define BEAT_DETECTION_H

#include <stdint.h>

#define SAMPLING_FREQUENCY_HZ 48000
#define FFT_BUFFER_LENGTH 1024

extern uint32_t lastBeatTime_ms;
extern bool isBeatDetected;

void InitBeatDetection();
void ComputeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);
void DetectBeat();

#endif // BEAT_DETECTION_H
