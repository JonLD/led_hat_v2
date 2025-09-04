#ifndef CONFIG_H
#define CONFIG_H

// Jellyfish configuration: 10 tentacles (each has 2 strips: down and up)
#define NUMBER_X_LEDS 20   // 2 strips per tentacle × 10 tentacles
#define NUMBER_Y_LEDS 15   // Height of each strip
#define NUM_LEDS (NUMBER_X_LEDS * NUMBER_Y_LEDS)

// max x and y values of LED matrix
#define MAX_X_INDEX (NUMBER_X_LEDS - 1)
#define MAX_Y_INDEX (NUMBER_Y_LEDS - 1)

#define BRIGHTNESS_RE_PIN_A 19
#define BRIGHTNESS_RE_PIN_B 18

// Matrix layout configuration
#define VERTICAL_ZIGZAG  // Define this for jellyfish (vertical zigzag), comment out for hat (horizontal zigzag)

#endif // CONFIG_H