#include <Arduino.h>

// Project Paladar - Production Demonstrator Firmware
// Platform: Arduino Nano (ATmega328P)

// Input Pin Definitions
const uint8_t BTN_SWEET_1  = 2;
const uint8_t BTN_SWEET_2  = 3;
const uint8_t BTN_SALTY_1  = 4;
const uint8_t BTN_SALTY_2  = 5;
const uint8_t BTN_SOUR_1   = 6;
const uint8_t BTN_SOUR_2   = 7;
const uint8_t BTN_BITTER_1 = 8;
const uint8_t BTN_BITTER_2 = 9;

// Output Pin Definitions
const uint8_t LED_SWEET    = 10;
const uint8_t LED_SALTY    = 11;
const uint8_t LED_SOUR     = 12;
const uint8_t LED_BITTER   = A0;
const uint8_t RELAY_PIN    = A1;

// Actuator & Interface Parameters
const unsigned long HAPTIC_PULSE_MS = 200;
const bool RELAY_ACTIVE_LEVEL       = LOW;  // Active-LOW optocoupled trigger

// State Variables
unsigned long motorStartTime = 0;
bool motorRunning            = false;
bool lastReflexInputState    = false;

// Debounce (REQ-FW-02): accept change only after stable for DEBOUNCE_MS
const unsigned long DEBOUNCE_MS = 30;
const uint8_t NUM_SWITCHES = 8;
const uint8_t INPUT_PINS[NUM_SWITCHES] = {
  BTN_SWEET_1, BTN_SWEET_2, BTN_SALTY_1, BTN_SALTY_2,
  BTN_SOUR_1, BTN_SOUR_2, BTN_BITTER_1, BTN_BITTER_2
};
bool rawLast[NUM_SWITCHES];              // true = pressed (LOW)
bool debounced[NUM_SWITCHES];            // true = pressed (LOW)
unsigned long lastChange[NUM_SWITCHES];

void setup() {
  for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
    pinMode(INPUT_PINS[i], INPUT_PULLUP);
  }
  unsigned long now = millis();
  for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
    rawLast[i] = (digitalRead(INPUT_PINS[i]) == LOW);
    debounced[i] = false;  // start released
    lastChange[i] = now;
  }

  pinMode(LED_SWEET, OUTPUT);
  pinMode(LED_SALTY, OUTPUT);
  pinMode(LED_SOUR, OUTPUT);
  pinMode(LED_BITTER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Initial Safe States
  digitalWrite(LED_SWEET, LOW);
  digitalWrite(LED_SALTY, LOW);
  digitalWrite(LED_SOUR, LOW);
  digitalWrite(LED_BITTER, LOW);
  digitalWrite(RELAY_PIN, !RELAY_ACTIVE_LEVEL);
}

void loop() {
  unsigned long currentMillis = millis();

  // Debounce each switch (active LOW), non-blocking
  for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
    bool raw = (digitalRead(INPUT_PINS[i]) == LOW);
    if (raw != rawLast[i]) {
      rawLast[i] = raw;
      lastChange[i] = currentMillis;
    }
    if (raw != debounced[i] && (currentMillis - lastChange[i] >= DEBOUNCE_MS)) {
      debounced[i] = raw;
    }
  }

  // Taste active if either debounced switch is pressed
  bool sweetActive  = debounced[0] || debounced[1];
  bool saltyActive  = debounced[2] || debounced[3];
  bool sourActive   = debounced[4] || debounced[5];
  bool bitterActive = debounced[6] || debounced[7];

  // Drive LED groups
  digitalWrite(LED_SWEET, sweetActive ? HIGH : LOW);
  digitalWrite(LED_SALTY, saltyActive ? HIGH : LOW);
  digitalWrite(LED_SOUR, sourActive ? HIGH : LOW);
  digitalWrite(LED_BITTER, bitterActive ? HIGH : LOW);

  // Evaluate reflex activation (Sour or Bitter)
  bool reflexActive = (sourActive || bitterActive);

  // Non-blocking trigger on rising edge
  if (reflexActive && !lastReflexInputState && !motorRunning) {
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LEVEL);
    motorStartTime = currentMillis;
    motorRunning = true;
  }
  lastReflexInputState = reflexActive;

  // Automated pulse cutoff
  if (motorRunning && (currentMillis - motorStartTime >= HAPTIC_PULSE_MS)) {
    digitalWrite(RELAY_PIN, !RELAY_ACTIVE_LEVEL);
    motorRunning = false;
  }
}
