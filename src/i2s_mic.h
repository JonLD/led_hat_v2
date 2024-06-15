#ifndef I2S_MIC_H
#define I2S_MIC_H

#include "driver/i2s.h"
#include "beat_detection.h"



void InitialiseI2sThread();
void I2sMain(void *);

#endif // I2S_MIC_H