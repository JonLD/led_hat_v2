#ifndef EFFECTS_H
#define EFFECTS_H

#include <FastLED.h>
#include <config.h>
#include <beat_detection.h>

#define RANDOM_X random(MAX_X_INDEX + 1)
#define RANDOM_Y random(MAX_Y_INDEX + 1)

// default all colours to blue
CRGB colour1 = CRGB::Blue;
CRGB colour2 = CRGB::Blue;
CRGB colour3 = CRGB::Blue;

CRGB leds[NUM_LEDS] = {0};

typedef void (*effect_function_ptr_t)();
typedef const effect_function_ptr_t effect_array_t[];

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
void verticalBarsClockwise();
void verticalBarsAnticlockwise();
void waveUp();
void waveDown();
void randomCross();
void horizontalRay();

effect_array_t vertical_bars = {
    verticalBarsClockwise,
    verticalBarsClockwise,
    verticalBarsAnticlockwise,
    verticalBarsAnticlockwise,
};
effect_array_t vertical_bars_clockwise = {
    verticalBarsClockwise,
};
effect_array_t vertical_bars_anticlockwise = {
    verticalBarsAnticlockwise,
};
effect_array_t wave_up = {
    waveUp,
};
effect_array_t wave_up_down = {
    waveUp,
    waveDown,
};
effect_array_t wave_bars = {
    waveUp,
    waveUp,
    verticalBarsClockwise,
    verticalBarsClockwise,
};
effect_array_t random_cross = {
    randomCross,
};
effect_array_t horizontal_ray = {
    horizontalRay,
};
// effect_array_t ray_cross = {
//     horizontalRay,
//     horizontalRay,
//     randomCross,
//     randomCross,
// };

// ----- Effect utils -----

// Map any x, y coordinate on LED matrix to LED array index
int mapXYtoIndex(int x, int y)
{
    x %= NUMBER_X_LEDS;
    if (x < 0)
    {
        x += NUMBER_X_LEDS;
    }
    int i;
    if (y % 2 == 0)
    {
        i = x + (MAX_X_INDEX + 1) * y; // i steps up by MAX_X_INDEX+1 each row
    }
    // LED strips setup in alternating direction so x value flips sign
    else
    {
        i = (MAX_X_INDEX + 1) * (y + 1) - (x + 1);
    }

    return i;
}

void fadeLeds(int fadeBy)
{
    EVERY_N_MILLIS(15)
    {
        fadeToBlackBy(leds, NUM_LEDS, fadeBy);
    }
}

void generateDistributedRandomNumbers(int *outBuffer, int count, int min, int max)
{
    int zoneSize = (max - min + 1) / count;

    for (int i = 0; i < count - 1; i++)
    {
        outBuffer[i] = random(min + zoneSize * i, min + zoneSize * (i + 1));
    }
    outBuffer[count - 1] = random(min + zoneSize * (count - 1), max + 1);
}

CRGB getRandomColourChoice()
{
    switch (random(3))
    {
    case 0:
        return colour1;
    case 1:
        return colour2;
    case 2:
        return colour3;
    default:
        return colour1;
    }
}

// ----- Effect functions -----

void noEffect()
{
    fadeLeds(100);
}

