#ifndef PROFILING_H
#define PROFILING_H

// Uncomment to enable print debugging (only enable one at a time)
#define OUTPUT_AUDIO
// #define TIME_PROFILING
// #define PRINT_BIN_MAGNITUDES
// #define PRINT_NOT_BEAT_DETECTED_REASON
// #define PRINT_CURRENT_BASS_MAG

#ifdef OUTPUT_AUDIO
// #define BPS_PROFILING
#endif // OUTPUT_AUDIO

extern unsigned long lastProfilingPoint_ms;
extern unsigned long microsNow;

#ifdef TIME_PROFILING
#define BPS_PROFILING
#define EMIT_PROFILING_EVENT {\
    microsNow = micros();\
    Serial.print(microsNow - lastProfilingPoint_ms);\
    Serial.print("\t");\
    lastProfilingPoint_ms = microsNow;\
}
#else
#define EMIT_PROFILING_EVENT do { } while(0)
#endif // PROFILING

#endif // PROFILING_H