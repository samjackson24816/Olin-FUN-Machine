// Teensy 4.1 firmware. Protocol: /pi-teensy-protocol.md (version 2).
// The Teensy only reports inputs and drives outputs. It never decides; the Pi does.
#include <Adafruit_NeoPixel.h>
#include "pins.h"

const uint8_t PROTOCOL_VERSION = 2;

// All structs are declared up here: the Arduino IDE adds function prototypes
// above the first function, so types used in function signatures must come first.
struct Button { bool stable; bool lastRaw; uint32_t changedMs; };
struct Analog { float filtered; uint16_t sent; uint32_t sentMs; };
// periodMs == 0: solid colour. periodMs > 0: breathe from off to the colour and back.
struct Led { uint8_t r, g, b; uint16_t periodMs; uint32_t startMs; };

// ---------- Inputs: buttons ----------

Button btns[NUM_BTNS];

void sendButton(uint8_t id) {
  Serial.printf("BTN %d %d\n", id, btns[id].stable);
}

void updateButtons() {
  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    bool raw = (digitalRead(BTN_PINS[i]) == LOW);
    if (raw != btns[i].lastRaw) {
      // the pin just changed: restart the settle timer
      btns[i].lastRaw = raw;
      btns[i].changedMs = millis();
    } else if (raw != btns[i].stable &&
               millis() - btns[i].changedMs >= BTN_DEBOUNCE_MS) {
      // it has held the new value long enough: accept it and report
      btns[i].stable = raw;
      sendButton(i);
    }
  }
}

// ---------- Inputs: sliders and proximity sensors (analog, 0..1023) ----------

Analog sliders[NUM_SLIDERS];
Analog prox[NUM_PROX];

uint16_t analogValue(const Analog &a) {
  return (uint16_t)(a.filtered + 0.5f);
}

void sendAnalog(const char *kind, uint8_t id, Analog &a) {
  a.sent = analogValue(a);
  a.sentMs = millis();
  Serial.printf("%s %d %d\n", kind, id, a.sent);
}

// Smooth each reading, then report it if it moved past the deadband
// and the last report for that input is old enough.
void updateAnalogs(const char *kind, const uint8_t *pins, Analog *a, uint8_t n,
                   uint16_t deadband, uint16_t intervalMs) {
  for (uint8_t i = 0; i < n; i++) {
    int raw = analogRead(pins[i]);
    a[i].filtered += (raw - a[i].filtered) * 0.25f;  // simple low-pass filter
    int moved = abs((int)analogValue(a[i]) - (int)a[i].sent);
    if (moved > deadband && millis() - a[i].sentMs >= intervalMs) {
      sendAnalog(kind, i, a[i]);
    }
  }
}

void initAnalogs(const uint8_t *pins, Analog *a, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    a[i].filtered = analogRead(pins[i]);
    a[i].sent = analogValue(a[i]);
    a[i].sentMs = millis();
  }
}

// ---------- Outputs: button LEDs on the TLC5947 ----------

Led leds[NUM_LEDS];
uint16_t tlcValues[TLC_CHANNELS];  // 0..4095 per output
uint32_t lastLedUpdateMs = 0;

// Shift all 24 outputs out (output 23 first, 12 bits each, MSB first), then latch.
void tlcWrite() {
  digitalWrite(TLC_LAT, LOW);
  for (int ch = TLC_CHANNELS - 1; ch >= 0; ch--) {
    for (int bit = 11; bit >= 0; bit--) {
      digitalWrite(TLC_CLK, LOW);
      digitalWrite(TLC_DIN, (tlcValues[ch] >> bit) & 1);
      delayNanoseconds(100);
      digitalWrite(TLC_CLK, HIGH);
      delayNanoseconds(100);
    }
  }
  digitalWrite(TLC_CLK, LOW);
  digitalWrite(TLC_LAT, HIGH);
  delayNanoseconds(100);
  digitalWrite(TLC_LAT, LOW);
}

// level: 0.0 (off) .. 1.0 (the LED's full colour)
void putLed(uint8_t id, float level) {
  const Led &l = leds[id];
  tlcValues[LED_CHANNELS[id][0]] = l.r * level * 4095 / 255;
  tlcValues[LED_CHANNELS[id][1]] = l.g * level * 4095 / 255;
  tlcValues[LED_CHANNELS[id][2]] = l.b * level * 4095 / 255;
}

void setLed(uint8_t id, uint8_t r, uint8_t g, uint8_t b, uint16_t periodMs) {
  leds[id] = { r, g, b, periodMs, millis() };
  putLed(id, periodMs == 0 ? 1.0f : 0.0f);  // a pulse starts from off
  tlcWrite();
}

