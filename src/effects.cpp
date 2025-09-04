#include "effects.h"
#include "interface.h"

#ifdef ADVANCED_JELLYFISH_GEOMETRY
#include "advanced_geometry.h"
#endif

// Initialise varialbes needed for FastLED
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LED_DATA_PIN 22

#define RANDOM_X random(MAX_X_INDEX + 1)
#define RANDOM_Y random(MAX_Y_INDEX + 1)

// default all colours to blue
CRGB colour1 = CRGB::Blue;
CRGB colour2 = CRGB::Blue;
CRGB colour3 = CRGB::Blue;

CRGB leds[NUM_LEDS] = {0};

static int MapXYtoIndex(int x, int y);
static void FadeLeds(int fadeBy);
static void GenerateDistributedRandomNumbers(int *outBuffer, int count, int min, int max);
static CRGB GetRandomColourChoice();


void FastLedInit()
{
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(radioData.brightness);

}

// ----- Effect functions -----

void NoEffect()
{
    FadeLeds(100);
}

void WaveUp()
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
                    leds[MapXYtoIndex(x, y)] = colour1;
                }
                else if ((y % 2 == 0) && (x % 2 == 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
                else if (x % 2 == 0)
                {
                    leds[MapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
            }
            y -= 1;
        }
    }
    FadeLeds(100);
}

