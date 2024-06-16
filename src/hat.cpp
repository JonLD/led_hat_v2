#include <Arduino.h>
#include <esp_now.h>
#include <FastLED.h>
#include <WiFi.h>
#include <Wire.h>

#include "beat_detection.h"
#include "config.h"
#include "effects.h"
#include "i2s_mic.h"
#include "interface.h"
#include "timing.h"
#include "profiling.h"

#define AMBIENT_EFFECT_TIMEOUT_MS 1000
#define BEAT_EFFECT_TIMEOUT_MS 520

#define PLAY_EFFECT_SEQUENCE(effect) PlayEffectSequence(effect, size(effect))

uint8_t com7Address[] = {0x0C, 0xB8, 0x15, 0xF8, 0xF6, 0x80};

Colour currentColour = static_cast<Colour>(radioData.colour);
Effect currentEffect = static_cast<Effect>(radioData.effect);

static void SetEffectColour();
static void PlayEffectSequence(effect_array_t effects_array, size_t array_size);
static void EffectSelectionEngine();
static void PlaySelectedEffect();
static void PopulateRadioData(const uint8_t *esp_now_info, const uint8_t *incomingData, int data_len);

// for getting the length of the above effect function pointer arrays
template <class T, size_t N>
constexpr size_t size(T (&)[N])
{
    return N;
}

// callback function that will be executed when data is received
static void PopulateRadioData(const uint8_t *esp_now_info, const uint8_t *incomingData, int data_len)
{
    memcpy(&radioData, incomingData, sizeof(radioData_t));
    Serial.print("Bytes received: ");
    Serial.println(data_len);
    Serial.print("Effect enum: ");
    Serial.println(radioData.effect);
    Serial.print("Colour enum: ");
    Serial.println(radioData.colour);
    Serial.print("Ambient override: ");
    Serial.println(radioData.ambientOverride);
    Serial.println();
}

//-------------- Effect Control --------------

// logic for selection of different colour pallettes
static void SetEffectColour()
{
    switch (currentColour)
    {
    case Colour::red:
        colour1 = colour2 = colour3 = CRGB::Red;
        break;
    case Colour::blue:
        colour1 = colour2 = colour3 = CRGB::Blue;
        break;
    case Colour::green:
        colour1 = colour2 = colour3 = CRGB::Green;
        break;
    case Colour::purple:
        colour1 = colour2 = colour3 = CRGB::Purple;
        break;
    case Colour::white:
        colour1 = colour2 = colour3 = CRGB::White;
        break;
    case Colour::yellow:
        colour1 = colour2 = colour3 = CRGB::Yellow;
        break;
    case Colour::orange:
        colour1 = colour2 = colour3 = CRGB::OrangeRed;
        break;
    case Colour::red_white:
        colour1 = colour2 = CRGB::Red;
        colour3 = CRGB::White;
        break;
    case Colour::green_white:
        colour1 = colour2 = CRGB::Green;
        colour3 = CRGB::White;
        break;
    case Colour::blue_white:
        colour1 = colour2 = CRGB::Blue;
        colour3 = CRGB::White;
        break;
    case Colour::cb:
        colour1 = CRGB::OrangeRed;
        colour2 = CRGB::Green;
        colour3 = CRGB::Purple;
        break;
    case Colour::cd:
        colour1 = CRGB::Yellow;
        colour2 = CRGB::Blue;
        colour3 = CRGB::Purple;
        break;
    case Colour::fire:
        colour1 = CRGB::Yellow;
        colour2 = CRGB::Red;
        colour3 = CRGB::OrangeRed;
        break;
    case Colour::purue:
        colour1 = CRGB::Purple;
        colour2 = CRGB::Red;
        colour3 = CRGB::Blue;
        break;
    case Colour::blue_red:
        colour1 = colour2 = CRGB::Blue;
        colour3 = CRGB::Red;
        break;
    }
}

static void PlayEffectSequence(effect_array_t effects_array, size_t array_size)
{
    static int i = 0;
    if (isBeatDetected && ++i >= array_size)
    {
        i = 0;
    }
    effects_array[i]();
}

