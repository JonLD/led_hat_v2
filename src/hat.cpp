#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <interface.h>
#include <FastLED.h>
#include <config.h>
#include <beat_detection.h>

typedef void (*effect_function_ptr_t)();

uint8_t com7Address[] = {0x0C, 0xB8, 0x15, 0xF8, 0xF6, 0x80};

// max x and y values of LED matrix
#define MAX_X_INDEX (NUMBER_X_LEDS - 1)
#define MAX_Y_INDEX (NUMBER_Y_LEDS - 1)

#define RANDOM_X random(MAX_X_INDEX + 1)
#define RANDOM_Y random(MAX_Y_INDEX + 1)

// default all colours to blue
CRGB colour1 = CRGB::Blue;
CRGB colour2 = CRGB::Blue;
CRGB colour3 = CRGB::Blue;

// Initialise varialbes needed for FastLED
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS (NUMBER_X_LEDS * NUMBER_Y_LEDS)
#define LED_DATA_PIN 22

#define PLAY_EFFECT_SEQUENCE(effect) play_effect_sequence(effect, size(effect))

CRGB leds[NUM_LEDS] = {0};

// enum encoding effect with whole and half denoting
// 1 and 1/2 beat length of effect respectively
enum effect_choice : int
{
    clear_display, // clear all leds
    wave_whole,    // vertically rising horizontal wave with fading tail
    wave_half,
    flash_whole, // flash of vertical lines
    flash_half,
    twinkle_whole, // bright spots lighting every 16th note and slowing fading
    twinkle_half,
    strobe_whole,
};

//-------------- Function prototypes --------------
void twinkle();
void verticalBars();
void waveUp();
void waveDown();
void randomCross();
void strobe();
void waveClockwise();
void waveAnticlockwise();

effect_function_ptr_t wave_flash_double[] = {waveUp, waveUp, verticalBars, verticalBars};
effect_function_ptr_t triple_bar_wave[] = {verticalBars, verticalBars, verticalBars, waveUp};
effect_function_ptr_t vertical_bars_clockwise[] = {verticalBars};
effect_function_ptr_t twinkle_[] = {twinkle};
effect_function_ptr_t wave_clockwise[] = {waveClockwise};
effect_function_ptr_t wave_anticlockwise[] = {waveAnticlockwise};
effect_function_ptr_t wave_up[] = {waveUp};
effect_function_ptr_t strobe_[] = {strobe};
effect_function_ptr_t wave_down[] = {waveDown};
effect_function_ptr_t wave_up_down[] = {waveDown, waveUp, waveDown, waveUp};
effect_function_ptr_t eigh_wave_eight_bars[] = {
    waveUp, waveUp, waveUp, waveUp, waveUp, waveUp, waveUp, waveUp,
    verticalBars, verticalBars, verticalBars, verticalBars, verticalBars, verticalBars, verticalBars, verticalBars,
    waveDown, waveDown, waveDown, waveDown, waveDown, waveDown, waveDown, waveDown};
effect_function_ptr_t random_cross[] = {randomCross};

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
    Serial.println();
}

//-------------- Effect Control --------------

// logic for selection of different colour pallettes
void setEffectColour()
{
    switch (radioData.colour)
    {
    case red:
        colour1 = colour2 = colour3 = CRGB::Red;
        break;
    case blue:
        colour1 = colour2 = colour3 = CRGB::Blue;
        break;
    case green:
        colour1 = colour2 = colour3 = CRGB::Green;
        break;
    case purple:
        colour1 = colour2 = colour3 = CRGB::Purple;
        break;
    case white:
        colour1 = colour2 = colour3 = CRGB::White;
        break;
    case yellow:
        colour1 = colour2 = colour3 = CRGB::Yellow;
        break;
    case orange:
        colour1 = colour2 = colour3 = CRGB::OrangeRed;
        break;
    case red_white:
        colour1 = colour2 = CRGB::Red;
        colour3 = CRGB::White;
        break;
    case green_white:
        colour1 = colour2 = CRGB::Green;
        colour3 = CRGB::White;
        break;
    case blue_white:
        colour1 = colour2 = CRGB::Blue;
        colour3 = CRGB::White;
        break;
    case cb:
        colour1 = CRGB::OrangeRed;
        colour2 = CRGB::Green;
        colour3 = CRGB::Purple;
        break;
    case cd:
        colour1 = CRGB::Yellow;
        colour2 = CRGB::Blue;
        colour3 = CRGB::Purple;
        break;
    case fire:
        colour1 = CRGB::Yellow;
        colour2 = CRGB::Red;
        colour3 = CRGB::OrangeRed;
        break;
    case purue:
        colour1 = CRGB::Purple;
        colour2 = CRGB::Red;
        colour3 = CRGB::Blue;
        break;
    case blue_red:
        colour1 = colour2 = CRGB::Blue;
        colour3 = CRGB::Red;
        break;
    }
}

