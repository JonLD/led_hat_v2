#ifndef INTERFACE_H
#define INTERFACE_H

#define MAX_COLOUR_KEYPAD_INDEX 15
#define MAX_EFFECT_KEYPAD_INDEX 30
#define AMBIENT_OVERRIDE_KEYPAD_INDEX 31

enum class Colour : int8_t
{
    // Colours
    white = 0,
    red = 7,
    green = 11,
    blue = 3,
    red_white = 6,
    green_white = 10,
    blue_white = 2,
    purple = 1,
    cb = 12,
    yellow = 8,
    orange = 5,
    cd = 13,
    fire = 4,
    purue = 14,
    blue_red = 15,
};

enum class Effect : int8_t
{
    // Beat effects
    wave_bars = 16,
    vertical_bars = 17,
    vertical_bars_clockwise = 18,
    vertical_bars_anticlockwise = 19,
    wave_up = 20,
    wave_up_down = 21,
    random_cross = 22,
    horizontal_ray = 23,
    // ray_cross = 24,
    // Strobe >:D
    // strobe = 25,
    // Ambient effects
    wave_anticlockwise = 27,
    wave_clockwise = 28,
    twinkle = 29,
    no_effect = 30,
};

const Effect beatEffectEnumValues[] = {
    Effect::wave_bars,
    Effect::vertical_bars,
    Effect::vertical_bars_clockwise,
    Effect::vertical_bars_anticlockwise,
    Effect::wave_up,
    Effect::wave_up_down,
    Effect::random_cross,
    Effect::random_cross,
    Effect::horizontal_ray,
    Effect::horizontal_ray,
    // Effect::ray_cross,
};
const Effect ambientEffectEnumValues[] = {
    Effect::wave_anticlockwise,
    Effect::wave_clockwise,
    Effect::twinkle,
};

typedef struct radioData_t
{
    bool isEffectCommand;
    int8_t effect;
    int8_t colour;
    uint8_t brightness;
    uint16_t beatLength_ms;
    bool ambientOverride;

    bool operator==(const radioData_t &other) const
    {
        return (
            isEffectCommand == other.isEffectCommand &&
            effect == other.effect &&
            colour == other.colour &&
            brightness == other.brightness &&
            beatLength_ms == other.beatLength_ms &&
            ambientOverride == other.ambientOverride);
    }
} radioData_s;

radioData_t radioData = {
    .isEffectCommand = false,
    .effect = static_cast<int8_t>(Effect::wave_bars),
    .colour = static_cast<int8_t>(Colour::blue),
    .brightness = 135,
    .beatLength_ms = 483,
    .ambientOverride = false,
};

#endif