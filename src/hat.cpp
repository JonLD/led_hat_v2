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

#define CASE_SET_COLOUR(colour, col1, col2, col3) \
    case colour:                                  \
        colour1 = col1;                           \
        colour2 = col2;                           \
        colour3 = col3;                           \
        break;                                    \

// logic for selection of different colour pallettes
static void SetEffectColour()
{
    switch (currentColour)
    {
    CASE_SET_COLOUR(Colour::red, CRGB::Red, CRGB::Red, CRGB::Red);
    CASE_SET_COLOUR(Colour::blue, CRGB::Blue, CRGB::Blue, CRGB::Blue);
    CASE_SET_COLOUR(Colour::green, CRGB::Green, CRGB::Green, CRGB::Green);
    CASE_SET_COLOUR(Colour::purple, CRGB::Purple, CRGB::Purple, CRGB::Purple);
    CASE_SET_COLOUR(Colour::white, CRGB::White, CRGB::White, CRGB::White);
    CASE_SET_COLOUR(Colour::yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow);
    CASE_SET_COLOUR(Colour::orange, CRGB::OrangeRed, CRGB::OrangeRed, CRGB::OrangeRed);
    CASE_SET_COLOUR(Colour::red_white, CRGB::Red, CRGB::Red, CRGB::White);
    CASE_SET_COLOUR(Colour::green_white, CRGB::Green, CRGB::Green, CRGB::White);
    CASE_SET_COLOUR(Colour::blue_white, CRGB::Blue, CRGB::Blue, CRGB::White);
    CASE_SET_COLOUR(Colour::cb, CRGB::OrangeRed, CRGB::Green, CRGB::Purple);
    CASE_SET_COLOUR(Colour::cd, CRGB::Yellow, CRGB::Blue, CRGB::Purple);
    CASE_SET_COLOUR(Colour::fire, CRGB::Yellow, CRGB::Red, CRGB::OrangeRed);
    CASE_SET_COLOUR(Colour::purue, CRGB::Purple, CRGB::Red, CRGB::Blue);
    CASE_SET_COLOUR(Colour::blue_red, CRGB::Blue, CRGB::Blue, CRGB::Red);
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

#define CASE_PLAY_EFFECT(name)      \
    case Effect::name:              \
        PLAY_EFFECT_SEQUENCE(name); \
        return;                     \

// logic for selection of next pre-set effect
static void PlaySelectedEffect()
{
    switch (currentEffect)
    {
    CASE_PLAY_EFFECT(wave_flash_double);
    CASE_PLAY_EFFECT(vertical_bars_clockwise);
    CASE_PLAY_EFFECT(wave_up);
    CASE_PLAY_EFFECT(wave_down);
    CASE_PLAY_EFFECT(wave_up_down);
    CASE_PLAY_EFFECT(random_cross);
    CASE_PLAY_EFFECT(horizontal_ray);
    CASE_PLAY_EFFECT(strobe);
    CASE_PLAY_EFFECT(wave_anticlockwise);
    CASE_PLAY_EFFECT(wave_clockwise);
    CASE_PLAY_EFFECT(twinkle);
    CASE_PLAY_EFFECT(no_effect);
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
        EMIT_MIC_READ_EVENT;
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
