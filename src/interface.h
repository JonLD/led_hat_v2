enum colourOrEffectEnum : long
{
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
    enum_wave_flash_double = 16,
    enum_just_flash = 17,
    enum_just_wave = 18,
    enum_wave_double = 19,
    enum_twinkle = 21,
    enum_strobe_bar = 20,
    enum_horizontalBars = 22,
};

typedef struct radioData_t
{
    int8_t effect;
    int8_t colour;
    uint8_t brightness;
    uint16_t beatLength_ms;
    bool shouldAttemptResync;

    bool operator==(const radioData_t &other) const
    {
        return (
            effect == other.effect &&
            colour == other.colour &&
            brightness == other.brightness &&
            beatLength_ms == other.beatLength_ms &&
            shouldAttemptResync == other.shouldAttemptResync);
    }
} radioData_s;

radioData_t radioData = {
    .effect = enum_wave_flash_double,
    .colour = blue,
    .brightness = 255,
    .beatLength_ms = 483,
    .shouldAttemptResync = false,
};


