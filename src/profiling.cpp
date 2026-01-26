#include "profiling.h"
#include "esp_log.h"
#include <stdint.h>

static const char *TAG = "profiling";

// Global timing variables
int64_t lastProfilingPoint_ms = 0;
int64_t microsNow = 0;

// Print various data for debugging (stub for now - implement when needed for FFT)
void PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType)
{
    // TODO: Implement when we need FFT debugging
    // Will need SAMPLING_FREQUENCY_HZ and FFT_BUFFER_LENGTH defined
    ESP_LOGI(TAG, "PrintVector called (not implemented yet)");
    (void)vData;
    (void)bufferSize;
    (void)scaleType;
}
