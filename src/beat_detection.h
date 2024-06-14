#ifndef BEAT_DETECTION_H
#define BEAT_DETECTION_H

#include "driver/i2s.h"

#define SAMPLING_FREQUENCY_HZ 48000
#define FFT_BUFFER_LENGTH 1024

extern const uint32_t samplingFrequency;
extern unsigned long lastBeatTime_ms;
extern bool isBeatDetected;

extern i2s_config_t i2s_config;
extern i2s_pin_config_t i2s_mic_pins;
void computeFFT();
void detectBeat();
bool readMicData();


#endif