static void EffectSelectionEngine()
{
    static bool isAmbientSection = false;
    if (isBeatDetected && isAmbientSection)
    {
        isAmbientSection = false;
        currentEffect = beatEffectEnumValues[random(size(beatEffectEnumValues))];
    }
    else if (GetMillis() - lastBeatTime_ms > AMBIENT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    {
        isAmbientSection = true;
        currentEffect = ambientEffectEnumValues[random(size(ambientEffectEnumValues))];
    }
    // TODO: seems to be a bit broken with this
    // else if (GetMillis() - lastBeatTime_ms > BEAT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    // {
    //     currentEffect = static_cast<Effect>(beatEffectEnumValues[random(size(beatEffectEnumValues))]);
    // }
}

// logic for selection of next pre-set effect
static void PlaySelectedEffect()
{
    switch (currentEffect)
    {
    case Effect::wave_flash_double:
        PLAY_EFFECT_SEQUENCE(wave_flash_double);
        return;
    case Effect::vertical_bars_clockwise:
        PLAY_EFFECT_SEQUENCE(vertical_bars_clockwise);
        return;
    case Effect::wave_up:
        PLAY_EFFECT_SEQUENCE(wave_up);
        return;
    case Effect::wave_down:
        PLAY_EFFECT_SEQUENCE(wave_down);
        return;
    case Effect::wave_up_down:
        PLAY_EFFECT_SEQUENCE(wave_up_down);
        return;
    case Effect::random_cross:
        PLAY_EFFECT_SEQUENCE(random_cross);
        return;
    case Effect::horizontal_ray:
        PLAY_EFFECT_SEQUENCE(horizontal_ray);
        return;
    case Effect::strobe:
        PLAY_EFFECT_SEQUENCE(strobe_);
        return;
    case Effect::wave_anticlockwise:
        PLAY_EFFECT_SEQUENCE(wave_anticlockwise);
        return;
    case Effect::wave_clockwise:
        PLAY_EFFECT_SEQUENCE(wave_clockwise);
        return;
    case Effect::twinkle:
        PLAY_EFFECT_SEQUENCE(twinkle_);
        return;
    case Effect::no_effect:
        PLAY_EFFECT_SEQUENCE(no_effect);
        return;
    }
    Serial.println("Effect not found!");
}

void setup()
{
    Serial.begin(BAUD_RATE);

    // Wifi Setup
    WiFi.mode(WIFI_STA); // Set device as a Wi-Fi Station
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(PopulateRadioData);

    I2sInit();
    FastLedInit();

    SetEffectColour();
}

void loop()
{
    if (radioData.isEffectCommand)
    {
        radioData.isEffectCommand = false;
        currentEffect = static_cast<Effect>(radioData.effect);
    }
    else
    {
        Colour radioDataColour = static_cast<Colour>(radioData.colour);
        if (currentColour != radioDataColour)
        {
            currentColour = radioDataColour;
            SetEffectColour();
        }
    }
    static uint8_t lastBrightness = radioData.brightness;
    if (radioData.brightness != lastBrightness)
    {
        FastLED.setBrightness(radioData.brightness);
        lastBrightness = radioData.brightness;
        Serial.println("setting new brightness");
    }
    EMIT_PROFILING_EVENT;
    int32_t rawMicSamples[FFT_BUFFER_LENGTH];
    if (ReadMicData(rawMicSamples))
    {
        ComputeFFT(rawMicSamples);
        EMIT_PROFILING_EVENT;
        DetectBeat();
        EMIT_PROFILING_EVENT;
    }
    if (radioData.ambientOverride)
    {
        isBeatDetected = false;
    }
    EffectSelectionEngine();
    PlaySelectedEffect();
    EMIT_PROFILING_EVENT;
    EVERY_N_MILLIS(15)
    {
        FastLED.show();
    }
    EMIT_PROFILING_EVENT;
    isBeatDetected = false;
#ifdef BPS_PROFILING
    Serial.print("\n");
#endif
}
