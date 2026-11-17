// Hardware test for the intermediate build: Teensy 4.1.
// Wiring: see ../teensy41_intermediate_build_wiring.png
// Protocol: follows /pi-teensy-protocol.md (Sam's Pi/Teensy serial protocol, version 1).
//
// USB serial, one ASCII line per message, ending in \n. Max 63 characters.
// Type commands in the Arduino Serial Monitor (line ending "Newline") or from the Pi.
// Commands are uppercase; lowercase is also accepted for typing by hand.
//
// Device IDs (0-based, the index into the pin arrays below):
//   BTN   0-3   buttons Btn 1-4 on pins 2-5
//   MOTOR 0     DC motor on the DRV8871 (pins 6, 7)
//   ACT   0     linear actuator on the BTS7960 (pins 8, 9)
//   LED   0-3   RGB button LEDs on the TLC5947 (OUT0-11, 3 channels each)
//   STRIP       WS2812B strip on pin 33 (through the 74AHCT125)
//
// Teensy -> Pi:
//   READY 1                  booted, every output in its safe state
//   BTN <id> <0|1>           button released / pressed (debounced, only on change)
//   PONG                     reply to PING
//   OK <command>             command accepted (the command is echoed back)
//   DONE <kind> <id>         a timed run finished, e.g. DONE MOTOR 0
//   ERR <reason>             one word: unknown, usage, range, toolong, aborted
//
// Pi -> Teensy:
//   PING                     Teensy replies PONG
//   SNAPSHOT                 Teensy sends BTN <id> <v> for every button
//   MOTOR <id> <speed> <ms>  speed -255..255 (negative = other direction), ms capped at 10000
//   ACT <id> <speed> <ms>    same, for the linear actuator
//   LED <id> <r> <g> <b>     button LED colour, 0-255 each
//   STRIP <r> <g> <b>        fill the whole strip with one colour (STRIP 0 0 0 = off)
//   STRIP WAVE               effect: blue strip with a red/purple wave running along it
//   STOP                     everything to a safe state now
//   TEST                     test-only: run every device once, then DONE TEST 0
//                            (send any line to abort: ERR aborted)

#include <Adafruit_NeoPixel.h>

// ---------- Pins (from the wiring diagram) ----------
const uint8_t BTN_PINS[] = {2, 3, 4, 5};  // Btn 1..4 -> IDs 0..3; other leg of each button to GND
const uint8_t NUM_BTNS = sizeof(BTN_PINS) / sizeof(BTN_PINS[0]);
const uint8_t DRV_IN1 = 6;
const uint8_t DRV_IN2 = 7;
const uint8_t BTS_RPWM = 8;
const uint8_t BTS_LPWM = 9;
const uint8_t TLC_DIN = 11;
const uint8_t TLC_CLK = 13;
const uint8_t TLC_LAT = 10;
const uint8_t STRIP_PIN = 33;

// TLC5947 channels for each button LED, in R, G, B order.
// If a colour comes out wrong, swap the numbers here to match the wiring.
const uint8_t LED_CHANNELS[][3] = {
  {0, 1, 2},   // LED 0 (Btn 1)
  {3, 4, 5},   // LED 1 (Btn 2)
  {6, 7, 8},   // LED 2 (Btn 3)
  {9, 10, 11}, // LED 3 (Btn 4)
};
const uint8_t NUM_LEDS = sizeof(LED_CHANNELS) / sizeof(LED_CHANNELS[0]);

// ---------- Settings ----------
const int STRIP_LEN = 212;                  // number of LEDs on the strip
const int STRIP_BRIGHTNESS = 50;            // 0..255, kept low so the 5V supply is not overloaded
const int WAVE_WIDTH = 3;                   // how many red LEDs are in the moving wave
const unsigned long WAVE_STEP_MS = 10;      // wave moves one LED every this many ms (smaller = faster)
const int TLC_CHANNELS = 24;                // one TLC5947 has 24 outputs
const unsigned long MAX_RUN_MS = 10000;     // safety: no motor/actuator run lasts longer than this
const unsigned long BTN_DEBOUNCE_MS = 20;
const uint8_t MAX_LINE = 63;                // longer lines are discarded with ERR toolong

