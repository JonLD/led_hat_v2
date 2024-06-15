#include <stdint.h>

#include "beat_detection.h"

#define QUEUE_LENGTH 10
// To make queue actions non-blocking (either work now or fail, don't wait)
#define NON_BLOCKING (0u)

typedef struct ThreadMsg_s {
    int32_t rawMicSamples[FFT_BUFFER_LENGTH];
} ThreadMsg_t;
