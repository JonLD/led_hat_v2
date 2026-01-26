#ifndef TIMING_H
#define TIMING_H

#include <Arduino.h>

// Get the current number of micros since power on, from the ESP's hardware timer.
// This would wrap after (2^64) / (10^6 * 60 * 60 * 24 * 365) = 584942 years
inline int64_t GetMicros(void)
{
    return esp_timer_get_time();
}

// Get the current number of millis since power on, from the ESP's hardware timer.
inline int64_t GetMillis(void)
{
    return (GetMicros() / 1000u);
}


#endif // TIMING_H
