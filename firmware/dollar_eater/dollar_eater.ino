// Dollar Eater firmware: Arduino Uno R3 + DRV8871 motor driver.
//
// The Arduino does only two things:
//   1. Watches the bill switch and reports when it is pressed or released.
//   2. Runs the motor when it is told to, for a fixed time.
// It never decides on its own when to eat (unless AUTO_EAT is true).
//
// Serial protocol: 115200 baud, one message per line.
//   Pi -> Arduino:
//     RUN <speed> <ms>   speed -255..255 (> 0 = eat, < 0 = spit out), then stop after ms
//     STOP               stop now
//   Arduino -> Pi:
//     READY              finished booting
//     SWITCH 1           bill switch pressed  (a bill is going in)
//     SWITCH 0           bill switch released
//     OK <command>       command accepted
//     DONE               a timed run finished, motor stopped
//     ERR <text>         command not understood

// ---------- Pins ----------
const int SWITCH_PIN = 2;   // bill switch: one wire here, the other wire to GND
const int IN1_PIN = 9;      // DRV8871 IN1
const int IN2_PIN = 10;     // DRV8871 IN2

// ---------- Settings ----------
const int PRESSED_LEVEL = LOW;           // what SWITCH_PIN reads when pressed (LOW for a normally-open switch)
const unsigned long DEBOUNCE_MS = 20;    // switch must be stable this long to count
const unsigned long MAX_RUN_MS = 10000;  // safety: no run lasts longer than this

// Standalone test without the Pi: set to true and the Arduino eats by itself.
const bool AUTO_EAT = false;
const int AUTO_EAT_SPEED = 255;
const unsigned long AUTO_EAT_MS = 3000;

// ---------- State ----------
bool switchPressed = false;      // debounced switch state
bool lastReading = false;        // raw reading from the previous loop
unsigned long lastChangeMs = 0;  // when the raw reading last changed

bool motorRunning = false;
unsigned long motorStartMs = 0;
unsigned long motorRunMs = 0;

char line[32];       // the serial command being received
byte lineLength = 0;

// ---------- Motor ----------

// Drive the DRV8871. speed: -255..255, 0 = brake.
// One input stays HIGH and the other is PWM'd ("slow decay" mode in the datasheet).
// When both inputs are HIGH the driver brakes, so speed 0 stops the motor quickly.
void setMotor(int speed) {
  speed = constrain(speed, -255, 255);
  if (speed >= 0) {
    digitalWrite(IN1_PIN, HIGH);
    analogWrite(IN2_PIN, 255 - speed);
  } else {
    analogWrite(IN1_PIN, 255 + speed);
    digitalWrite(IN2_PIN, HIGH);
  }
}

void stopMotor() {
  setMotor(0);
  motorRunning = false;
}

void runMotor(int speed, unsigned long ms) {
  if (speed == 0 || ms == 0) {
    stopMotor();
    return;
  }
  setMotor(speed);
  motorRunning = true;
  motorStartMs = millis();
  motorRunMs = min(ms, MAX_RUN_MS);
}

// Stop the motor when its time is up.
void updateMotor() {
  if (motorRunning && millis() - motorStartMs >= motorRunMs) {
    stopMotor();
    Serial.println("DONE");
  }
}

// ---------- Switch ----------

void updateSwitch() {
  bool reading = (digitalRead(SWITCH_PIN) == PRESSED_LEVEL);
  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = millis();
  }

  bool stable = (millis() - lastChangeMs >= DEBOUNCE_MS);
  if (stable && reading != switchPressed) {
    switchPressed = reading;
    Serial.println(switchPressed ? "SWITCH 1" : "SWITCH 0");

    if (switchPressed && AUTO_EAT) {
      runMotor(AUTO_EAT_SPEED, AUTO_EAT_MS);
    }
  }
}

// ---------- Serial commands ----------

void handleCommand(const char *command) {
  int speed;
  unsigned long ms;

  if (sscanf(command, "RUN %d %lu", &speed, &ms) == 2) {
    runMotor(speed, ms);
    Serial.print("OK ");
    Serial.println(command);
  } else if (strcmp(command, "STOP") == 0) {
    stopMotor();
    Serial.println("OK STOP");
  } else {
    Serial.print("ERR unknown command: ");
    Serial.println(command);
  }
}

// Collect characters until end of line, then run the command.
void updateSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLength > 0) {
        line[lineLength] = '\0';
        handleCommand(line);
        lineLength = 0;
      }
    } else if (lineLength < sizeof(line) - 1) {
      line[lineLength] = c;
      lineLength++;
    }
  }
}

// ---------- Main ----------

void setup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  stopMotor();

  Serial.begin(115200);
  Serial.println("READY");
}

void loop() {
  updateSerial();
  updateSwitch();
  updateMotor();
}
