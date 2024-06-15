#ifndef I2S_MIC_H
#define I2S_MIC_H

#include "driver/i2s.h"
#include "beat_detection.h"


bool readMicData(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);
void i2sInit();

#endif // I2S_MIC_H