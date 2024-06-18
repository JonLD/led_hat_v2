#ifndef BPM_DETECTION_H
#define BPM_DETECTION_H

#include <stdint.h>

void BpmDetection_Init(int64_t nowMs);
void BpmDetection_Step(int64_t nowMs);

// Number of samples on which to base our BPM detection
#define NUM_BPM_SAMPLES (100u)

// Convert from a bpm to a millisecond interval and back
#define BPM_TO_MS(bpm) (60000u / bpm)
#define MS_TO_BPM(ms)  (60000u / ms)

typedef struct {
    float currentBpmEstimate;
    uint32_t lastInRangeBpmValues[NUM_BPM_SAMPLES];
    int64_t lastBeatTime;
} BpmDetectionState_t;

extern BpmDetectionState_t bpmState;

#endif // BPM_DETECTION_H
