#pragma once
#include <Arduino.h>

// The index into each array is the device ID on the wire (see pi-teensy-protocol.md).
// Reordering an array changes the wire IDs. Add new devices at the end.

// Btn 1..4 in the schematic -> IDs 0..3 on the wire
const uint8_t BTN_PINS[] = { 2, 3, 4, 5 };
const uint8_t NUM_BTNS   = sizeof(BTN_PINS) / sizeof(BTN_PINS[0]);
const uint16_t BTN_DEBOUNCE_MS = 20;

// Sliders (linear potentiometers): wiper -> pin, ends -> 3.3V and GND.
// Never connect a slider end to 5V: Teensy 4.1 pins are NOT 5V tolerant.
const uint8_t SLIDER_PINS[] = { A0, A1, A2 };  // pins 14, 15, 16
const uint8_t NUM_SLIDERS   = sizeof(SLIDER_PINS) / sizeof(SLIDER_PINS[0]);

// Proximity (analog distance) sensors, e.g. Sharp GP2Y0A02 / GP2Y0A21: output -> pin.
// Bigger value = closer. The output must stay below 3.3V.
const uint8_t PROX_PINS[] = { A3 };  // pin 17
const uint8_t NUM_PROX    = sizeof(PROX_PINS) / sizeof(PROX_PINS[0]);

// Analog inputs are reported only when they move more than the deadband,
// and at most once per interval, so the line stays quiet.
const uint16_t SLIDER_DEADBAND    = 8;   // out of 0..1023
const uint16_t SLIDER_INTERVAL_MS = 20;
const uint16_t PROX_DEADBAND      = 10;
const uint16_t PROX_INTERVAL_MS   = 50;

// TLC5947 LED driver (12-bit PWM, 24 outputs) for the button RGB LEDs.
const uint8_t TLC_DIN = 11;
const uint8_t TLC_CLK = 13;
const uint8_t TLC_LAT = 10;
const uint8_t TLC_CHANNELS = 24;

// TLC5947 outputs of each button LED, in R, G, B order. LED ID = index.
// Which LED is which button (e.g. the big red one) is set in the Pi's config, not here.
const uint8_t LED_CHANNELS[][3] = {
  { 0, 1, 2 },    // LED 0
  { 3, 4, 5 },    // LED 1
  { 6, 7, 8 },    // LED 2
  { 9, 10, 11 },  // LED 3
};
const uint8_t NUM_LEDS = sizeof(LED_CHANNELS) / sizeof(LED_CHANNELS[0]);

// WS2812B strip (through the 74AHCT125 level shifter).
const uint8_t  STRIP_PIN = 33;
const uint16_t STRIP_LEN = 212;
const uint8_t  STRIP_BRIGHTNESS = 50;  // 0..255, limits current draw from the 5V supply
