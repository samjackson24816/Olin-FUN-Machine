#include "pins.h"
struct Button { bool stable; bool lastRaw; uint32_t changedMs; };
Button btns[NUM_BTNS];
void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    bool pressed = digitalRead(BTN_PINS[i]) == LOW;
    btns[i] = { pressed, pressed, millis() };
  }
  Serial.println("READY 1");
}



void updateButtons() {
  for (uint8_t i = 0; i < NUM_BTNS; i++) {
    bool raw = (digitalRead(BTN_PINS[i]) == LOW);
    if (raw != btns[i].lastRaw) {
      // the pin just changed: restart the settle timer
    } else if (raw != btns[i].stable &&
               millis() - btns[i].changedMs >= BTN_DEBOUNCE_MS) {
      // it has held the new value long enough: accept it and report
    }
  }
}


void sendButton(uint8_t id) {
  Serial.printf("BTN %d %d\n", id, btns[id].stable);
}


void handleLine(const char* line) {
  if (strcmp(line, "PING") == 0)          { Serial.println("PONG"); }
  else if (strcmp(line, "SNAPSHOT") == 0) { /* loop i: sendButton(i) */ }
  else                                    { Serial.println("ERR unknown"); }
}


void updateSerial() {
  static char buf[64];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { buf[len] = '\0'; if (len) handleLine(buf); len = 0; }
    else if (c != '\r' && len < sizeof(buf) - 1) { buf[len++] = c; }
  }
}



void loop() {
  updateButtons();
  updateSerial();
}
