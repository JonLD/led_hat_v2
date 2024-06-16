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

void ControlLed(bool);

//-------------- Ambient effect arrays --------------
void NoEffect();
void Twinkle();
void Strobe();
void WaveClockwise();
void WaveAnticlockwise();

effect_array_t no_effect = {NoEffect};
effect_array_t twinkle = {Twinkle};
effect_array_t wave_clockwise = {WaveClockwise};
effect_array_t wave_anticlockwise = {WaveAnticlockwise};
effect_array_t strobe = {Strobe};

//-------------- Beat effect arrays --------------
void VerticalBars();
void WaveUp();
void WaveDown();
void RandomCross();
void HorizontalRay();
void FastLedInit();
void HorizontalBars();

effect_array_t vertical_bars_clockwise = {
    VerticalBars,
};
effect_array_t wave_up = {
    WaveUp,
};
effect_array_t wave_down = {
    WaveDown,
};
effect_array_t wave_up_down = {
    WaveDown,
    WaveUp,
};
effect_array_t wave_flash_double = {
    WaveUp,
    WaveUp,
    VerticalBars,
    VerticalBars,
};
effect_array_t eigh_wave_eight_bars = {
    WaveUp,
    WaveUp,
    WaveUp,
    WaveUp,
    WaveUp,
    WaveUp,
    WaveUp,
    WaveUp,
    VerticalBars,
    VerticalBars,
    VerticalBars,
    VerticalBars,
    VerticalBars,
    VerticalBars,
    VerticalBars,
    VerticalBars,
    WaveDown,
    WaveDown,
    WaveDown,
    WaveDown,
    WaveDown,
    WaveDown,
    WaveDown,
    WaveDown,
};
effect_array_t random_cross = {
    RandomCross,
};
effect_array_t horizontal_ray = {
    HorizontalRay,
};

#endif // EFFECTS_H
