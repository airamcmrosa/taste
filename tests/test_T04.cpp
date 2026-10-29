// T-04 tests: switch debounce. Host build against tests/mock/Arduino.h.
// Covers REQ-FW-02 (debounce 20-50 ms), REQ-FW-04 (LED response <= 100 ms),
// REQ-INP-03 (either switch of a taste activates it, with debounce).
// Run from the repo root (reads firmware source by path).
//
// NOTE: do not use the `long` keyword here: the mock remaps it to 32-bit.
#include <cctype>
#include <Arduino.h>
#include "../firmware/paladar/paladar.ino"

// ---------- minimal harness ----------
static int g_tests = 0;
static int g_failed = 0;
static unsigned g_total_delay_calls = 0;

static void check(bool cond, const char* req, const char* msg) {
  g_tests++;
  if (!cond) {
    g_failed++;
    printf("FAIL [%s] %s\n", req, msg);
  } else {
    printf("ok   [%s] %s\n", req, msg);
  }
}

// ---------- independent description of the hardware (from requirements) ----------
static const uint8_t SW[4][2] = {{2, 3}, {4, 5}, {6, 7}, {8, 9}};
static const uint8_t LEDP[4] = {10, 11, 12, A0};
static const uint8_t RLY = A1;
static const char* TASTE[4] = {"Sweet", "Salty", "Sour", "Bitter"};

// Debounce interval as declared in the source (filled by test_source_scan).
static int g_debounce = 30;

// ---------- simulation helpers ----------
static bool g_led_seen_high = false;   // any LED output HIGH observed since clear_obs()
static bool g_relay_seen_low = false;  // relay A1 LOW observed since clear_obs()
static int g_relay_falls = 0;
static int g_relay_last = HIGH;
static bool g_led_dropped = false;     // observed LED low after being armed (see watch_led)
static int g_watch_led = -1;           // taste index whose LED must stay HIGH, -1 none

static void clear_obs() {
  g_led_seen_high = false;
  g_relay_seen_low = false;
  g_relay_falls = 0;
  g_relay_last = mock_get_output(RLY);
  g_led_dropped = false;
  g_watch_led = -1;
}

static void observe() {
  for (int i = 0; i < 4; i++)
    if (mock_get_output(LEDP[i]) == HIGH) g_led_seen_high = true;
  int r = mock_get_output(RLY);
  if (r == LOW) g_relay_seen_low = true;
  if (r != g_relay_last) {
    if (r == LOW) g_relay_falls++;
    g_relay_last = r;
  }
  if (g_watch_led >= 0 && mock_get_output(LEDP[g_watch_led]) != HIGH) g_led_dropped = true;
}

static void run_ms(unsigned n) {
  for (; n; --n) {
    loop();
    observe();
    mock_advance(1);
  }
}

static void press(uint8_t pin) { mock_set_input(pin, LOW); }
static void release(uint8_t pin) { mock_set_input(pin, HIGH); }

static void fresh(uint32_t start) {
  g_total_delay_calls += mock_delay_calls;
  mock_reset();
  mock_set_millis(0);
  setup();
  run_ms(400);
  g_total_delay_calls += mock_delay_calls;
  mock_reset();
  mock_set_millis(start);
  setup();
  clear_obs();
}

static bool only_led(int t) {
  for (int i = 0; i < 4; i++) {
    int want = (i == t) ? HIGH : LOW;
    if (mock_get_output(LEDP[i]) != want) return false;
  }
  return true;
}
static bool all_leds_low() {
  for (int i = 0; i < 4; i++) if (mock_get_output(LEDP[i]) != LOW) return false;
  return true;
}

// Level `level` just applied to `pin`; step 1 ms at a time until LED t == want.
// Returns ms elapsed (loop calls) or -1 if not reached within `max`.
static int ms_until_led(int t, int want, int max) {
  for (int ms = 0; ms <= max; ms++) {
    if (mock_get_output(LEDP[t]) == want && ms > 0) return ms;
    run_ms(1);
    if (mock_get_output(LEDP[t]) == want) return ms + 1;
  }
  return -1;
}

