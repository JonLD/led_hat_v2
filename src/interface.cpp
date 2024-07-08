#include "interface.h"

#include <arduino.h>

radioData_t radioData = {
    .isEffectCommand = false,
    .effect = static_cast<int8_t>(Effect::wave_up_down),
    .colour = static_cast<int8_t>(Colour::blue),
    .brightness = 30,
    .beatLength_ms = 483,
    .ambientOverride = false,
};