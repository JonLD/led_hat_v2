#ifndef BEAT_DETECTION_H
#define BEAT_DETECTION_H

#include <driver/i2s.h>


extern const uint32_t samplingFrequency;
extern unsigned long lastBeatTime_ms;
extern bool isBeatDetected;

extern i2s_config_t i2s_config;
extern i2s_pin_config_t i2s_mic_pins;

// --------- Pudblic function declarations -----------
void computeFFT();
void detectBeat();
void readMicData();

#endif
