#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <interface.h>
#include <FastLED.h>
#include <config.h>
#include <beat_detection.h>

typedef void (*effect_function_ptr_t)();
typedef const effect_function_ptr_t effect_array_t[];

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

//-------------- Function prototypes --------------
void twinkle();
void verticalBars();
void waveUp();
void waveDown();
void randomCross();
void horizontalRays();
void strobe();
void waveClockwise();
void waveAnticlockwise();

effect_array_t wave_flash_double = {waveUp, waveUp, verticalBars, verticalBars};
effect_array_t triple_bar_wave = {verticalBars, verticalBars, verticalBars, waveUp};
effect_array_t vertical_bars_clockwise = {verticalBars};
effect_array_t twinkle_ = {twinkle};
effect_array_t wave_clockwise = {waveClockwise};
effect_array_t wave_anticlockwise = {waveAnticlockwise};
effect_array_t wave_up = {waveUp};
effect_array_t strobe_ = {strobe};
effect_array_t wave_down = {waveDown};
effect_array_t wave_up_down = {waveDown, waveUp, waveDown, waveUp};
effect_array_t eigh_wave_eight_bars = {
    waveUp, waveUp, waveUp, waveUp, waveUp, waveUp, waveUp, waveUp,
    verticalBars, verticalBars, verticalBars, verticalBars, verticalBars, verticalBars, verticalBars, verticalBars,
    waveDown, waveDown, waveDown, waveDown, waveDown, waveDown, waveDown, waveDown};
effect_array_t random_cross = {randomCross};
effect_array_t horizontal_rays = {horizontalRays};

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
    if (((millis() - lastBeatTime_ms) > AMBIENT_EFFECT_TIMEOUT_MS) && !isAmbientSection)
    {
        currentEffect = static_cast<Effect>(random(32, 36));
        isAmbientSection = true;
    }
    else if (millis() - lastBeatTime_ms > BEAT_EFFECT_TIMEOUT_MS && !isAmbientSection)
    {
        currentEffect = static_cast<Effect>(random(17, 19));
    }
    if (isBeatDetected && isAmbientSection)
    {
        isAmbientSection = false;
        currentEffect = static_cast<Effect>(random(17, 19));
    }
}

// logic for selection of next pre-set effect
void playSelectedEffect()
{
    switch (currentEffect)
    {
    case Effect::wave_flash_double:
        PLAY_EFFECT_SEQUENCE(wave_flash_double);
        break;
    case Effect::vertical_bars_clockwise:
        PLAY_EFFECT_SEQUENCE(vertical_bars_clockwise);
        break;
    case Effect::twinkle:
        PLAY_EFFECT_SEQUENCE(twinkle_);
        break;
    case Effect::just_wave_up:
        PLAY_EFFECT_SEQUENCE(wave_up);
        break;
    case Effect::just_wave_down:
        PLAY_EFFECT_SEQUENCE(wave_down);
        break;
    case Effect::strobe:
        PLAY_EFFECT_SEQUENCE(strobe_);
        break;
    case Effect::wave_clockwise:
        PLAY_EFFECT_SEQUENCE(wave_clockwise);
        break;
    case Effect::wave_anticlockwise:
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
    if (x < 0)
    {
        x += NUMBER_X_LEDS;
    }
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

void generateDistributedRandomNumbers(int *outBuffer, int count, int min, int max)
{
    int zoneSize = (max - min + 1) / count;

    for (int i = 0; i < count - 1; i++)
    {
        outBuffer[i] = random(min + zoneSize * i, min + zoneSize * (i + 1));
    }
    outBuffer[count - 1] = random(min + zoneSize * (count - 1), max + 1);
}

CRGB getRandomColourChoice()
{
    switch (random(3))
    {
    case 0:
        return colour1;
    case 1:
        return colour2;
    case 2:
        return colour3;
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
    fadeLeds(40);
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
    fadeLeds(30);
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
    fadeLeds(40);
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
        int randXs[3] = {0};
        generateDistributedRandomNumbers(randXs, 3, 0, MAX_X_INDEX);

        int randY = RANDOM_Y;

        for (int x = 0; x <= MAX_X_INDEX; x++)
        {
            leds[mapXYtoIndex(x, randY)] = colour1;
        }

        for (int y = 0; y <= MAX_Y_INDEX; y++)
        {
            for (int i = 0; i < 3; i++)
            {
                leds[mapXYtoIndex(randXs[i], y)] = colour1;
            }
        }
    }
    fadeLeds(100);
}

void horizontalRays()
{
    static bool active = false;
    static int counter = 0;
    static int x1 = 0;
    static int x2 = MAX_X_INDEX / 2;
    static int y = -1;

    if (isBeatDetected)
    {
        active = true;
        counter = 0;

        y = RANDOM_Y;
    }

    if (active)
    {
        EVERY_N_MILLISECONDS(5)
        {
            leds[mapXYtoIndex(x1 + counter, y)] = colour1;
            leds[mapXYtoIndex(x1 - counter, y)] = colour1;

            leds[mapXYtoIndex(x2 + counter, y)] = colour2;
            leds[mapXYtoIndex(x2 - counter, y)] = colour2;

            if (++counter > NUMBER_X_LEDS / 4 + (NUMBER_X_LEDS % 4 != 0))
            {
                active = false;
                counter = 0;
            }
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
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
        leds[mapXYtoIndex(RANDOM_X, RANDOM_Y)] = getRandomColourChoice();
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
