#ifndef I2S_MIC_H
#define I2S_MIC_H

#include "driver/i2s.h"
#include "beat_detection.h"


bool ReadMicData(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);
void I2sInit();

#endif // I2S_MIC_H