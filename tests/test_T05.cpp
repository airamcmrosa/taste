// T-05 tests: safe boot state of relay and LED outputs. Host build against tests/mock/Arduino.h.
// Covers REQ-SAF-01 (LED pins LOW from the moment they become outputs),
// REQ-SAF-02 (relay pin A1 HIGH from the moment it becomes an output),
// REQ-REL-02 (A1 LOW = relay energized; HIGH = de-energized).
// Run from the repo root.
//
// NOTE: do not use the `long` keyword here: the mock remaps it to 32-bit.
#include <Arduino.h>
#include "../firmware/paladar/paladar.ino"

static int g_tests = 0;
static int g_failed = 0;

static void check(bool cond, const char* req, const char* msg) {
  g_tests++;
  if (!cond) {
    g_failed++;
    printf("FAIL [%s] %s\n", req, msg);
  } else {
    printf("ok   [%s] %s\n", req, msg);
  }
}

static const uint8_t LEDP[4] = {10, 11, 12, A0};
static const char* LEDN[4] = {"D10", "D11", "D12", "A0"};
static const uint8_t RLY = A1;

// ---------- history helpers ----------
static int first_event(uint8_t kind, uint8_t pin) {
  for (size_t i = 0; i < mock_history.size(); i++)
    if (mock_history[i].kind == kind && mock_history[i].pin == pin) return (int)i;
  return -1;
}
static int first_output_mode(uint8_t pin) {
  for (size_t i = 0; i < mock_history.size(); i++)
    if (mock_history[i].kind == MOCK_EV_PINMODE && mock_history[i].pin == pin &&
        mock_history[i].value == OUTPUT)
      return (int)i;
  return -1;
}
static bool ever_written(uint8_t pin, uint8_t level) {
  for (size_t i = 0; i < mock_history.size(); i++)
    if (mock_history[i].kind == MOCK_EV_WRITE && mock_history[i].pin == pin &&
        mock_history[i].value == level)
      return true;
  return false;
}
static bool first_write_is(uint8_t pin, uint8_t level) {
  int w = first_event(MOCK_EV_WRITE, pin);
  return w >= 0 && mock_history[w].value == level;
}
// A write of `level` happens before pinMode(pin, OUTPUT).
static bool preload_before_output(uint8_t pin, uint8_t level) {
  int m = first_output_mode(pin);
  if (m < 0) return false;
  for (int i = 0; i < m; i++)
    if (mock_history[i].kind == MOCK_EV_WRITE && mock_history[i].pin == pin &&
        mock_history[i].value == level)
      return true;
  return false;
}

static void run_ms(unsigned n) {
  for (; n; --n) {
    loop();
    mock_advance(1);
  }
}

static void boot(int held_pin /* -1 none */) {
  mock_reset();
  mock_set_millis(0);
  if (held_pin >= 0) mock_set_input((uint8_t)held_pin, LOW);
  setup();
}

// ---------- scenario checks ----------
static void check_boot_safety(const char* tag) {
  char msg[128];

  // Relay: REQ-SAF-02
  snprintf(msg, sizeof msg, "%s: A1 configured as OUTPUT", tag);
  check(first_output_mode(RLY) >= 0 && mock_get_mode(RLY) == OUTPUT, "REQ-SAF-02", msg);
  snprintf(msg, sizeof msg, "%s: first digitalWrite(A1) is HIGH", tag);
  check(first_write_is(RLY, HIGH), "REQ-SAF-02", msg);
  snprintf(msg, sizeof msg, "%s: A1 HIGH written before pinMode(A1, OUTPUT)", tag);
  check(preload_before_output(RLY, HIGH), "REQ-SAF-02", msg);
  snprintf(msg, sizeof msg, "%s: A1 never written LOW during setup()", tag);
  check(!ever_written(RLY, LOW), "REQ-SAF-02", msg);
  snprintf(msg, sizeof msg, "%s: A1 reads HIGH after setup()", tag);
  check(mock_get_output(RLY) == HIGH, "REQ-SAF-02", msg);

  // LEDs: REQ-SAF-01
  for (int i = 0; i < 4; i++) {
    uint8_t p = LEDP[i];
    snprintf(msg, sizeof msg, "%s: %s configured as OUTPUT", tag, LEDN[i]);
    check(first_output_mode(p) >= 0 && mock_get_mode(p) == OUTPUT, "REQ-SAF-01", msg);
    snprintf(msg, sizeof msg, "%s: %s never written HIGH during setup()", tag, LEDN[i]);
    check(!ever_written(p, HIGH), "REQ-SAF-01", msg);
    snprintf(msg, sizeof msg, "%s: %s LOW written before pinMode(OUTPUT)", tag, LEDN[i]);
    check(preload_before_output(p, LOW), "REQ-SAF-01", msg);
    snprintf(msg, sizeof msg, "%s: %s reads LOW after setup()", tag, LEDN[i]);
    check(mock_get_output(p) == LOW, "REQ-SAF-01", msg);
  }

  // REQ-REL-02: not energized at boot.
  snprintf(msg, sizeof msg, "%s: relay not energized at boot (A1 not LOW)", tag);
  check(mock_get_output(RLY) != LOW, "REQ-REL-02", msg);
}

