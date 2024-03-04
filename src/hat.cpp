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

Colour currentColour = static_cast<Colour>(radioData.colour);
Effect currentEffect = static_cast<Effect>(radioData.effect);

#define _countof(x) (sizeof(x) / sizeof (x[0]))

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

void loopThroughEffectSequence(Effect effect_sequence[])
{
    static int i = 0;
    if (isBeatDetected)
    {
        if (++i >= _countof(effect_sequence))
        {
            i = 0;
        }
    }
    switch (effect_sequence[i])
    {
    case Effect::clear_display:
        FastLED.clear();
        break;
    case Effect::wave_up:
        waveUp();
        break;
    case Effect::wave_down:
        waveDown();
        break;
    case Effect::vertical_bars_clockwise:
        verticalBars();
        break;
    case Effect::random_cross:
        randomCross();
        break;
    case Effect::horizontal_ray:
        horizontalRay();
        break;
    case Effect::twinkle:
        twinkle();
        break;
    case Effect::wave_anticlockwise:
        waveAnticlockwise();
        break;
    case Effect::wave_clockwise:
        waveClockwise();
        break;
    case Effect::strobe:
        strobe();
        break;
    case Effect::no_effect:
        noEffect();
        break;
    }
}

#define AMBIENT_EFFECT_TIMEOUT_MS 1000
#define BEAT_EFFECT_TIMEOUT_MS 520
void effectSelectionEngine()
{
    static bool isAmbientSection = false;
    if (isBeatDetected && isAmbientSection)
    {
        isAmbientSection = false;
        currentEffect = beatEffectEnumValues[random(_countof(beatEffectEnumValues))];
    }
    else if (millis() - lastBeatTime_ms > AMBIENT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    {
        isAmbientSection = true;
        currentEffect = ambientEffectEnumValues[random(_countof(ambientEffectEnumValues))];
    }
    // TODO: seems to be a bit broken with this
    // else if (millis() - lastBeatTime_ms > BEAT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    // {
    //     currentEffect = static_cast<Effect>(beatEffectEnumValues[random(_countof(beatEffectEnumValues))]);
    // }
}

// logic for selection of next pre-set effect
void playSelectedEffectSequence()
{
    switch (currentEffect)
    {
    case Effect::wave_flash_double:
        loopThroughEffectSequence(wave_flash_double);
        return;
    case Effect::vertical_bars_clockwise:
        loopThroughEffectSequence(vertical_bars_clockwise);
        return;
    case Effect::wave_up:
        loopThroughEffectSequence(wave_up);
        return;
    case Effect::wave_down:
        loopThroughEffectSequence(wave_down);
        return;
    case Effect::wave_up_down:
        loopThroughEffectSequence(wave_up_down);
        return;
    case Effect::random_cross:
        loopThroughEffectSequence(random_cross);
        return;
    case Effect::horizontal_ray:
        loopThroughEffectSequence(horizontal_ray);
        return;
    case Effect::strobe:
        loopThroughEffectSequence(strobe_);
        return;
    case Effect::wave_anticlockwise:
        loopThroughEffectSequence(wave_anticlockwise);
        return;
    case Effect::wave_clockwise:
        loopThroughEffectSequence(wave_clockwise);
        return;
    case Effect::twinkle:
        loopThroughEffectSequence(twinkle_);
        return;
    case Effect::no_effect:
        loopThroughEffectSequence(no_effect);
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
        currentEffect = static_cast<Effect>(radioData.effect);
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
    playSelectedEffectSequence();
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
