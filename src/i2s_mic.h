#ifndef I2S_MIC_H
#define I2S_MIC_H

#include <stdint.h>
#include <stdbool.h>
#include "beat_detection.h"

// Initialize I2S microphone (call once at startup)
void I2sInit();

// Read audio samples from microphone
// Returns true if successfully read FFT_BUFFER_LENGTH samples
bool ReadMicData(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);

#endif // I2S_MIC_H
