#pragma once
#include <Arduino.h>

// Btn 1..4 in the schematic -> IDs 0..3 on the wire
const uint8_t BTN_PINS[] = { 2, 3, 4, 5 };
const uint8_t NUM_BTNS   = sizeof(BTN_PINS) / sizeof(BTN_PINS[0]);
const uint16_t BTN_DEBOUNCE_MS = 20;