// ---------- tests ----------
static std::string strip_comments(const std::string& s) {
  std::string out;
  size_t i = 0;
  while (i < s.size()) {
    if (s.compare(i, 2, "//") == 0) { while (i < s.size() && s[i] != '\n') i++; }
    else if (s.compare(i, 2, "/*") == 0) {
      size_t e = s.find("*/", i + 2);
      i = (e == std::string::npos) ? s.size() : e + 2;
    } else out += s[i++];
  }
  return out;
}

static void test_source_scan() {
  FILE* f = fopen("firmware/paladar/paladar.ino", "rb");
  check(f != nullptr, "REQ-FW-02", "firmware/paladar/paladar.ino readable (run from repo root)");
  if (!f) return;
  std::string raw;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) raw.append(buf, n);
  fclose(f);
  std::string src = strip_comments(raw);

  size_t p = src.find("DEBOUNCE_MS");
  bool found = false;
  int val = -1;
  while (p != std::string::npos) {
    size_t q = p + strlen("DEBOUNCE_MS");
    while (q < src.size() && isspace((unsigned char)src[q])) q++;
    if (q < src.size() && src[q] == '=') {
      q++;
      while (q < src.size() && isspace((unsigned char)src[q])) q++;
      if (q < src.size() && isdigit((unsigned char)src[q])) {
        val = atoi(src.c_str() + q);
        found = true;
        break;
      }
    }
    p = src.find("DEBOUNCE_MS", p + 1);
  }
  check(found, "REQ-FW-02", "DEBOUNCE_MS = <number> declared in firmware source");
  check(found && val >= 20 && val <= 50, "REQ-FW-02", "DEBOUNCE_MS within 20..50 ms");
  if (found && val >= 20 && val <= 50) g_debounce = val;

  // No blocking delay in the source.
  bool has_delay = false;
  size_t pos = 0;
  while ((pos = src.find("delay", pos)) != std::string::npos) {
    bool before_ok = pos == 0 || !(isalnum((unsigned char)src[pos - 1]) || src[pos - 1] == '_');
    if (before_ok) has_delay = true;
    pos += 5;
  }
  check(!has_delay, "REQ-FW-02", "debounce does not use delay() in source");
}

static void test_glitch_rejected_all_switches() {
  for (int t = 0; t < 4; t++) {
    for (int s = 0; s < 2; s++) {
      uint8_t pin = SW[t][s];
      char msg[112];

      // Single 10 ms LOW glitch.
      fresh(0);
      run_ms(20);
      press(pin);
      run_ms(10);
      release(pin);
      run_ms(150);
      snprintf(msg, sizeof msg, "D%d (%s): 10 ms LOW glitch: no LED change", pin, TASTE[t]);
      check(!g_led_seen_high && all_leds_low(), "REQ-FW-02", msg);
      snprintf(msg, sizeof msg, "D%d (%s): 10 ms LOW glitch: relay never actuated", pin, TASTE[t]);
      check(!g_relay_seen_low && mock_get_output(RLY) == HIGH, "REQ-FW-02", msg);

      // Repeated chatter, every gap well below the interval.
      fresh(0);
      run_ms(20);
      for (int k = 0; k < 20; k++) {
        press(pin);
        run_ms(5);
        release(pin);
        run_ms(5);
      }
      for (int k = 0; k < 10; k++) {
        press(pin);
        run_ms(15);
        release(pin);
        run_ms(1);
      }
      run_ms(150);
      snprintf(msg, sizeof msg, "D%d (%s): chatter (gaps < interval): no LED change", pin, TASTE[t]);
      check(!g_led_seen_high && all_leds_low(), "REQ-FW-02", msg);
      snprintf(msg, sizeof msg, "D%d (%s): chatter: relay never actuated", pin, TASTE[t]);
      check(!g_relay_seen_low && mock_get_output(RLY) == HIGH, "REQ-FW-02", msg);
    }
  }
}

