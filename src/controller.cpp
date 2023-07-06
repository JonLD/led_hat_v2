#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <interface.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_NeoTrellis.h>
#include <config.h>

uint8_t com8Address[] = {0x0C, 0xB8, 0x15, 0xF8, 0xE6, 0x40};
uint8_t com7Address[] = {0x0C, 0xB8, 0x15, 0xF8, 0xF6, 0x80};

// Must remain global!
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    char macStr[18];
    Serial.print("Packet to: ");
    // Copies the sender mac address to a string
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
    Serial.print(" send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// --------- Trellis -----------
#define Y_DIM 8 // number of rows of key
#define X_DIM 4 // number of columns of keys

// create a matrix of trellis panels
Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {
    {Adafruit_NeoTrellis(0x2E)},
    {Adafruit_NeoTrellis(0x30)}};

// pass this matrix to the multitrellis object
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM / 4,
                              X_DIM / 4);

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
    if (WheelPos < 85)
    {
        return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
    else if (WheelPos < 170)
    {
        WheelPos -= 85;
        return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    return 0;
}

void setColorOrEffect(int keypadButtonNumber)
{
    if (keypadButtonNumber <= 16)
    {
        radioData.isEffectCommand = false;
        radioData.colour = keypadButtonNumber;
    }
    else if (keypadButtonNumber <= 21)
    {
        radioData.isEffectCommand = true;
        radioData.effect = keypadButtonNumber;
    }
}

// define a callback for key presses
TrellisCallback blink(keyEvent evt)
{
    if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
    {
        trellis.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, X_DIM * Y_DIM,
                                                     0, 255))); // on rising
        setColorOrEffect(evt.bit.NUM);
    }
    else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
    {
        trellis.setPixelColor(evt.bit.NUM, 0); // off falling2
    }
    trellis.show();
    return 0;
}

void setupTrellisKeypad()
{
    if (!trellis.begin())
    {
        Serial.println("failed to begin trellis");
        while (1)
            delay(1);
    }
    else
    {
        Serial.println("Trellis setup successful!");
    }
    /* the array can be addressed as x,y or with the key number */
    for (int i = 0; i < Y_DIM * X_DIM; i++)
    {
        trellis.setPixelColor(
            i, Wheel(map(i, 0, X_DIM * Y_DIM, 0, 50))); // addressed with keynum
        trellis.show();
        delay(50);
    }

    for (int y = 0; y < Y_DIM; y++)
    {
        for (int x = 0; x < X_DIM; x++)
        {
            // activate rising and falling edges on all keys
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
            trellis.registerCallback(x, y, blink);
            trellis.setPixelColor(x, y, 0x000000); // addressed with x,y
            trellis.show();                        // show all LEDs
            delay(50);
        }
    }
}

// --------- Wifi -----------
void setupWifiConnection()
{
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);

    // register peer
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    // register first peer
    memcpy(peerInfo.peer_addr, com7Address, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }
}

void trySend()
{
    static radioData_t oldRadioData = radioData;
    if (!(oldRadioData == radioData))
    {
        esp_err_t result = esp_now_send(0, (uint8_t *)&radioData, sizeof(radioData_t));

        if (result == ESP_OK)
        {
            Serial.println("Sent with success");
            Serial.print("Effect enum: ");
            Serial.println(radioData.effect);
            Serial.print("Colour enum: ");
            Serial.println(radioData.colour);
            oldRadioData = radioData;
        }
        else
        {
            Serial.println("Error sending the data");
        }
    }
}

// ----- Brightness knob -----
const uint8_t BRIGHTNESS_LIMIT = 255;

void pollBrightnessKnob() {
  static uint8_t brightness_pin_a_current = HIGH;
  static uint8_t brightness_pin_a_previous = brightness_pin_a_current;

    /*BRIGHTNESS_RE_PIN_A and BRIGHTNESS_RE_PIN_B both defualt to HIGH
    On rotation both pass through LOW to HIGH*/
    brightness_pin_a_current = digitalRead(BRIGHTNESS_RE_PIN_A);

    if ((brightness_pin_a_previous == LOW) &&
        (brightness_pin_a_current == HIGH)) {
      // if counter clockwise BRIGHTNESS_RE_PIN_B returns to HIGH before
      // BRIGHTNESS_RE_PIN_A and so will be HIGH
      if ((digitalRead(BRIGHTNESS_RE_PIN_B) == HIGH) &&
          (radioData.brightness >= 3)) {
        radioData.brightness -= 3;
        Serial.println(radioData.brightness);
      }
      // if clockwise BRIGHTNESS_RE_PIN_B returns to HIGH after
      // BRIGHTNESS_RE_PIN_A and thus will still be low
      else if ((digitalRead(BRIGHTNESS_RE_PIN_B) == LOW) &&
               (radioData.brightness <= (BRIGHTNESS_LIMIT - 3))) {
        radioData.brightness += 3;
        Serial.println(radioData.brightness);
      }
    }
    brightness_pin_a_previous = brightness_pin_a_current;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Setting up devic");
    setupWifiConnection();
    setupTrellisKeypad();
    pinMode(BRIGHTNESS_RE_PIN_A, INPUT_PULLUP);
    pinMode(BRIGHTNESS_RE_PIN_B, INPUT_PULLUP);
}

void loop()
{
    trellis.read();
    pollBrightnessKnob();
    trySend();
}