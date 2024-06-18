/* Attempt to detect the BPM given that a beat is happening now.

Tries to be robust to intermediate "groove" beats.
*/
#include <stdint.h>
#include <Arduino.h>

#include "bpm_detection.h"

typedef float float32_t;


// Minimum and maximum BPM we'll allow as a "valid" reading
#define MIN_BPM_DETECTABLE (110u)
#define MAX_BPM_DETECTABLE (135u)

// Maximum and minimum (note reciprocal relationship) BPMs we'll allow
#define MAX_ALLOWED_INTERVAL BPM_TO_MS(MIN_BPM_DETECTABLE)
#define MIN_ALLOWED_INTERVAL BPM_TO_MS(MAX_BPM_DETECTABLE)

// Stores data related to our BPM estimate

BpmDetectionState_t bpmState = {
    .currentBpmEstimate = 124,
    .lastInRangeBpmValues = { 0 },
    .lastBeatTime = 0
};

// A wrapping increment; adds one to val but wraps to 0 if it tries to exceed lim
#define WRAPPING_INC(val, lim) val = ((val + 1u) % lim)
#define BETWEEN(low, val, high) ((low <= val) && (val <= high))


void BpmDetection_Init(int64_t nowMs)
{
    bpmState.lastBeatTime = nowMs;
}


// Update our BPM detection state, given the current time since boot in milliseconds
void BpmDetection_Step(int64_t nowMs)
{
    const uint32_t beatIntervalMs = (uint32_t)(nowMs - bpmState.lastBeatTime);
    bpmState.lastBeatTime = nowMs;

    // If this interval was acceptable, update our array
    if (BETWEEN(MIN_ALLOWED_INTERVAL, beatIntervalMs, MAX_ALLOWED_INTERVAL))
    {
        static uint32_t idx = 0u;
        bpmState.lastInRangeBpmValues[idx] = MS_TO_BPM(beatIntervalMs);
        WRAPPING_INC(idx, NUM_BPM_SAMPLES);
    }

    // Find the mean and use that as our estimate
    uint32_t totalBeats = 0u;
    uint32_t beatAcc = 0u;
    for (uint32_t idx = 0u; idx < NUM_BPM_SAMPLES; idx++)
    {
        if (bpmState.lastInRangeBpmValues[idx] != 0u)
        {
            totalBeats++;
            beatAcc += bpmState.lastInRangeBpmValues[idx];
        }
    }
    if (totalBeats > 0)
    {
        bpmState.currentBpmEstimate = (float)beatAcc / totalBeats;
    }
}