// ---------- Motors (MOTOR 0 and ACT 0) ----------

struct Motor {
  const char *kind;  // "MOTOR" or "ACT", used in DONE messages
  uint8_t id;
  uint8_t pinA;
  uint8_t pinB;
  bool isDrv;        // DRV8871 and BTS7960 brake differently
  bool running;
  unsigned long startMs;
  unsigned long runMs;
};

Motor motor = {"MOTOR", 0, DRV_IN1, DRV_IN2, true, false, 0, 0};
Motor act = {"ACT", 0, BTS_RPWM, BTS_LPWM, false, false, 0, 0};

// Set a pin to a duty cycle 0..255. 0 and 255 use plain digital LOW/HIGH so the pin is fully off/on.
// (On Teensy, pinMode() is needed to switch a pin back from PWM to digital.)
void setPin(uint8_t pin, int duty) {
  if (duty <= 0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else if (duty >= 255) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  } else {
    analogWrite(pin, duty);
  }
}

// speed: -255..255, 0 = brake.
void applySpeed(Motor &m, int speed) {
  speed = constrain(speed, -255, 255);
  if (m.isDrv) {
    // DRV8871: one input HIGH, PWM the other. Both HIGH = brake.
    if (speed >= 0) {
      setPin(m.pinA, 255);
      setPin(m.pinB, 255 - speed);
    } else {
      setPin(m.pinA, 255 + speed);
      setPin(m.pinB, 255);
    }
  } else {
    // BTS7960: PWM one side, the other LOW. Both LOW = brake.
    if (speed >= 0) {
      setPin(m.pinA, speed);
      setPin(m.pinB, 0);
    } else {
      setPin(m.pinA, 0);
      setPin(m.pinB, -speed);
    }
  }
}

void stopMotor(Motor &m) {
  applySpeed(m, 0);
  m.running = false;
}

// A new run replaces the one in progress.
void runMotor(Motor &m, int speed, unsigned long ms) {
  if (speed == 0 || ms == 0) {
    stopMotor(m);
    return;
  }
  applySpeed(m, speed);
  m.running = true;
  m.startMs = millis();
  m.runMs = min(ms, MAX_RUN_MS);
}

void updateMotor(Motor &m) {
  if (m.running && millis() - m.startMs >= m.runMs) {
    stopMotor(m);
    Serial.printf("DONE %s %d\n", m.kind, m.id);
  }
}

// ---------- TLC5947 (button LEDs) ----------

uint16_t tlcValues[TLC_CHANNELS];  // 0..4095 per channel

// Shift all 24 channels out (channel 23 first, 12 bits each, MSB first), then latch.
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

// Set one button LED. r, g, b: 0..255 (scaled to the TLC's 0..4095).
void setLed(uint8_t id, int r, int g, int b) {
  tlcValues[LED_CHANNELS[id][0]] = r * 4095 / 255;
  tlcValues[LED_CHANNELS[id][1]] = g * 4095 / 255;
  tlcValues[LED_CHANNELS[id][2]] = b * 4095 / 255;
  tlcWrite();
}

void ledsOff() {
  for (int ch = 0; ch < TLC_CHANNELS; ch++) {
    tlcValues[ch] = 0;
  }
  tlcWrite();
}

// ---------- WS2812B strip ----------

Adafruit_NeoPixel strip(STRIP_LEN, STRIP_PIN, NEO_GRB + NEO_KHZ800);
bool stripWave = false;
int wavePos = 0;  // index of the first red LED
unsigned long stripLastStepMs = 0;

void stripFill(uint8_t r, uint8_t g, uint8_t b) {
  stripWave = false;
  strip.fill(strip.Color(r, g, b));
  strip.show();
}