void WaveClockwise()
{
    static int16_t x = 0;
    if (x <= MAX_X_INDEX)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            {
                if ((x % 2 == 0) && (y % 2 != 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour1;
                }
                else if ((x % 2 == 0) && (y % 2 == 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
                else if (y % 2 == 0)
                {
                    leds[MapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
            }
            ++x;
        }
    }
    else
    {
        x = 0;
    }
    FadeLeds(40);
}

void WaveAnticlockwise()
{
    static int16_t x = 0;
    if (x >= 0)
    {
        EVERY_N_MILLISECONDS(40)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            {
                if ((x % 2 == 0) && (y % 2 != 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour1;
                }
                else if ((x % 2 == 0) && (y % 2 == 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
                else if (y % 2 == 0)
                {
                    leds[MapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
            }
            --x;
        }
    }
    else
    {
        x = MAX_X_INDEX;
    }
    FadeLeds(40);
}

void WaveDown()
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
            {
                if ((y % 2 == 0) && (x % 2 != 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour1;
                }
                else if ((y % 2 == 0) && (x % 2 == 0))
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
                else if (x % 2 == 0)
                {
                    leds[MapXYtoIndex(x, y)] = colour2;
                }
                else
                {
                    leds[MapXYtoIndex(x, y)] = colour3;
                }
            }
            ++y;
        }
    }
    FadeLeds(100);
}

void VerticalBars()
{
    if (isBeatDetected)
    {
        static int xStart = 0;
        for (int x = xStart; x <= MAX_X_INDEX; x += 3)
        {
            for (int y = 0; y <= MAX_Y_INDEX; y++)
            {
                leds[MapXYtoIndex(x, y)] = colour1;
            }
        }
        FastLED.show();
        xStart = (xStart == 3) ? (xStart - 2) : ++xStart;
    }
    FadeLeds(100);
}

void HorizontalBars()
{
    if (isBeatDetected)
    {
        static int yStart = 0;
        for (int y = yStart; y <= MAX_X_INDEX; y += 3)
        { // fill x
            for (int x = 0; x <= MAX_Y_INDEX; x++)
            { // fill y
                leds[MapXYtoIndex(x, y)] = colour1;
            }
        }

        FastLED.show();
        yStart = yStart ? yStart : ++yStart; // TODO What is this logic :O
    }
    FadeLeds(100);
}

void RandomCross()
{
    if (isBeatDetected)
    {
        int randXs[3] = {0};
        GenerateDistributedRandomNumbers(randXs, 3, 0, MAX_X_INDEX);

        int randY = RANDOM_Y;

        for (int x = 0; x <= MAX_X_INDEX; x++)
        {
            leds[MapXYtoIndex(x, randY)] = colour1;
        }

        for (int y = 0; y <= MAX_Y_INDEX; y++)
        {
            for (int i = 0; i < 3; i++)
            {
                leds[MapXYtoIndex(randXs[i], y)] = colour1;
            }
        }
    }
    FadeLeds(100);
}

void HorizontalRay()
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
            leds[MapXYtoIndex(x1 + counter, y)] = colour1;
            leds[MapXYtoIndex(x1 - counter, y)] = colour1;

            leds[MapXYtoIndex(x2 + counter, y)] = colour2;
            leds[MapXYtoIndex(x2 - counter, y)] = colour2;

            if (++counter > NUMBER_X_LEDS / 4 + (NUMBER_X_LEDS % 4 != 0))
            {
                active = false;
                counter = 0;
            }
        }
    }

    FadeLeds(100);
}

void Twinkle()
{
    FadeLeds(20);
    EVERY_N_MILLIS(20)
    {
        leds[MapXYtoIndex(RANDOM_X, RANDOM_Y)] = GetRandomColourChoice();
        leds[MapXYtoIndex(RANDOM_X, RANDOM_Y)] = GetRandomColourChoice();
        leds[MapXYtoIndex(RANDOM_X, RANDOM_Y)] = GetRandomColourChoice();
        leds[MapXYtoIndex(RANDOM_X, RANDOM_Y)] = GetRandomColourChoice();
    }
}

void Strobe()
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

void ControlLed(bool beatDetected)
{
    if (beatDetected)
    {
        for (int x = 0; x <= MAX_X_INDEX; ++x)
        {
            for (int y = 0; y <= MAX_Y_INDEX; ++y)
            {
                leds[MapXYtoIndex(x, y)] = CRGB::Blue;
            }
        }
    }
    else
    {
        FadeLeds(400);
    }
}

#ifdef ADVANCED_JELLYFISH_GEOMETRY

// Advanced jellyfish effects using section-based geometry

void TentacleWave()
{
    static int16_t wave_position = -1;
    
    if (isBeatDetected)
    {
        wave_position = 0;
    }
    
    if (wave_position >= 0 && wave_position < CENTER_TENTACLE_LENGTH)
    {
        EVERY_N_MILLISECONDS(40)
        {
            // Wave down center tentacle
            int center_down_leds[CENTER_TENTACLE_LENGTH];
            int center_down_count;
            GetCenterTentacleDownLEDs(center_down_leds, &center_down_count);
            
            if (wave_position < center_down_count)
            {
                leds[center_down_leds[wave_position]] = colour1;
            }
            
            // Wave down all edge tentacles simultaneously
            for (int t = 0; t < NUM_EDGE_TENTACLES; t++)
            {
                int edge_down_leds[EDGE_TENTACLE_LENGTH];
                int edge_down_count;
                GetEdgeTentacleDownLEDs(t, edge_down_leds, &edge_down_count);
                
                if (wave_position < edge_down_count)
                {
                    leds[edge_down_leds[wave_position]] = colour2;
                }
            }
            
            wave_position++;
        }
    }
    else if (wave_position >= CENTER_TENTACLE_LENGTH && wave_position < CENTER_TENTACLE_LENGTH * 2)
    {
        EVERY_N_MILLISECONDS(40)
        {
            int up_pos = wave_position - CENTER_TENTACLE_LENGTH;
            
            // Wave up center tentacle
            int center_up_leds[CENTER_TENTACLE_LENGTH];
            int center_up_count;
            GetCenterTentacleUpLEDs(center_up_leds, &center_up_count);
            
            if (up_pos < center_up_count)
            {
                leds[center_up_leds[up_pos]] = colour1;
            }
            
            // Wave up all edge tentacles simultaneously
            for (int t = 0; t < NUM_EDGE_TENTACLES; t++)
            {
                int edge_up_leds[EDGE_TENTACLE_LENGTH];
                int edge_up_count;
                GetEdgeTentacleUpLEDs(t, edge_up_leds, &edge_up_count);
                
                if (up_pos < edge_up_count)
                {
                    leds[edge_up_leds[up_pos]] = colour2;
                }
            }
            
            wave_position++;
        }
    }
    else if (wave_position >= CENTER_TENTACLE_LENGTH * 2)
    {
        wave_position = -1; // Reset for next beat
    }
    
    FadeLeds(80);
}

void RimBlink()
{
    static bool blink_on = false;
    static unsigned long last_blink = 0;
    
    if (isBeatDetected)
    {
        blink_on = true;
        last_blink = GetMillis();
        
        // Light up all rim LEDs
        int rim_leds[TOTAL_RIM_LEDS];
        int rim_count;
        GetRimLEDs(rim_leds, &rim_count);
        
        for (int i = 0; i < rim_count; i++)
        {
            leds[rim_leds[i]] = colour3;
        }
        
        // Light up connector LEDs too
        int connector_leds[CENTER_TO_RIM_CONNECTOR_LEDS];
        int connector_count;
        GetConnectorLEDs(connector_leds, &connector_count);
        
        for (int i = 0; i < connector_count; i++)
        {
            leds[connector_leds[i]] = colour3;
        }
    }
    
    // Turn off after 100ms
    if (blink_on && (GetMillis() - last_blink) > 100)
    {
        int rim_leds[TOTAL_RIM_LEDS];
        int rim_count;
        GetRimLEDs(rim_leds, &rim_count);
        
        for (int i = 0; i < rim_count; i++)
        {
            leds[rim_leds[i]] = CRGB::Black;
        }
        
        int connector_leds[CENTER_TO_RIM_CONNECTOR_LEDS];
        int connector_count;
        GetConnectorLEDs(connector_leds, &connector_count);
        
        for (int i = 0; i < connector_count; i++)
        {
            leds[connector_leds[i]] = CRGB::Black;
        }
        
        blink_on = false;
    }
    
    FadeLeds(20);
}

void TentacleWaveToRim()
{
    static int16_t wave_position = -1;
    
    if (isBeatDetected)
    {
        wave_position = 0;
    }
    
    if (wave_position >= 0 && wave_position < CENTER_TENTACLE_LENGTH)
    {
        EVERY_N_MILLISECONDS(30)
        {
            // Wave up center tentacle
            int center_up_leds[CENTER_TENTACLE_LENGTH];
            int center_up_count;
            GetCenterTentacleUpLEDs(center_up_leds, &center_up_count);
            
            if (wave_position < center_up_count)
            {
                leds[center_up_leds[wave_position]] = colour1;
            }
            
            // Wave up all edge tentacles simultaneously  
            for (int t = 0; t < NUM_EDGE_TENTACLES; t++)
            {
                int edge_up_leds[EDGE_TENTACLE_LENGTH];
                int edge_up_count;
                GetEdgeTentacleUpLEDs(t, edge_up_leds, &edge_up_count);
                
                if (wave_position < edge_up_count)
                {
                    leds[edge_up_leds[wave_position]] = colour2;
                }
            }
            
            wave_position++;
        }
    }
    else if (wave_position == CENTER_TENTACLE_LENGTH)
    {
        // Wave has reached the top - light up the rim and connector
        int rim_leds[TOTAL_RIM_LEDS];
        int rim_count;
        GetRimLEDs(rim_leds, &rim_count);
        
        for (int i = 0; i < rim_count; i++)
        {
            leds[rim_leds[i]] = colour3;
        }
        
        int connector_leds[CENTER_TO_RIM_CONNECTOR_LEDS];
        int connector_count;
        GetConnectorLEDs(connector_leds, &connector_count);
        
        for (int i = 0; i < connector_count; i++)
        {
            leds[connector_leds[i]] = colour3;
        }
        
        wave_position++;
    }
    else if (wave_position > CENTER_TENTACLE_LENGTH)
    {
        wave_position = -1; // Reset for next beat
    }
    
    FadeLeds(60);
}

void AlternatingTentacleBlink()
{
    static bool alternate = false;
    
    if (isBeatDetected)
    {
        alternate = !alternate; // Switch which tentacles to light
        
        // Always light up center tentacle
        int center_leds[CENTER_TENTACLE_LEDS];
        int center_count;
        GetCenterTentacleLEDs(center_leds, &center_count);
        
        for (int i = 0; i < center_count; i++)
        {
            leds[center_leds[i]] = colour1;
        }
        
        // Always light up rim and connector
        int rim_leds[TOTAL_RIM_LEDS];
        int rim_count;
        GetRimLEDs(rim_leds, &rim_count);
        
        for (int i = 0; i < rim_count; i++)
        {
            leds[rim_leds[i]] = colour3;
        }
        
        int connector_leds[CENTER_TO_RIM_CONNECTOR_LEDS];
        int connector_count;
        GetConnectorLEDs(connector_leds, &connector_count);
        
        for (int i = 0; i < connector_count; i++)
        {
            leds[connector_leds[i]] = colour3;
        }
        
        // Light up alternating edge tentacles
        for (int t = 0; t < NUM_EDGE_TENTACLES; t++)
        {
            if ((t % 2 == 0) == alternate) // Alternate between even/odd tentacles
            {
                int edge_leds[EDGE_TENTACLE_LEDS_EACH];
                int edge_count;
                GetEdgeTentacleLEDs(t, edge_leds, &edge_count);
                
                for (int i = 0; i < edge_count; i++)
                {
                    leds[edge_leds[i]] = colour2;
                }
            }
        }
    }
    
    FadeLeds(100);
}


#endif // ADVANCED_JELLYFISH_GEOMETRY

// ----- Effect utils -----

// Map any x, y coordinate on LED matrix to LED array index
static int MapXYtoIndex(int x, int y)
{
#ifdef ADVANCED_JELLYFISH_GEOMETRY
    // Use advanced geometry mapping
    return MapXYToAdvancedLED(x, y);
#elif defined(VERTICAL_ZIGZAG)
    // Vertical zigzag for jellyfish - tentacles alternate up/down
    // Each pair of X columns represents one tentacle (down and up)
    x %= NUMBER_X_LEDS;
    if (x < 0)
    {
        x += NUMBER_X_LEDS;
    }
    int i;
    if (x % 2 == 0)
    {
        // Even X: LEDs go down (normal direction)
        i = y + x * NUMBER_Y_LEDS;
    }
    else
    {
        // Odd X: LEDs go up (reversed direction)
        i = (x + 1) * NUMBER_Y_LEDS - y - 1;
    }
#else
    // Horizontal zigzag for hat - rows alternate left/right
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
#endif

    return i;
}

static void FadeLeds(int fadeBy)
{
    EVERY_N_MILLIS(15)
    {
        fadeToBlackBy(leds, NUM_LEDS, fadeBy);
    }
}

static void GenerateDistributedRandomNumbers(int *outBuffer, int count, int min, int max)
{
    int zoneSize = (max - min + 1) / count;

    for (int i = 0; i < count - 1; i++)
    {
        outBuffer[i] = random(min + zoneSize * i, min + zoneSize * (i + 1));
    }
    outBuffer[count - 1] = random(min + zoneSize * (count - 1), max + 1);
}

static CRGB GetRandomColourChoice()
{
    switch (random(3))
    {
    case 0:
        return colour1;
    case 1:
        return colour2;
    case 2:
        return colour3;
    default:
        return colour1;
    }
}