// Recalculate the breathing LEDs every 10 ms.
void updateLeds() {
  if (millis() - lastLedUpdateMs < 10) return;
  lastLedUpdateMs = millis();
  bool changed = false;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (leds[i].periodMs == 0) continue;
    float phase = (float)((millis() - leds[i].startMs) % leds[i].periodMs) / leds[i].periodMs;
    float wave = (1.0f - cosf(2.0f * PI * phase)) / 2.0f;  // 0 -> 1 -> 0, smooth
    putLed(i, wave * wave);  // squared: looks more even to the eye
    changed = true;
  }
  if (changed) tlcWrite();
}

// ---------- Outputs: WS2812B strip ----------

Adafruit_NeoPixel strip(STRIP_LEN, STRIP_PIN, NEO_GRB + NEO_KHZ800);

void stripFill(uint8_t r, uint8_t g, uint8_t b) {
  strip.fill(strip.Color(r, g, b));
  strip.show();
}

// ---------- Safe state ----------

void allOff() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = { 0, 0, 0, 0, 0 };
    putLed(i, 0.0f);
  }
  tlcWrite();
  stripFill(0, 0, 0);
}

// ---------- Serial commands ----------

bool isColour(int v) { return v >= 0 && v <= 255; }

void handleLine(const char* line) {
  int id, r, g, b, period;
  char extra;  // catches anything after the expected fields

  if (strcmp(line, "PING") == 0)          { Serial.println("PONG"); }
  else if (strcmp(line, "SNAPSHOT") == 0) {
    for (uint8_t i = 0; i < NUM_BTNS; i++)    sendButton(i);
    for (uint8_t i = 0; i < NUM_SLIDERS; i++) sendAnalog("SLIDER", i, sliders[i]);
    for (uint8_t i = 0; i < NUM_PROX; i++)    sendAnalog("PROX", i, prox[i]);
  }
  else if (strcmp(line, "STOP") == 0)     { allOff(); Serial.println("OK STOP"); }
  else if (sscanf(line, "LED %d %d %d %d %c", &id, &r, &g, &b, &extra) == 4) {
    if (id < 0 || id >= NUM_LEDS || !isColour(r) || !isColour(g) || !isColour(b)) {
      Serial.println("ERR range");
    } else {
      setLed(id, r, g, b, 0);
    }
  }
  else if (sscanf(line, "PULSE %d %d %d %d %d %c", &id, &r, &g, &b, &period, &extra) == 5) {
    if (id < 0 || id >= NUM_LEDS || !isColour(r) || !isColour(g) || !isColour(b) ||
        period < 100 || period > 60000) {
      Serial.println("ERR range");
    } else {
      setLed(id, r, g, b, period);
    }
  }
  else if (sscanf(line, "STRIP %d %d %d %c", &r, &g, &b, &extra) == 3) {
    if (!isColour(r) || !isColour(g) || !isColour(b)) {
      Serial.println("ERR range");
    } else {
      stripFill(r, g, b);
    }
  }
  else                                    { Serial.println("ERR unknown"); }
}

// Collect characters until \n, then handle the line. Lines over 63 characters are discarded.
void updateSerial() {
  static char buf[64];
  static uint8_t len = 0;
  static bool tooLong = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (tooLong) Serial.println("ERR toolong");
      else if (len) { buf[len] = '\0'; handleLine(buf); }
      len = 0;
      tooLong = false;
    }
    else if (c != '\r') {
      if (len < sizeof(buf) - 1) buf[len++] = c;
      else tooLong = true;
    }
  }
}

// ---------- Main ----------

void setup() {
  Serial.begin(115200);

  pinMode(TLC_DIN, OUTPUT);
  pinMode(TLC_CLK, OUTPUT);
  pinMode(TLC_LAT, OUTPUT);
  strip.begin();
  strip.setBrightness(STRIP_BRIGHTNESS);
  allOff();

  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    bool pressed = digitalRead(BTN_PINS[i]) == LOW;
    btns[i] = { pressed, pressed, millis() };
  }
  initAnalogs(SLIDER_PINS, sliders, NUM_SLIDERS);
  initAnalogs(PROX_PINS, prox, NUM_PROX);

  Serial.printf("READY %d\n", PROTOCOL_VERSION);
}

void loop() {
  updateButtons();
  updateAnalogs("SLIDER", SLIDER_PINS, sliders, NUM_SLIDERS, SLIDER_DEADBAND, SLIDER_INTERVAL_MS);
  updateAnalogs("PROX", PROX_PINS, prox, NUM_PROX, PROX_DEADBAND, PROX_INTERVAL_MS);
  updateLeds();
  updateSerial();
}
