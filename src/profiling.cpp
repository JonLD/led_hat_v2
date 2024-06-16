#include "profiling.h"

#include <arduino.h>
#include "beat_detection.h"

#include "driver/i2s.h"

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include <arduinoFFT.h>

int64_t lastProfilingPoint_us;
int64_t microsNow;

// Print various data for debugging
void PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType)
{
    for (uint16_t i = 0; i < bufferSize; i++)
    {
        float abscissa;
        /* Print abscissa value */
        switch (scaleType)
        {
        case SCL_INDEX:
            abscissa = (i * 1.0);
            break;
        case SCL_TIME:
            abscissa = ((i * 1.0) / SAMPLING_FREQUENCY_HZ);
            break;
        case SCL_FREQUENCY:
            abscissa = ((i * 1.0 * SAMPLING_FREQUENCY_HZ) / FFT_BUFFER_LENGTH);
            break;
        }
        Serial.print(abscissa, 6);
        if (scaleType == SCL_FREQUENCY)
            Serial.print("Hz");
        Serial.print(" ");
        Serial.println(vData[i] / 1000, 4);
    }
    Serial.println();
}
