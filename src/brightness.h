#include <Arduino.h>
// #include <interface.h>

const uint8_t BRIGHTNESS_RE_PIN_A = 19;
const uint8_t BRIGHTNESS_RE_PIN_B = 18;
const uint8_t BRIGHTNESS_LIMIT = 255;
byte brightness = 100;


void adjust_brightness() {
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
          (brightness >= 3)) {
        brightness -= 3;
        Serial.println(brightness);
      }
      // if clockwise BRIGHTNESS_RE_PIN_B returns to HIGH after
      // BRIGHTNESS_RE_PIN_A and thus will still be low
      else if ((digitalRead(BRIGHTNESS_RE_PIN_B) == LOW) &&
               (brightness <= (BRIGHTNESS_LIMIT - 3))) {
        brightness += 3;
        Serial.println(brightness);
      }
    }
    brightness_pin_a_previous = brightness_pin_a_current;
}

void setup() {
  Serial.begin(115200);
  pinMode(BRIGHTNESS_RE_PIN_A, INPUT_PULLUP);
  pinMode(BRIGHTNESS_RE_PIN_B, INPUT_PULLUP);
//   attachInterrupt(digitalPinToInterrupt(BRIGHTNESS_RE_PIN_A), adjust_brightness,
//                   CHANGE);
}

void loop() {
    adjust_brightness();

}