static void test_stable_accept_and_not_early() {
  for (int t = 0; t < 4; t++) {
    for (int s = 0; s < 2; s++) {
      uint8_t pin = SW[t][s];
      char msg[112];

      // Not accepted before DEBOUNCE_MS - 5 ms of stability.
      fresh(0);
      run_ms(20);
      press(pin);
      run_ms((unsigned)(g_debounce - 5));
      snprintf(msg, sizeof msg, "D%d: press not accepted before DEBOUNCE_MS-5 ms", pin);
      check(!g_led_seen_high && all_leds_low(), "REQ-FW-02", msg);

      // Held >= 50 ms: accepted.
      run_ms(50);
      snprintf(msg, sizeof msg, "D%d (%s): press held stable >= 50 ms is accepted", pin, TASTE[t]);
      check(only_led(t), "REQ-FW-02", msg);

      // Release: not accepted before stable...
      release(pin);
      run_ms((unsigned)(g_debounce - 5));
      snprintf(msg, sizeof msg, "D%d: release not accepted before DEBOUNCE_MS-5 ms", pin);
      check(only_led(t), "REQ-FW-02", msg);
      // ...accepted when stable.
      run_ms(50);
      snprintf(msg, sizeof msg, "D%d: release held stable >= 50 ms is accepted", pin);
      check(all_leds_low(), "REQ-FW-02", msg);
    }
  }
}

static void test_bounce_then_settle() {
  // Bouncy press edge that settles LOW is accepted; bouncy release edge that settles HIGH too.
  fresh(0);
  run_ms(20);
  for (int k = 0; k < 4; k++) { press(2); run_ms(3); release(2); run_ms(3); }
  press(2);
  run_ms(80);
  check(only_led(0), "REQ-FW-02", "bouncy press that settles LOW is accepted");
  for (int k = 0; k < 4; k++) { release(2); run_ms(3); press(2); run_ms(3); }
  release(2);
  run_ms(80);
  check(all_leds_low(), "REQ-FW-02", "bouncy release that settles HIGH is accepted");

  // A short HIGH glitch while pressed does not drop the LED.
  fresh(0);
  press(4);
  run_ms(80);
  check(only_led(1), "REQ-FW-02", "Salty accepted before release-glitch test");
  g_watch_led = 1;
  release(4);
  run_ms(10);
  press(4);
  run_ms(100);
  check(!g_led_dropped && only_led(1), "REQ-FW-02", "10 ms HIGH glitch while held does not turn LED off");
}

static void test_response_time() {
  for (int t = 0; t < 4; t++) {
    for (int s = 0; s < 2; s++) {
      uint8_t pin = SW[t][s];
      char msg[112];
      fresh(0);
      run_ms(20);
      press(pin);
      int on = ms_until_led(t, HIGH, 200);
      snprintf(msg, sizeof msg, "D%d (%s): LED on %d ms after stable press (<= 100)", pin, TASTE[t], on);
      check(on > 0 && on <= 100, "REQ-FW-04", msg);
      run_ms(100);
      release(pin);
      int off = ms_until_led(t, LOW, 200);
      snprintf(msg, sizeof msg, "D%d (%s): LED off %d ms after stable release (<= 100)", pin, TASTE[t], off);
      check(off > 0 && off <= 100, "REQ-FW-04", msg);
    }
  }
  // Sour/Bitter relay also within 100 ms of a stable press.
  const uint8_t reflex[] = {6, 7, 8, 9};
  for (uint8_t pin : reflex) {
    fresh(0);
    run_ms(20);
    press(pin);
    int ms = 0;
    while (ms < 200 && mock_get_output(RLY) != LOW) { run_ms(1); ms++; }
    char msg[96];
    snprintf(msg, sizeof msg, "D%d: relay actuated %d ms after stable press (<= 100)", pin, ms);
    check(mock_get_output(RLY) == LOW && ms <= 100, "REQ-FW-04", msg);
  }
}

