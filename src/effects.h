#ifndef EFFECTS_H
#define EFFECTS_H

#include <FastLED.h>
#include <config.h>
#include "beat_detection.h"

extern CRGB colour1;
extern CRGB colour2;
extern CRGB colour3;

typedef void (*effect_function_ptr_t)();
typedef const effect_function_ptr_t effect_array_t[];

void controlLed(bool);

//-------------- Ambient effect arrays --------------
void noEffect();
void twinkle();
void strobe();
void waveClockwise();
void waveAnticlockwise();

effect_array_t no_effect = {noEffect};
effect_array_t twinkle_ = {twinkle};
effect_array_t wave_clockwise = {waveClockwise};
effect_array_t wave_anticlockwise = {waveAnticlockwise};
effect_array_t strobe_ = {strobe};

//-------------- Beat effect arrays --------------
void verticalBars();
void waveUp();
void waveDown();
void randomCross();
void horizontalRay();
void fastLedInit();

effect_array_t vertical_bars_clockwise = {
    verticalBars,
};
effect_array_t wave_up = {
    waveUp,
};
effect_array_t wave_down = {
    waveDown,
};
effect_array_t wave_up_down = {
    waveDown,
    waveUp,
};
effect_array_t wave_flash_double = {
    waveUp,
    waveUp,
    verticalBars,
    verticalBars,
};
effect_array_t eigh_wave_eight_bars = {
    waveUp,
    waveUp,
    waveUp,
    waveUp,
    waveUp,
    waveUp,
    waveUp,
    waveUp,
    verticalBars,
    verticalBars,
    verticalBars,
    verticalBars,
    verticalBars,
    verticalBars,
    verticalBars,
    verticalBars,
    waveDown,
    waveDown,
    waveDown,
    waveDown,
    waveDown,
    waveDown,
    waveDown,
    waveDown,
};
effect_array_t random_cross = {
    randomCross,
};
effect_array_t horizontal_ray = {
    horizontalRay,
};

#endif // EFFECTS_H