// Wave on a blue background: purple, WAVE_WIDTH red LEDs, purple.
// It runs to the end of the strip and wraps around to the start, forever.
void updateStrip() {
  if (!stripWave || millis() - stripLastStepMs < WAVE_STEP_MS) return;
  stripLastStepMs = millis();

  strip.fill(strip.Color(0, 0, 255));  // background: blue
  // purple just before the first red (wraps to the strip's end when wavePos is 0)
  strip.setPixelColor((wavePos - 1 + STRIP_LEN) % STRIP_LEN, strip.Color(150, 0, 255));
  for (int i = 0; i < WAVE_WIDTH; i++) {
    strip.setPixelColor((wavePos + i) % STRIP_LEN, strip.Color(255, 0, 0));  // red
  }
  // purple just after the last red
  strip.setPixelColor((wavePos + WAVE_WIDTH) % STRIP_LEN, strip.Color(150, 0, 255));
  strip.show();
  wavePos++;
  if (wavePos >= STRIP_LEN) {
    wavePos = 0;
  }
}

// ---------- Buttons ----------

struct Button {
  bool stable;       // debounced state, true = pressed
  bool lastRaw;      // raw reading from the previous pass
  uint32_t changedMs;  // when the raw reading last changed
};
Button btns[NUM_BTNS];

void sendButton(uint8_t id) {
  Serial.printf("BTN %d %d\n", id, btns[id].stable);
}

void updateButtons() {
  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    bool raw = (digitalRead(BTN_PINS[i]) == LOW);  // pressed = connected to GND
    if (raw != btns[i].lastRaw) {
      // the pin just changed: restart the settle timer
      btns[i].lastRaw = raw;
      btns[i].changedMs = millis();
    } else if (raw != btns[i].stable && millis() - btns[i].changedMs >= BTN_DEBOUNCE_MS) {
      // it has held the new value long enough: accept it and report
      btns[i].stable = raw;
      sendButton(i);
    }
  }
}

// ---------- Safe state and test ----------

void allSafe() {
  stopMotor(motor);
  stopMotor(act);
  ledsOff();
  stripFill(0, 0, 0);
}

// Wait while keeping motors and buttons running. Returns false if a line arrives (abort).
bool waitMs(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    updateButtons();
    updateMotor(motor);
    updateMotor(act);
    if (Serial.available() > 0) {
      while (Serial.available() > 0) Serial.read();
      return false;
    }
  }
  return true;
}

// Run every device once, one after another.
bool runTest() {
  allSafe();

  // MOTOR 0: forward 1 s, then backward 1 s
  runMotor(motor, 200, 1000);
  if (!waitMs(1300)) return false;
  runMotor(motor, -200, 1000);
  if (!waitMs(1300)) return false;

  // ACT 0: extend 2 s, then retract 2 s
  runMotor(act, 255, 2000);
  if (!waitMs(2300)) return false;
  runMotor(act, -255, 2000);
  if (!waitMs(2300)) return false;

  // LED 0-3: red, green, blue on each
  for (uint8_t id = 0; id < NUM_LEDS; id++) {
    setLed(id, 255, 0, 0);
    if (!waitMs(250)) return false;
    setLed(id, 0, 255, 0);
    if (!waitMs(250)) return false;
    setLed(id, 0, 0, 255);
    if (!waitMs(250)) return false;
    setLed(id, 0, 0, 0);
  }

  // STRIP: red, green, blue
  stripFill(255, 0, 0);
  if (!waitMs(700)) return false;
  stripFill(0, 255, 0);
  if (!waitMs(700)) return false;
  stripFill(0, 0, 255);
  if (!waitMs(700)) return false;

  allSafe();
  return true;
}

// ---------- Serial commands ----------

// True if s is a whole (optionally negative) integer.
bool isInt(const char *s) {
  if (s == nullptr || *s == '\0') return false;
  if (*s == '-') s++;
  if (*s == '\0') return false;
  for (; *s; s++) {
    if (!isdigit(*s)) return false;
  }
  return true;
}

// True if all n tokens are integers.
bool allInts(char *tok[], int n) {
  for (int i = 0; i < n; i++) {
    if (!isInt(tok[i])) return false;
  }
  return true;
}

bool inRange(int v, int lo, int hi) {
  return v >= lo && v <= hi;
}

void handleMotor(Motor &m, char *tok[], const char *echo) {
  if (!allInts(tok, 3)) {
    Serial.println("ERR usage");
    return;
  }
  int id = atoi(tok[0]);
  int speed = atoi(tok[1]);
  long ms = atol(tok[2]);
  if (id != m.id || !inRange(speed, -255, 255) || ms < 0) {
    Serial.println("ERR range");
    return;
  }
  runMotor(m, speed, ms);
  Serial.printf("OK %s\n", echo);
}