static void test_two_switch_taste() {
  for (int t = 0; t < 4; t++) {
    uint8_t a = SW[t][0], b = SW[t][1];
    char msg[112];

    // Bounce on one while the other is held steady: stays active.
    fresh(0);
    run_ms(20);
    press(b);
    run_ms(100);
    snprintf(msg, sizeof msg, "%s: second switch alone activates taste", TASTE[t]);
    check(only_led(t), "REQ-INP-03", msg);
    clear_obs();
    g_watch_led = t;
    for (int k = 0; k < 15; k++) { press(a); run_ms(4); release(a); run_ms(4); }
    press(a);
    run_ms(100);
    release(a);
    run_ms(100);
    snprintf(msg, sizeof msg, "%s: bounce on one switch while other held keeps taste active", TASTE[t]);
    check(!g_led_dropped && only_led(t), "REQ-INP-03", msg);
    if (t >= 2) {
      snprintf(msg, sizeof msg, "%s: bounce while other switch held causes no extra relay pulse", TASTE[t]);
      check(g_relay_falls == 0, "REQ-INP-03", msg);
    }

    // Release the held switch only: taste goes inactive after debounce.
    release(b);
    run_ms(100);
    snprintf(msg, sizeof msg, "%s: inactive once both switches released", TASTE[t]);
    check(all_leds_low(), "REQ-INP-03", msg);

    // Either switch LOW => active; both LOW, one released => still active.
    fresh(0);
    run_ms(20);
    press(a); press(b);
    run_ms(100);
    check(only_led(t), "REQ-INP-03", "both switches LOW: taste active");
    release(a);
    run_ms(100);
    check(only_led(t), "REQ-INP-03", "one of two released: still active");
  }

  // Sour bounce below the interval does not pulse the relay (either switch).
  const uint8_t sour[] = {6, 7};
  for (uint8_t pin : sour) {
    fresh(0);
    run_ms(20);
    for (int k = 0; k < 10; k++) { press(pin); run_ms(8); release(pin); run_ms(8); }
    run_ms(200);
    char msg[96];
    snprintf(msg, sizeof msg, "Sour D%d bounce below interval does not pulse relay", pin);
    check(!g_relay_seen_low && g_relay_falls == 0 && !g_led_seen_high, "REQ-INP-03", msg);
  }

  // A real Sour press after bounce yields exactly one pulse.
  fresh(0);
  run_ms(20);
  for (int k = 0; k < 5; k++) { press(6); run_ms(4); release(6); run_ms(4); }
  press(6);
  run_ms(600);
  check(g_relay_falls == 1, "REQ-INP-03", "bouncy Sour press then hold yields exactly one relay pulse");
}

static void test_rollover_and_no_delay() {
  const uint32_t starts[] = {0xFFFFFFFFu - 25u, 0xFFFFFFFFu - 5u, 0xFFFFFFFFu - 60u, 0xFFFFFFFFu};
  for (uint32_t st : starts) {
    char msg[112];
    // Glitch across the wrap is rejected.
    fresh(st);
    press(6);
    run_ms(10);
    release(6);
    run_ms(150);
    snprintf(msg, sizeof msg, "start=0x%08X: glitch across millis rollover rejected", (unsigned)st);
    check(!g_led_seen_high && !g_relay_seen_low, "REQ-FW-02", msg);

    // Stable press across the wrap is accepted within 100 ms.
    fresh(st);
    press(6);
    int on = ms_until_led(2, HIGH, 200);
    snprintf(msg, sizeof msg, "start=0x%08X: press accepted across rollover in %d ms", (unsigned)st, on);
    check(on > 0 && on <= 100, "REQ-FW-04", msg);
    run_ms(300);
    snprintf(msg, sizeof msg, "start=0x%08X: clock wrapped during scenario", (unsigned)st);
    check(mock_millis_now() < st, "REQ-FW-02", msg);
    release(6);
    int off = ms_until_led(2, LOW, 200);
    snprintf(msg, sizeof msg, "start=0x%08X: release accepted in %d ms", (unsigned)st, off);
    check(off > 0 && off <= 100, "REQ-FW-04", msg);
  }

  // Rollover while idle / pressed does not create spurious changes.
  fresh(0xFFFFFFFFu - 100u);
  run_ms(300);
  check(!g_led_seen_high && !g_relay_seen_low, "REQ-FW-02", "no spurious activity when clock wraps while idle");

  fresh(0xFFFFFFFFu - 100u);
  press(3);
  run_ms(250);
  check(only_led(0), "REQ-FW-02", "held switch stays active across rollover");
}

int main() {
  mock_reset();
  mock_clear_touched();
  mock_set_millis(0);

  test_source_scan();
  test_glitch_rejected_all_switches();
  test_stable_accept_and_not_early();
  test_bounce_then_settle();
  test_response_time();
  test_two_switch_taste();
  test_rollover_and_no_delay();

  g_total_delay_calls += mock_delay_calls;
  check(g_total_delay_calls == 0, "REQ-FW-02", "delay()/delayMicroseconds() never called at runtime");

  printf("RESULT tests=%d failed=%d\n", g_tests, g_failed);
  return g_failed ? 1 : 0;
}