// 1
void waveUp()
{
    static int16_t y = -1;
    if (isBeatDetected)
    {
        y = MAX_Y_INDEX;
    }
    if (y >= 0)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int x = 0; x <= MAX_X_INDEX; ++x)
            { // Fill x row
                // sequence of logic to create pattern
                if ((y % 2 == 0) && (x % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((y % 2 == 0) && (x % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (x % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            y -= 1;
        }
    }
    fadeLeds(100);
}

void waveClockwise()
{
    static int16_t x = 0;
    if (x <= MAX_X_INDEX)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            { // Fill x row
                // sequence of logic to create pattern
                if ((x % 2 == 0) && (y % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((x % 2 == 0) && (y % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (y % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            ++x;
        }
    }
    else
    {
        x = 0;
    }
    fadeLeds(40);
}

void waveAnticlockwise()
{
    static int16_t x = 0;
    if (x >= 0)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            { // Fill x row
                // sequence of logic to create pattern
                if ((x % 2 == 0) && (y % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((x % 2 == 0) && (y % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (y % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            --x;
        }
    }
    else
    {
        x = MAX_X_INDEX;
    }
    fadeLeds(40);
}

void waveDown()
{
    static int16_t y = -1;
    if (isBeatDetected)
    {
        y = 0;
    }
    if (y <= MAX_Y_INDEX)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int x = 0; x <= MAX_X_INDEX; ++x)
            { // Fill x row
                // sequence of logic to create pattern
                if ((y % 2 == 0) && (x % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((y % 2 == 0) && (x % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (x % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            ++y;
        }
    }
    fadeLeds(100);
}

// 2
// Flash LED and decay to 0
void verticalBarsClockwise()
{
    if (isBeatDetected)
    {
        static int xStart = 0;
        for (int x = xStart; x <= MAX_X_INDEX; x += 3)
        { // fill x
            for (int y = 0; y <= MAX_Y_INDEX; y++)
            { // fill y
                leds[mapXYtoIndex(x, y)] = colour1;
            }
        }
        FastLED.show();
        xStart = (xStart == 3) ? (xStart - 2) : ++xStart;
    }
    fadeLeds(100);
}

void verticalBarsAnticlockwise()
{
    if (isBeatDetected)
    {
        static int xStart = 0;
        for (int x = xStart; x <= MAX_X_INDEX; x += 3)
        { // fill x
            for (int y = 0; y <= MAX_Y_INDEX; y++)
            { // fill y
                leds[mapXYtoIndex(-x, y)] = colour1;
            }
        }
        FastLED.show();
        xStart = (xStart == 3) ? (xStart - 2) : ++xStart;
    }
    fadeLeds(100);
}

void horizontalBars()
{
    if (isBeatDetected)
    {
        static int yStart = 0;
        for (int y = yStart; y <= MAX_X_INDEX; y += 3)
        { // fill x
            for (int x = 0; x <= MAX_Y_INDEX; x++)
            { // fill y
                leds[mapXYtoIndex(x, y)] = colour1;
            }
        }

        FastLED.show();
        yStart = yStart ? yStart : ++yStart;
    }
    fadeLeds(100);
}

void randomCross()
{
    if (isBeatDetected)
    {
        int randXs[3] = {0};
        generateDistributedRandomNumbers(randXs, 3, 0, MAX_X_INDEX);

        int randY = RANDOM_Y;

        for (int x = 0; x <= MAX_X_INDEX; x++)
        {
            leds[mapXYtoIndex(x, randY)] = colour1;
        }

        for (int y = 0; y <= MAX_Y_INDEX; y++)
        {
            for (int i = 0; i < 3; i++)
            {
                leds[mapXYtoIndex(randXs[i], y)] = colour1;
            }
        }
    }
    fadeLeds(100);
}

void horizontalRay()
{
    static bool active = false;
    static int counter = 0;
    static int x1 = 0;
    static int x2 = MAX_X_INDEX / 2;
    static int y = -1;

    if (isBeatDetected)
    {
        active = true;
        counter = 0;

        y = RANDOM_Y;
    }

    if (active)
    {
        EVERY_N_MILLISECONDS(5)
        {
            leds[mapXYtoIndex(x1 + counter, y)] = colour1;
            leds[mapXYtoIndex(x1 - counter, y)] = colour1;

            leds[mapXYtoIndex(x2 + counter, y)] = colour2;
            leds[mapXYtoIndex(x2 - counter, y)] = colour2;

            if (++counter > NUMBER_X_LEDS / 4 + (NUMBER_X_LEDS % 4 != 0))
            {
                active = false;
                counter = 0;
            }
        }
    }

    fadeLeds(100);
}

// add new bright spots every quarter note
void twinkle()
{
    fadeLeds(20);
    EVERY_N_MILLIS(20)
    {
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
    }
}

void strobe()
{
    FastLED.clear();
    EVERY_N_MILLISECONDS(40)
    {
        fill_solid(leds, NUM_LEDS, CRGB::White);
    }

    EVERY_N_MILLISECONDS(80)
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
}

void controlLed(bool isBeatDetected)
{
    if (isBeatDetected)
    {
        for (int x = 0; x <= MAX_X_INDEX; ++x)
        { // fill x
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            { // fill y
                leds[mapXYtoIndex(x, y)] = CRGB::Blue;
            }
        }
    }
    else
    {
        fadeLeds(400);
    }
}

#endif