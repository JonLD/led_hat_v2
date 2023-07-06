#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <interface.h>
#include <FastLED.h>
#include <config.h>
#include <beat_detection.h>
#include <effects.h>

uint8_t com7Address[] = {0x0C, 0xB8, 0x15, 0xF8, 0xF6, 0x80};

// Initialise varialbes needed for FastLED
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LED_DATA_PIN 22

#define PLAY_EFFECT_SEQUENCE(effect) play_effect_sequence(effect, size(effect))

Colour currentColour = static_cast<Colour>(radioData.colour);
Effect currentEffect = static_cast<Effect>(radioData.effect);

// for getting the length of the above effect function pointer arrays
template <class T, size_t N>
constexpr size_t size(T (&)[N])
{
    return N;
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&radioData, incomingData, sizeof(radioData_t));
    Serial.print("Bytes received: ");
    Serial.println(len);
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
void setEffectColour()
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

void play_effect_sequence(effect_array_t effects_array, size_t array_size)
{
    static int i = 0;
    if (isBeatDetected && ++i >= array_size)
    {
        i = 0;
    }
    effects_array[i]();
}

#define AMBIENT_EFFECT_TIMEOUT_MS 1000
#define BEAT_EFFECT_TIMEOUT_MS 520
void effectSelectionEngine()
{
    static bool isAmbientSection = false;
    if (isBeatDetected && isAmbientSection)
    {
        isAmbientSection = false;
        currentEffect = beatEffectEnumValues[random(size(beatEffectEnumValues))];
    }
    else if (millis() - lastBeatTime_ms > AMBIENT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    {
        isAmbientSection = true;
        currentEffect = ambientEffectEnumValues[random(size(ambientEffectEnumValues))];
    }
    // TODO: seems to be a bit broken with this
    // else if (millis() - lastBeatTime_ms > BEAT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    // {
    //     currentEffect = static_cast<Effect>(beatEffectEnumValues[random(size(beatEffectEnumValues))]);
    // }
}

// logic for selection of next pre-set effect
void playSelectedEffect()
{
    switch (currentEffect)
    {
    case Effect::wave_bars:
        PLAY_EFFECT_SEQUENCE(wave_bars);
        return;
    case Effect::vertical_bars:
        PLAY_EFFECT_SEQUENCE(vertical_bars);
        return;
    case Effect::vertical_bars_clockwise:
        PLAY_EFFECT_SEQUENCE(vertical_bars_clockwise);
        return;
    case Effect::vertical_bars_anticlockwise:
        PLAY_EFFECT_SEQUENCE(vertical_bars_anticlockwise);
        return;
    case Effect::wave_up:
        PLAY_EFFECT_SEQUENCE(wave_up);
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
    // case Effect::ray_cross:
    //     PLAY_EFFECT_SEQUENCE(ray_cross);
    //     return;
    // case Effect::strobe:
    //     PLAY_EFFECT_SEQUENCE(strobe_);
    //     return;
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
    Serial.begin(115200);

    // Wifi Setup
    WiFi.mode(WIFI_STA); // Set device as a Wi-Fi Station
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_register_recv_cb(OnDataRecv);

    // start up the I2S peripheral
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);

    // FastLED setup
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(radioData.brightness);

    setEffectColour();
}

void loop()
{
#ifdef PRINT_PROFILING
    initialMicros = micros();
#endif
    if (radioData.isEffectCommand)
    {
        radioData.isEffectCommand = false;
        if (!(radioData.effect == 24 || radioData.effect == 25 || radioData.effect == 26))
        {
            currentEffect = static_cast<Effect>(radioData.effect);
        }
    }
    else
    {
        Colour radioDataColour = static_cast<Colour>(radioData.colour);
        if (currentColour != radioDataColour)
        {
            currentColour = radioDataColour;
            setEffectColour();
        }
    }
    static uint8_t lastBrightness = radioData.brightness;
    if (radioData.brightness != lastBrightness)
    {
        FastLED.setBrightness(radioData.brightness);
        lastBrightness = radioData.brightness;
        Serial.println("setting new brightness");
    }
#ifdef PRINT_PROFILING
    Serial.print(" Set colour: ");
    Serial.print(micros() - initialMicros);
#endif
    readMicData();
#ifdef PRINT_PROFILING
    Serial.print(" Assignedment: ");
    Serial.print(micros() - initialMicros);
#endif
    computeFFT();
#ifdef PRINT_PROFILING
    Serial.print(" Analyze: ");
    Serial.print(micros() - initialMicros);
#endif
    detectBeat();
    if (radioData.ambientOverride)
    {
        isBeatDetected = false;
    }
    effectSelectionEngine();
    playSelectedEffect();
#ifdef PRINT_PROFILING
    Serial.print(" LED: ");
    Serial.println(micros() - initialMicros);
#endif
    EVERY_N_MILLIS(15)
    {
        FastLED.show();
    }
    isBeatDetected = false;
}
