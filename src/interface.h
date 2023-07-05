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
    wave_flash_double = 16,
    vertical_bars_clockwise = 17,
    just_wave_up = 18,
    just_wave_down = 19,
    horizontal_bars = 20,
    // Strobe >:D
    strobe = 24,
    // Ambient effects
    wave_anticlockwise = 32,
    wave_clockwise = 33,
    twinkle = 34,
    noEffect = 35,
};

typedef struct radioData_t
{
    bool isEffectCommand;
    int8_t effect;
    int8_t colour;
    uint8_t brightness;
    uint16_t beatLength_ms;
    bool shouldAttemptResync;

    bool operator==(const radioData_t &other) const
    {
        return (
            isEffectCommand == other.isEffectCommand &&
            effect == other.effect &&
            colour == other.colour &&
            brightness == other.brightness &&
            beatLength_ms == other.beatLength_ms &&
            shouldAttemptResync == other.shouldAttemptResync);
    }
} radioData_s;

radioData_t radioData = {
    .isEffectCommand = false,
    .effect = static_cast<int8_t>(Effect::wave_flash_double),
    .colour = static_cast<int8_t>(Colour::blue),
    .brightness = 255,
    .beatLength_ms = 483,
    .shouldAttemptResync = false,
};