void play_effect_sequence(effect_function_ptr_t effects_array[], size_t array_size)
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
    if (((millis() - lastBeatTime_ms) > AMBIENT_EFFECT_TIMEOUT_MS) && !isAmbientSection)
    {
        radioData.effect = random(32, 36);
        isAmbientSection = true;
    }
    else if (millis() - lastBeatTime_ms > BEAT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    {
        radioData.effect = random(17, 19);
    }
    if (isBeatDetected && isAmbientSection)
    {
        isAmbientSection = false;
        radioData.effect = random(17, 19);
    }
}

// logic for selection of next pre-set effect
void playSelectedEffect()
{
    PLAY_EFFECT_SEQUENCE(random_cross);
    return;
    switch (radioData.effect)
    {
    case enum_wave_flash_double:
        PLAY_EFFECT_SEQUENCE(wave_flash_double);
        break;
    case enum_vertical_bars_clockwise:
        PLAY_EFFECT_SEQUENCE(vertical_bars_clockwise);
        break;
    case enum_twinkle:
        PLAY_EFFECT_SEQUENCE(twinkle_);
        break;
    case enum_just_wave_up:
        PLAY_EFFECT_SEQUENCE(wave_up);
        break;
    case enum_just_wave_down:
        PLAY_EFFECT_SEQUENCE(wave_down);
        break;
    case enum_strobe:
        PLAY_EFFECT_SEQUENCE(strobe_);
        break;
    case enum_wave_clockwise:
        PLAY_EFFECT_SEQUENCE(wave_clockwise);
        break;
    case enum_wave_anticlockwise:
        PLAY_EFFECT_SEQUENCE(wave_anticlockwise);
        break;
    }
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

radioData_t oldRadioData = radioData;
void loop()
{
#ifdef PRINT_PROFILING
    initialMicros = micros();
#endif
    if (oldRadioData.colour != radioData.colour)
    {
        setEffectColour();
        oldRadioData.colour = radioData.colour;
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

// ----- Effects -----
// Map any x, y coordinate on LED matrix to LED array index
int mapXYtoIndex(int x, int y)
{
#ifdef IS_HAT
    if (y == 0 || y == 1)
    {
        x++;
    }
#endif
    x %= NUMBER_X_LEDS;
    int i;
    if (y % 2 == 0)
    {
        i = x + (MAX_X_INDEX + 1) * y; // i steps up by MAX_X_INDEX+1 each row
    }
    // LED strips setup in alternating direction so x value flips sign
    else
    {
        i = (MAX_X_INDEX + 1) * (y + 1) - (x + 1);
    }

    return i;
}

void fadeLeds(int fadeBy)
{
    EVERY_N_MILLIS(15)
    {
        fadeToBlackBy(leds, NUM_LEDS, fadeBy);
    }
}

// 1
void waveUp()
{
    static int16_t y = -1;
    if (isBeatDetected)
    {
        y = MAX_Y_INDEX;
    }
    if (y >= 0)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int x = 0; x <= MAX_X_INDEX; ++x)
            { // Fill x row
                // sequence of logic to create pattern
                if ((y % 2 == 0) && (x % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((y % 2 == 0) && (x % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (x % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            y -= 1;
        }
    }
    fadeLeds(100);
}

void waveClockwise()
{
    static int16_t x = 0;
    if (x <= MAX_X_INDEX)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            { // Fill x row
                // sequence of logic to create pattern
                if ((x % 2 == 0) && (y % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((x % 2 == 0) && (y % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (y % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            ++x;
        }
    }
    else
    {
        x = 0;
    }
    fadeLeds(100);
}

void waveAnticlockwise()
{
    static int16_t x = 0;
    if (x >= 0)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            { // Fill x row
                // sequence of logic to create pattern
                if ((x % 2 == 0) && (y % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((x % 2 == 0) && (y % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (y % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            --x;
        }
    }
    else
    {
        x = MAX_X_INDEX;
    }
    fadeLeds(100);
}

void waveDown()
{
    static int16_t y = -1;
    if (isBeatDetected)
    {
        y = 0;
    }
    if (y <= MAX_Y_INDEX)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int x = 0; x <= MAX_X_INDEX; ++x)
            { // Fill x row
                // sequence of logic to create pattern
                if ((y % 2 == 0) && (x % 2 != 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour1;
                }
                else if ((y % 2 == 0) && (x % 2 == 0))
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
                else if (x % 2 == 0)
                {
                    leds[mapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[mapXYtoIndex(x, y)] = colour3;
                }
            }
            ++y;
        }
    }
    fadeLeds(100);
}

// 2
// Flash LED and decay to 0
void verticalBars()
{
    if (isBeatDetected)
    {
        static int xStart = 0;
        for (int x = xStart; x <= MAX_X_INDEX; x += 3)
        { // fill x
            for (int y = 0; y <= MAX_Y_INDEX; y++)
            { // fill y
                leds[mapXYtoIndex(x, y)] = colour1;
            }
        }
        FastLED.show();
        xStart = (xStart == 3) ? (xStart - 2) : ++xStart;
    }
    fadeLeds(100);
}

void horizontalBars()
{
    if (isBeatDetected)
    {
        static int yStart = 0;
        for (int y = yStart; y <= MAX_X_INDEX; y += 3)
        { // fill x
            for (int x = 0; x <= MAX_Y_INDEX; x++)
            { // fill y
                leds[mapXYtoIndex(x, y)] = colour1;
            }
        }
        FastLED.show();
        yStart = yStart ? yStart : ++yStart;
    }
    fadeLeds(100);
}

void randomCross()
{
    if (isBeatDetected)
    {
        int rand_x_limit = NUMBER_X_LEDS / 3;
        int rand_x1 = random(rand_x_limit);
        int rand_x2 = random(rand_x_limit, rand_x_limit * 2);
        int rand_x3 = random(rand_x_limit * 2, NUMBER_X_LEDS);

        int rand_y = RANDOM_Y;

        for (int x = 0; x <= MAX_X_INDEX; x++)
        {
            leds[mapXYtoIndex(x, rand_y)] = colour1;
        }

        for (int y = 0; y <= MAX_Y_INDEX; y++)
        {
            leds[mapXYtoIndex(rand_x1, y)] = colour1;
            leds[mapXYtoIndex(rand_x2, y)] = colour1;
            leds[mapXYtoIndex(rand_x3, y)] = colour1;
        }
    }
    fadeLeds(100);
}

// add new bright spots every quarter note
void twinkle()
{
    fadeLeds(20);
    EVERY_N_MILLIS(20)
    {
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = colour1;
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = colour2;
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = colour3;
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = colour1;
    }
}

void strobe()
{
    FastLED.clear();
    EVERY_N_MILLISECONDS(40)
    {
        fill_solid(leds, NUM_LEDS, CRGB::White);
    }

    EVERY_N_MILLISECONDS(80)
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
}

void controlLed(bool isBeatDetected)
{
    if (isBeatDetected)
    {
        for (int x = 0; x <= MAX_X_INDEX; ++x)
        { // fill x
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            { // fill y
                leds[mapXYtoIndex(x, y)] = CRGB::Blue;
            }
        }
    }
    else
    {
        fadeLeds(400);
    }
}
