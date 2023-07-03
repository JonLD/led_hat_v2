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

// default all colours to blue
CRGB colour1 = CRGB::Blue;
CRGB colour2 = CRGB::Blue;
CRGB colour3 = CRGB::Blue;

// Initialise varialbes needed for FastLED
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS (NUMBER_X_LEDS * NUMBER_Y_LEDS)
#define LED_DATA_PIN 22

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
void twinkle_shaker();
void verticalBars();
void wave_effect();
void strobe();

effect_function_ptr_t wave_flash_double[] = {wave_effect, wave_effect, verticalBars, verticalBars};
effect_function_ptr_t just_flash[] = {verticalBars};
effect_function_ptr_t twinkle[] = {twinkle_shaker};
effect_function_ptr_t just_wave[] = {wave_effect};
effect_function_ptr_t strobe_bar[] = {strobe};

// for getting the length of the above effect function pointer arrays
template <class T, size_t N>
constexpr size_t size(T (&)[N]) { return N; }

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

void play_effect_sequence(effect_function_ptr_t effects_array[], size_t array_size, bool isBeatDetected)
{
    static int i = 0;
    if (isBeatDetected && ++i >= array_size)
    {
        i = 0;
    }
    effects_array[i]();
}

// logic for selection of next pre-set effect
void play_selected_effect(bool isBeatDetected)
{
    switch (radioData.effect)
    {
    case enum_wave_flash_double:
        play_effect_sequence(wave_flash_double, size(wave_flash_double), isBeatDetected);
        break;
    case enum_just_flash:
        play_effect_sequence(just_flash, size(just_flash), isBeatDetected);
        break;
    case enum_twinkle:
        play_effect_sequence(twinkle, size(twinkle), isBeatDetected);
        break;
    case enum_just_wave:
        play_effect_sequence(just_wave, size(just_wave), isBeatDetected);
        break;
    case enum_strobe_bar:
        play_effect_sequence(strobe_bar, size(strobe_bar), isBeatDetected);
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
    isBeatDetected = detectBeat();
    play_selected_effect(isBeatDetected);
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

// 1
void wave_effect()
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
    EVERY_N_MILLIS(5)
    {
        fadeToBlackBy(leds, NUM_LEDS, 50);
    }
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
    EVERY_N_MILLIS(5)
    {
        fadeToBlackBy(leds, NUM_LEDS, 50);
    }
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
    EVERY_N_MILLIS(5)
    {
        fadeToBlackBy(leds, NUM_LEDS, 50);
    }
}

// add new bright spots every quarter note
void twinkle_shaker()
{
    EVERY_N_MILLIS(20)
    {
        fadeToBlackBy(leds, NUM_LEDS, 25);
        int rand_x1 = random(MAX_X_INDEX + 1);
        int rand_y1 = random(MAX_Y_INDEX + 1);
        leds[mapXYtoIndex(rand_x1, rand_y1)] = colour1;
        int rand_x2 = random(MAX_X_INDEX + 1);
        int rand_y2 = random(MAX_Y_INDEX + 1);
        leds[mapXYtoIndex(rand_x2, rand_y2)] = colour2;
        int rand_x3 = random(MAX_X_INDEX + 1);
        int rand_y3 = random(MAX_Y_INDEX + 1);
        leds[mapXYtoIndex(rand_x3, rand_y3)] = colour3;
        int rand_x4 = random(MAX_X_INDEX + 1);
        int rand_y4 = random(MAX_Y_INDEX + 1);
        leds[mapXYtoIndex(rand_x4, rand_y4)] = colour1;
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
        EVERY_N_MILLIS(2)
        {
            fadeToBlackBy(leds, NUM_LEDS, 50);
        }
    }
}
