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

void setup() {
  const uint8_t inputPins[] = {
    BTN_SWEET_1, BTN_SWEET_2, BTN_SALTY_1, BTN_SALTY_2,
    BTN_SOUR_1, BTN_SOUR_2, BTN_BITTER_1, BTN_BITTER_2
  };

  for (uint8_t i = 0; i < 8; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
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

  // Read button inputs (active LOW)
  bool sweetActive  = (digitalRead(BTN_SWEET_1) == LOW)  || (digitalRead(BTN_SWEET_2) == LOW);
  bool saltyActive  = (digitalRead(BTN_SALTY_1) == LOW)  || (digitalRead(BTN_SALTY_2) == LOW);
  bool sourActive   = (digitalRead(BTN_SOUR_1) == LOW)   || (digitalRead(BTN_SOUR_2) == LOW);
  bool bitterActive = (digitalRead(BTN_BITTER_1) == LOW) || (digitalRead(BTN_BITTER_2) == LOW);

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
