#ifndef PROFILING_H
#define PROFILING_H

#include <stdint.h>
#include "timing.h"
#include "esp_log.h"

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
    ESP_LOGI("profiling", "%lld\t", (long long)(microsNow - lastProfilingPoint_ms));\
    lastProfilingPoint_ms = microsNow;\
}
#else
#define EMIT_PROFILING_EVENT do { } while(0)
#endif // TIME_PROFILING

#ifdef PROFILE_MIC_READ
#define EMIT_MIC_READ_EVENT {\
    microsNow = GetMicros();\
    ESP_LOGI("profiling", "Mic read: %lld us", (long long)(microsNow - lastProfilingPoint_ms));\
    lastProfilingPoint_ms = microsNow;\
}
#else
#define EMIT_MIC_READ_EVENT do { } while(0)
#endif // PROFILE_MIC_READ

#ifdef BEAT_DETECTION_PROFILING
#define BPS_PROFILING
#define EMIT_DETECTION_EVENT (DETECTION_CONDITION) {\
    ESP_LOGI("profiling", "%d\t", DETECTION_CONDITION);\
}
#else
#define EMIT_DETECTION_EVENT () do { } while(0)
#endif // BEAT_DETECTION_PROFILING

#endif // PROFILING_H