static void check_idle_400ms(const char* tag) {
  char msg[128];
  bool relay_low = false, led_high = false;
  for (int k = 0; k < 400; k++) {
    loop();
    if (mock_get_output(RLY) == LOW) relay_low = true;
    for (int i = 0; i < 4; i++)
      if (mock_get_output(LEDP[i]) == HIGH) led_high = true;
    mock_advance(1);
  }
  snprintf(msg, sizeof msg, "%s: A1 never LOW in 400 ms of loop() with no input", tag);
  check(!relay_low && !ever_written(RLY, LOW), "REQ-SAF-02", msg);
  snprintf(msg, sizeof msg, "%s: LEDs never HIGH in 400 ms with no input", tag);
  check(!led_high && !ever_written(LEDP[0], HIGH) && !ever_written(LEDP[1], HIGH) &&
            !ever_written(LEDP[2], HIGH) && !ever_written(LEDP[3], HIGH),
        "REQ-SAF-01", msg);
  snprintf(msg, sizeof msg, "%s: relay de-energized (A1 HIGH) when idle", tag);
  check(mock_get_output(RLY) == HIGH, "REQ-REL-02", msg);
}

static void test_cold_boot() {
  boot(-1);
  check_boot_safety("cold boot");
  check_idle_400ms("cold boot");
}

static void test_reset_repeat() {
  // Second setup() call, simulating a reset (hardware state wiped, history restarted).
  boot(-1);
  run_ms(400);
  boot(-1);
  check_boot_safety("reset");
  check_idle_400ms("reset");

  // Reset with each switch held at boot.
  for (int pin = 2; pin <= 9; pin++) {
    char tag[48];
    snprintf(tag, sizeof tag, "reset with D%d held", pin);
    boot(-1);
    run_ms(400);
    boot(pin);
    check_boot_safety(tag);
    // Safety during setup() itself even with switch held: no LOW on A1, no HIGH on LEDs.
    char msg[128];
    snprintf(msg, sizeof msg, "%s: no relay LOW write inside setup()", tag);
    check(!ever_written(RLY, LOW), "REQ-SAF-02", msg);
    run_ms(5);  // less than the debounce: still must not energize.
    snprintf(msg, sizeof msg, "%s: A1 HIGH 5 ms after boot (before any debounce completes)", tag);
    check(mock_get_output(RLY) == HIGH && !ever_written(RLY, LOW), "REQ-SAF-02", msg);
  }

  // setup() called twice without clearing history (re-entry): A1 still never LOW.
  boot(-1);
  setup();
  check(!ever_written(RLY, LOW), "REQ-SAF-02", "setup() called twice: A1 never LOW");
  bool led_hi = false;
  for (int i = 0; i < 4; i++) led_hi = led_hi || ever_written(LEDP[i], HIGH);
  check(!led_hi, "REQ-SAF-01", "setup() called twice: no LED HIGH write");
}

// REQ-REL-02: A1 LOW only while pulsing for Sour/Bitter.
static void test_relay_polarity() {
  const uint8_t reflex[] = {6, 7, 8, 9};
  const uint8_t calm[] = {2, 3, 4, 5};
  for (uint8_t pin : reflex) {
    char msg[128];
    boot(-1);
    run_ms(100);
    snprintf(msg, sizeof msg, "D%d: A1 HIGH (de-energized) before press", pin);
    check(mock_get_output(RLY) == HIGH, "REQ-REL-02", msg);

    mock_set_input(pin, LOW);
    bool seen_low = false;
    unsigned low_ms = 0;
    int falls = 0;
    int last = HIGH;
    for (int k = 0; k < 800; k++) {
      loop();
      int r = mock_get_output(RLY);
      if (r == LOW) { seen_low = true; low_ms++; }
      if (r == LOW && last == HIGH) falls++;
      last = r;
      mock_advance(1);
    }
    snprintf(msg, sizeof msg, "D%d: A1 driven LOW (relay energized) after press", pin);
    check(seen_low, "REQ-REL-02", msg);
    snprintf(msg, sizeof msg, "D%d: single energize pulse, %u ms low", pin, low_ms);
    check(falls == 1 && low_ms > 0 && low_ms < 400, "REQ-REL-02", msg);
    snprintf(msg, sizeof msg, "D%d: A1 back HIGH (de-energized) after pulse while still held", pin);
    check(mock_get_output(RLY) == HIGH, "REQ-REL-02", msg);

    mock_set_input(pin, HIGH);
    run_ms(200);
    snprintf(msg, sizeof msg, "D%d: A1 HIGH after release", pin);
    check(mock_get_output(RLY) == HIGH, "REQ-REL-02", msg);

    // Relay pin is never configured as anything but OUTPUT, and never written LOW before the press.
    snprintf(msg, sizeof msg, "D%d: A1 stays OUTPUT", pin);
    check(mock_get_mode(RLY) == OUTPUT, "REQ-REL-02", msg);
  }

  for (uint8_t pin : calm) {
    char msg[128];
    boot(-1);
    run_ms(50);
    mock_set_input(pin, LOW);
    run_ms(400);
    snprintf(msg, sizeof msg, "D%d (Sweet/Salty): relay never energized", pin);
    check(mock_get_output(RLY) == HIGH && !ever_written(RLY, LOW), "REQ-REL-02", msg);
  }
}

int main() {
  mock_reset();
  mock_clear_touched();
  mock_set_millis(0);

  test_cold_boot();
  test_reset_repeat();
  test_relay_polarity();

  printf("RESULT tests=%d failed=%d\n", g_tests, g_failed);
  return g_failed ? 1 : 0;
}