void handleLine(char *line) {
  for (char *p = line; *p; p++) {
    *p = toupper(*p);
  }
  char echo[MAX_LINE + 1];
  strcpy(echo, line);

  // Split into the command and up to 4 arguments.
  char *cmd = strtok(line, " ");
  char *tok[4];
  for (int i = 0; i < 4; i++) {
    tok[i] = strtok(nullptr, " ");
  }
  if (cmd == nullptr) return;

  if (strcmp(cmd, "PING") == 0) {
    Serial.println("PONG");
  } else if (strcmp(cmd, "SNAPSHOT") == 0) {
    for (uint8_t i = 0; i < NUM_BTNS; i++) {
      sendButton(i);
    }
  } else if (strcmp(cmd, "MOTOR") == 0) {
    handleMotor(motor, tok, echo);
  } else if (strcmp(cmd, "ACT") == 0) {
    handleMotor(act, tok, echo);
  } else if (strcmp(cmd, "LED") == 0) {
    if (!allInts(tok, 4)) {
      Serial.println("ERR usage");
      return;
    }
    int id = atoi(tok[0]);
    int r = atoi(tok[1]);
    int g = atoi(tok[2]);
    int b = atoi(tok[3]);
    if (!inRange(id, 0, NUM_LEDS - 1) || !inRange(r, 0, 255) || !inRange(g, 0, 255) || !inRange(b, 0, 255)) {
      Serial.println("ERR range");
      return;
    }
    setLed(id, r, g, b);
    Serial.printf("OK %s\n", echo);
  } else if (strcmp(cmd, "STRIP") == 0) {
    if (tok[0] != nullptr && strcmp(tok[0], "WAVE") == 0) {
      stripWave = true;
      wavePos = 0;
      Serial.printf("OK %s\n", echo);
      return;
    }
    if (!allInts(tok, 3)) {
      Serial.println("ERR usage");
      return;
    }
    int r = atoi(tok[0]);
    int g = atoi(tok[1]);
    int b = atoi(tok[2]);
    if (!inRange(r, 0, 255) || !inRange(g, 0, 255) || !inRange(b, 0, 255)) {
      Serial.println("ERR range");
      return;
    }
    stripFill(r, g, b);
    Serial.printf("OK %s\n", echo);
  } else if (strcmp(cmd, "STOP") == 0) {
    allSafe();
    Serial.println("OK STOP");
  } else if (strcmp(cmd, "TEST") == 0) {
    Serial.println("OK TEST");
    if (runTest()) {
      Serial.println("DONE TEST 0");
    } else {
      allSafe();
      Serial.println("ERR aborted");
    }
  } else {
    Serial.println("ERR unknown");
  }
}

// Collect characters until \n (\r is ignored), then handle the line.
void updateSerial() {
  static char buf[MAX_LINE + 1];
  static uint8_t len = 0;
  static bool tooLong = false;
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      if (tooLong) {
        Serial.println("ERR toolong");
      } else if (len > 0) {
        buf[len] = '\0';
        handleLine(buf);
      }
      len = 0;
      tooLong = false;
    } else if (c != '\r') {
      if (len < MAX_LINE) {
        buf[len++] = c;
      } else {
        tooLong = true;
      }
    }
  }
}

// ---------- Main ----------

void setup() {
  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }
  pinMode(TLC_DIN, OUTPUT);
  pinMode(TLC_CLK, OUTPUT);
  pinMode(TLC_LAT, OUTPUT);

  strip.begin();
  strip.setBrightness(STRIP_BRIGHTNESS);

  allSafe();

  Serial.begin(115200);  // Teensy USB serial ignores the baud rate
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) {
    // wait up to 2 s for the serial terminal so READY is not missed
  }

  // Start from the buttons' current state, so nothing is reported at boot.
  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    bool pressed = (digitalRead(BTN_PINS[i]) == LOW);
    btns[i] = {pressed, pressed, millis()};
  }

  Serial.println("READY 1");
}

void loop() {
  updateSerial();
  updateButtons();
  updateMotor(motor);
  updateMotor(act);
  updateStrip();
}
