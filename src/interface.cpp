#include "interface.h"

#include <arduino.h>

radioData_t radioData = {
    .isEffectCommand = false,
    .effect = static_cast<int8_t>(Effect::wave_flash_double),
    .colour = static_cast<int8_t>(Colour::blue),
    .brightness = 135,
    .beatLength_ms = 483,
    .ambientOverride = false,
};