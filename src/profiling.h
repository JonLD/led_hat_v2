#ifndef PROFILING_H
#define PROFILING_H

#include <arduino.h>

#include "timing.h"

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

// Uncomment to enable print debugging (only enable one at a time)
// #define OUTPUT_AUDIO
// #define TIME_PROFILING
// #define PRINT_BIN_MAGNITUDES
// #define PRINT_NOT_BEAT_DETECTED_REASON
// #define PRINT_CURRENT_BASS_MAG
// #define PROFILE_MIC_READ
// #define BEAT_DETECTION_PROFILING


extern int64_t lastProfilingPoint_ms;
extern int64_t microsNow;

#ifdef TIME_PROFILING
#define BPS_PROFILING
#define EMIT_PROFILING_EVENT {\
    microsNow = GetMicros();\
    Serial.print(microsNow - lastProfilingPoint_ms);\
    Serial.print("\t");\
    lastProfilingPoint_ms = microsNow;\
}
#else
#define EMIT_PROFILING_EVENT do { } while(0)
#endif // TIME_PROFILING

#ifdef PROFILE_MIC_READ
#define EMIT_MIC_READ_EVENT {\
    microsNow = GetMicros();\
    Serial.println(microsNow - lastProfilingPoint_ms);\
    lastProfilingPoint_ms = microsNow;\
}
#else
#define EMIT_MIC_READ_EVENT do { } while(0)
#endif // PROFILE_MIC_READ

#ifdef BEAT_DETECTION_PROFILING
#define BPS_PROFILING
#define EMIT_DETECTION_EVENT (DETECTION_CONDITION) {\
    Serial.print(DETECTION_CONDITION);\
    Serial.print("\t");\
}
#else
#define EMIT_DETECTION_EVENT () do { } while(0)
#endif // TIME_PROFILING

void PrintVector(float *, uint16_t, uint8_t);


#endif // PROFILING_H
