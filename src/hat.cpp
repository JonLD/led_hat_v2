#include <Arduino.h>
#include <esp_now.h>
#include <FastLED.h>
#include <WiFi.h>
#include <Wire.h>

#include "config.h"
#include "effects.h"
#include "interface.h"
#include "timing.h"
#include "profiling.h"

#define BPM 120
#define BEAT_INTERVAL_MS (60000 / BPM)  // 500ms at 120 BPM
#define EFFECT_CHANGE_INTERVAL_MS (4 * 60 * 1000)  // 4 minutes
#define COLOR_CHANGE_INTERVAL_MS (8 * 60 * 1000)   // 8 minutes

#define PLAY_EFFECT_SEQUENCE(effect) PlayEffectSequence(effect, size(effect))

uint8_t com7Address[] = {0x0C, 0xB8, 0x15, 0xF8, 0xF6, 0x80};

Colour currentColour = static_cast<Colour>(radioData.colour);
Effect currentEffect = static_cast<Effect>(radioData.effect);

static void SetEffectColour();
static void PlayEffectSequence(effect_array_t effects_array, size_t array_size);
static void EffectSelectionEngine();
static void AutoRotateEffects();
static void AutoRotateColors();
static void GenerateFixedBeat();

// Fixed BPM timing variables
unsigned long lastBeatTime_ms = 0;  // Global for effects.cpp
bool isBeatDetected = false;  // Global for effects.cpp
static unsigned long lastEffectChangeTime_ms = 0;
static unsigned long lastColorChangeTime_ms = 0;
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
    case Colour::cyan:
        colour1 = colour2 = colour3 = CRGB::Cyan;
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
    static uint i = 0;
    if (isBeatDetected)
    {
        i++;
    }
    if (!(i < array_size))
    {
        i = 0;
    }
    effects_array[i]();
}

// Generate fixed BPM beat timing
static void GenerateFixedBeat()
{
    unsigned long currentTime = GetMillis();
    if (currentTime - lastBeatTime_ms >= BEAT_INTERVAL_MS)
    {
        isBeatDetected = true;
        lastBeatTime_ms = currentTime;
    }
}

// Auto-rotate through effects every 4 minutes
static void AutoRotateEffects()
{
    unsigned long currentTime = GetMillis();
    if (currentTime - lastEffectChangeTime_ms >= EFFECT_CHANGE_INTERVAL_MS)
    {
        // Weighted rotation - vertical waves appear more often
        static uint8_t effectIndex = 0;
        Effect effects[] = {Effect::wave_up, Effect::wave_down, Effect::wave_up, 
                           Effect::wave_down, Effect::wave_clockwise, 
                           Effect::wave_anticlockwise, Effect::twinkle};
        currentEffect = effects[effectIndex];
        effectIndex = (effectIndex + 1) % 7;
        lastEffectChangeTime_ms = currentTime;
    }
}

// Auto-rotate through colors every 8 minutes
static void AutoRotateColors()
{
    unsigned long currentTime = GetMillis();
    if (currentTime - lastColorChangeTime_ms >= COLOR_CHANGE_INTERVAL_MS)
    {
        // Cycle through available colors
        static uint8_t colorIndex = 0;
        Colour colors[] = {Colour::blue, Colour::cyan, Colour::red, 
                          Colour::green, Colour::purple, Colour::fire};
        currentColour = colors[colorIndex];
        SetEffectColour();
        colorIndex = (colorIndex + 1) % 6;
        lastColorChangeTime_ms = currentTime;
    }
}

static void EffectSelectionEngine()
{
    // For jellyfish, simply use auto-rotation
    // Effects and colors change automatically on their own timers
    AutoRotateEffects();
    AutoRotateColors();
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

    // No microphone init needed for jellyfish
    FastLedInit();

    // Initialize with first color and effect
    currentEffect = Effect::wave_up;
    currentColour = Colour::blue;
    SetEffectColour();
    
    // Initialize timers
    lastBeatTime_ms = GetMillis();
    lastEffectChangeTime_ms = GetMillis();
    lastColorChangeTime_ms = GetMillis();
}

void loop()
{
    // Still respond to controller commands if connected
    if (radioData.isEffectCommand)
    {
        radioData.isEffectCommand = false;
        currentEffect = static_cast<Effect>(radioData.effect);
        lastEffectChangeTime_ms = GetMillis();  // Reset auto-rotate timer
    }
    else
    {
        Colour radioDataColour = static_cast<Colour>(radioData.colour);
        if (currentColour != radioDataColour)
        {
            currentColour = radioDataColour;
            SetEffectColour();
            lastColorChangeTime_ms = GetMillis();  // Reset auto-rotate timer
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
    
    // Generate fixed BPM beat instead of microphone detection
    GenerateFixedBeat();
    
    // Handle effect and color auto-rotation
    EffectSelectionEngine();
    
    // Play the selected effect
    PlaySelectedEffect();
    
    EMIT_PROFILING_EVENT;
    
    // Update LEDs
    EVERY_N_MILLIS(15)
    {
        FastLED.show();
    }
    
    EMIT_PROFILING_EVENT;
    
    // Reset beat flag for next cycle
    isBeatDetected = false;
    
#ifdef BPS_PROFILING
    Serial.print("\n");
#endif
}
