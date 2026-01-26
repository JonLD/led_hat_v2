#ifndef CONFIG_H
#define CONFIG_H

#define NUMBER_X_LEDS 34
#define NUMBER_Y_LEDS 5
#define NUM_LEDS (NUMBER_X_LEDS * NUMBER_Y_LEDS)

// max x and y values of LED matrix
#define MAX_X_INDEX (NUMBER_X_LEDS - 1)
#define MAX_Y_INDEX (NUMBER_Y_LEDS - 1)

#define BRIGHTNESS_RE_PIN_A 19
#define BRIGHTNESS_RE_PIN_B 18

#endif // CONFIG_H