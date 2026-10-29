// Host-side mock of the Arduino core API, used to unit-test firmware with g++.
// Header-only (C++17 inline variables). Not production code.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

typedef uint8_t byte;

#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Arduino Nano analog pins used as digital I/O.
constexpr uint8_t A0 = 14;
constexpr uint8_t A1 = 15;
constexpr uint8_t A2 = 16;
constexpr uint8_t A3 = 17;
constexpr uint8_t A4 = 18;
constexpr uint8_t A5 = 19;

constexpr int MOCK_NUM_PINS = 20;      // D0..D13 + A0..A5
constexpr uint8_t MOCK_MODE_UNSET = 0xFF;

// ---- History of every pinMode / digitalWrite call, in order ----
enum MockEventKind : uint8_t { MOCK_EV_PINMODE = 0, MOCK_EV_WRITE = 1 };
struct MockEvent {
  MockEventKind kind;
  uint8_t pin;
  uint8_t value;   // mode for PINMODE, level for WRITE
  uint32_t time;   // mock millis at the time of the call
};

// ---- Simulated hardware state ----
inline uint8_t mock_mode[MOCK_NUM_PINS];
inline uint8_t mock_out[MOCK_NUM_PINS];      // output latch level
inline uint8_t mock_in[MOCK_NUM_PINS];       // externally driven level (tests)
inline bool mock_in_set[MOCK_NUM_PINS];      // test explicitly drove this pin
inline std::vector<MockEvent> mock_history;
inline uint32_t mock_now = 0;
inline unsigned mock_delay_calls = 0;
// Pins ever touched by firmware (mode/read/write). Survives mock_reset();
// cleared only by mock_clear_touched(). Index 63 = any out-of-range pin.
inline bool mock_touched[64];

inline void mock_touch(int pin) {
  if (pin < 0 || pin >= MOCK_NUM_PINS) mock_touched[63] = true;
  else mock_touched[pin] = true;
}

inline void mock_clear_touched() { memset(mock_touched, 0, sizeof(mock_touched)); }

// Reset pins, history and delay counter. Does NOT touch the clock.
inline void mock_reset() {
  for (int i = 0; i < MOCK_NUM_PINS; i++) {
    mock_mode[i] = MOCK_MODE_UNSET;
    mock_out[i] = LOW;
    mock_in[i] = HIGH;
    mock_in_set[i] = false;
  }
  mock_history.clear();
  mock_delay_calls = 0;
}

// ---- Clock control ----
inline void mock_set_millis(uint32_t v) { mock_now = v; }
inline void mock_advance(uint32_t ms) { mock_now += ms; }  // wraps at 2^32 like hardware
inline uint32_t mock_millis_now() { return mock_now; }

// ---- Test-side pin access ----
inline void mock_set_input(uint8_t pin, uint8_t level) {
  if (pin < MOCK_NUM_PINS) { mock_in[pin] = level; mock_in_set[pin] = true; }
}
inline uint8_t mock_get_output(uint8_t pin) { return pin < MOCK_NUM_PINS ? mock_out[pin] : 0; }
inline uint8_t mock_get_mode(uint8_t pin) { return pin < MOCK_NUM_PINS ? mock_mode[pin] : MOCK_MODE_UNSET; }

// ---- Arduino API ----
inline void pinMode(uint8_t pin, uint8_t mode) {
  mock_touch(pin);
  if (pin < MOCK_NUM_PINS) mock_mode[pin] = mode;
  mock_history.push_back({MOCK_EV_PINMODE, pin, mode, mock_now});
}

inline void digitalWrite(uint8_t pin, uint8_t val) {
  mock_touch(pin);
  uint8_t v = val ? HIGH : LOW;
  if (pin < MOCK_NUM_PINS) mock_out[pin] = v;
  mock_history.push_back({MOCK_EV_WRITE, pin, v, mock_now});
}

inline int digitalRead(uint8_t pin) {
  mock_touch(pin);
  if (pin >= MOCK_NUM_PINS) return LOW;
  switch (mock_mode[pin]) {
    case OUTPUT: return mock_out[pin];
    case INPUT_PULLUP: return mock_in_set[pin] ? mock_in[pin] : HIGH;
    default: return mock_in_set[pin] ? mock_in[pin] : LOW;  // floating input
  }
}

inline uint32_t millis() { return mock_now; }

// Blocking delays must never be used; calls are counted, time does not advance.
inline void delay(uint32_t) { mock_delay_calls++; }
inline void delayMicroseconds(uint32_t) { mock_delay_calls++; }

// On LP64 hosts `unsigned long` is 64-bit, unlike the AVR's 32-bit. Make firmware
// code written with `unsigned long` behave as on the Nano so rollover is real.
// (Must stay last; all standard headers are already included above.)
#if defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8
#define long int
#endif
