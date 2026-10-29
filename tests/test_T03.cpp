// T-03 tests: core firmware behaviour, host build against tests/mock/Arduino.h.
// Covers REQ-INP-01..03, REQ-LED-02..04, REQ-HAP-01..05, REQ-FW-01, REQ-FW-03,
// REQ-PWR-01, REQ-PWR-04. Run from the repo root (reads firmware source by path).
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
static const uint8_t T_SWEET = 0, T_SALTY = 1, T_SOUR = 2, T_BITTER = 3;
static const uint8_t SW[4][2] = {{2, 3}, {4, 5}, {6, 7}, {8, 9}};   // REQ-INP-02
static const uint8_t LEDP[4] = {10, 11, 12, A0};                    // REQ-LED-02
static const uint8_t RLY = A1;
static const char* TASTE[4] = {"Sweet", "Salty", "Sour", "Bitter"};

// ---------- simulation helpers ----------
struct Edge { uint32_t t; int level; };
static std::vector<Edge> g_relay_edges;
static int g_relay_last = HIGH;

static void sample_relay() {
  int l = mock_get_output(RLY);
  if (l != g_relay_last) {
    g_relay_last = l;
    g_relay_edges.push_back({mock_millis_now(), l});
  }
}

// Run loop() once per simulated millisecond.
static void run_ms(unsigned n) {
  for (; n; --n) {
    loop();
    sample_relay();
    mock_advance(1);
  }
}

// After every input change: ~60 ms of loop() so a 20-50 ms debounce is satisfied.
static void settle() { run_ms(60); }

static void press(uint8_t pin) { mock_set_input(pin, LOW); }
static void release(uint8_t pin) { mock_set_input(pin, HIGH); }

// Boot a clean device with the clock at `start`.
static void fresh(uint32_t start) {
  g_total_delay_calls += mock_delay_calls;  // mock_reset() clears the counter
  // Drain any state left by the previous scenario (running pulse etc.).
  mock_reset();
  mock_set_millis(0);
  setup();
  run_ms(400);
  g_total_delay_calls += mock_delay_calls;
  // Real boot.
  mock_reset();
  mock_set_millis(start);
  setup();
  g_relay_edges.clear();
  g_relay_last = mock_get_output(RLY);
}

static int count_falls() {
  int n = 0;
  for (const Edge& e : g_relay_edges) if (e.level == LOW) n++;
  return n;
}

// Duration of the k-th (0-based) LOW period of the relay, -1 if not complete.
static int pulse_ms(int k) {
  int seen = 0;
  for (size_t i = 0; i < g_relay_edges.size(); i++) {
    if (g_relay_edges[i].level != LOW) continue;
    if (seen++ != k) continue;
    if (i + 1 >= g_relay_edges.size()) return -1;
    return (int)(uint32_t)(g_relay_edges[i + 1].t - g_relay_edges[i].t);
  }
  return -1;
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

// ---------- tests ----------
static void test_boot_and_config() {
  fresh(0);
  bool pullups = true;
  for (int t = 0; t < 4; t++)
    for (int s = 0; s < 2; s++)
      if (mock_get_mode(SW[t][s]) != INPUT_PULLUP) pullups = false;
  check(pullups, "REQ-INP-01", "D2-D9 all configured INPUT_PULLUP");

  bool outs = true;
  for (int i = 0; i < 4; i++) if (mock_get_mode(LEDP[i]) != OUTPUT) outs = false;
  check(outs, "REQ-LED-02", "D10,D11,D12,A0 configured as OUTPUT");
  check(mock_get_mode(RLY) == OUTPUT, "REQ-HAP-04", "relay pin A1 configured as OUTPUT");

  check(all_leds_low(), "REQ-LED-04", "all LED outputs LOW after setup");
  check(mock_get_output(RLY) == HIGH, "REQ-HAP-04", "relay de-energized (HIGH) after setup");

  bool never_low = true;
  for (const MockEvent& e : mock_history)
    if (e.kind == MOCK_EV_WRITE && e.pin == RLY && e.value == LOW) never_low = false;
  check(never_low, "REQ-HAP-04", "relay never driven LOW during boot");

  run_ms(300);
  check(all_leds_low(), "REQ-LED-04", "LEDs stay LOW with nothing pressed");
  check(mock_get_output(RLY) == HIGH && g_relay_edges.empty(), "REQ-HAP-04",
        "relay stays de-energized with nothing pressed");
}

static void test_input_to_led_mapping() {
  for (int t = 0; t < 4; t++) {
    for (int s = 0; s < 2; s++) {
      fresh(0);
      char msg[96];
      press(SW[t][s]);
      settle();
      snprintf(msg, sizeof msg, "%s switch D%d drives only its LED HIGH", TASTE[t], SW[t][s]);
      check(only_led(t), "REQ-INP-02", msg);
      snprintf(msg, sizeof msg, "%s LOW switch D%d makes taste active (LED HIGH)", TASTE[t], SW[t][s]);
      check(mock_get_output(LEDP[t]) == HIGH, "REQ-INP-03", msg);
      snprintf(msg, sizeof msg, "%s LED output pin HIGH while active", TASTE[t]);
      check(mock_get_output(LEDP[t]) == HIGH, "REQ-LED-03", msg);

      release(SW[t][s]);
      settle();
      snprintf(msg, sizeof msg, "%s LED LOW after release of D%d", TASTE[t], SW[t][s]);
      check(all_leds_low(), "REQ-LED-04", msg);
    }
    // Both switches of the taste.
    fresh(0);
    press(SW[t][0]); press(SW[t][1]);
    settle();
    check(only_led(t), "REQ-INP-03", "both switches of a taste pressed: active");
    release(SW[t][0]);
    settle();
    check(only_led(t), "REQ-INP-03", "taste stays active while the other switch is still LOW");
    release(SW[t][1]);
    settle();
    check(all_leds_low(), "REQ-LED-04", "taste inactive once both switches released");
  }

  // Independent tastes at once.
  fresh(0);
  press(SW[T_SWEET][0]); press(SW[T_SALTY][1]);
  settle();
  check(mock_get_output(LEDP[T_SWEET]) == HIGH && mock_get_output(LEDP[T_SALTY]) == HIGH &&
        mock_get_output(LEDP[T_SOUR]) == LOW && mock_get_output(LEDP[T_BITTER]) == LOW,
        "REQ-LED-03", "multiple tastes light independently");
}

static void test_haptic_basic() {
  const uint8_t reflex[] = {6, 7, 8, 9};
  for (uint8_t p : reflex) {
    fresh(0);
    char msg[96];
    press(p);
    settle();
    snprintf(msg, sizeof msg, "relay energized (LOW) after pressing D%d", p);
    check(mock_get_output(RLY) == LOW, "REQ-HAP-01", msg);
    run_ms(400);
    snprintf(msg, sizeof msg, "D%d: exactly one pulse while held", p);
    check(count_falls() == 1, "REQ-HAP-01", msg);
    int d = pulse_ms(0);
    snprintf(msg, sizeof msg, "D%d: pulse lasts 200 ms (measured %d)", p, d);
    check(d >= 200 && d <= 201, "REQ-HAP-02", msg);
    check(mock_get_output(RLY) == HIGH, "REQ-HAP-02", "relay de-energized after pulse");
  }

  // Sour + Bitter together: still a single pulse.
  fresh(0);
  press(6); press(8);
  settle();
  run_ms(400);
  check(count_falls() == 1, "REQ-HAP-03", "Sour+Bitter pressed together yield a single pulse");
}

static void test_sweet_salty_no_haptic() {
  const uint8_t pins[] = {2, 3, 4, 5};
  for (uint8_t p : pins) {
    fresh(0);
    press(p);
    settle();
    run_ms(500);
    char msg[80];
    snprintf(msg, sizeof msg, "relay stays HIGH while only D%d held", p);
    check(count_falls() == 0 && mock_get_output(RLY) == HIGH, "REQ-HAP-04", msg);
  }
  fresh(0);
  press(2); press(4);
  settle();
  run_ms(500);
  check(count_falls() == 0 && mock_get_output(RLY) == HIGH, "REQ-HAP-04",
        "relay stays HIGH with Sweet and Salty together");
}

static void test_retrigger_ignored_during_pulse() {
  // Sour released and re-pressed while the pulse runs.
  fresh(0);
  press(6);
  settle();
  release(6);
  settle();
  press(6);
  settle();
  run_ms(500);
  check(count_falls() == 1, "REQ-HAP-03", "re-press of Sour during pulse starts no new pulse");
  check(pulse_ms(0) >= 200 && pulse_ms(0) <= 201, "REQ-HAP-03", "original pulse not extended by re-press");

  // Bitter pressed during a Sour pulse, held past the end of the pulse.
  fresh(0);
  press(6);
  settle();
  release(6);
  settle();
  press(8);
  settle();
  run_ms(500);
  check(count_falls() == 1, "REQ-HAP-03", "Bitter press during Sour pulse is ignored");
  check(count_falls() == 1, "REQ-HAP-05", "input still held after pulse end does not retrigger");
}

static void test_no_repulse_while_held() {
  fresh(0);
  press(6);
  settle();
  run_ms(800);
  check(count_falls() == 1, "REQ-HAP-05", "held Sour yields one pulse over 860 ms");
  check(mock_get_output(RLY) == HIGH, "REQ-HAP-05", "relay stays off after pulse while still held");

  release(6);
  settle();
  press(6);
  settle();
  run_ms(400);
  check(count_falls() == 2, "REQ-HAP-05", "release then press again starts a second pulse");
  check(pulse_ms(1) >= 200 && pulse_ms(1) <= 201, "REQ-HAP-02", "second pulse also 200 ms");

  // Same for Bitter via the second switch.
  fresh(0);
  press(9);
  settle();
  run_ms(800);
  check(count_falls() == 1, "REQ-HAP-05", "held Bitter yields one pulse");
}

static void test_millis_rollover() {
  const uint32_t starts[] = {0xFFFFFFFFu - 50u, 0xFFFFFFFFu - 150u, 0xFFFFFFFFu - 10u, 0xFFFFFFFFu};
  for (uint32_t st : starts) {
    fresh(st);
    char msg[96];
    press(6);
    settle();
    run_ms(400);
    snprintf(msg, sizeof msg, "start=0x%08X: clock wrapped during scenario", (unsigned)st);
    check(mock_millis_now() < st, "REQ-FW-03", msg);
    snprintf(msg, sizeof msg, "start=0x%08X: one pulse across rollover", (unsigned)st);
    check(count_falls() == 1, "REQ-FW-03", msg);
    int d = pulse_ms(0);
    snprintf(msg, sizeof msg, "start=0x%08X: pulse length 200 ms across rollover (measured %d)", (unsigned)st, d);
    check(d >= 200 && d <= 201, "REQ-FW-03", msg);
    check(mock_get_output(RLY) == HIGH, "REQ-FW-03", "relay off after pulse across rollover");
  }

  // Rollover while idle must not cause a spurious pulse.
  fresh(0xFFFFFFFFu - 100u);
  run_ms(300);
  check(count_falls() == 0, "REQ-FW-03", "no spurious pulse when clock wraps while idle");
}

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

static bool has_identifier(const std::string& src, const char* word) {
  size_t n = strlen(word), pos = 0;
  while ((pos = src.find(word, pos)) != std::string::npos) {
    bool before_ok = pos == 0 || !(isalnum((unsigned char)src[pos - 1]) || src[pos - 1] == '_');
    if (before_ok) return true;
    pos += n;
  }
  return false;
}

static void test_source_scan() {
  FILE* f = fopen("firmware/paladar/paladar.ino", "rb");
  check(f != nullptr, "REQ-PWR-04", "firmware/paladar/paladar.ino readable (run from repo root)");
  if (!f) return;
  std::string raw;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) raw.append(buf, n);
  fclose(f);
  std::string src = strip_comments(raw);

  check(!has_identifier(src, "Serial"), "REQ-PWR-04", "no Serial usage in firmware");
  check(!has_identifier(src, "WiFi"), "REQ-PWR-04", "no WiFi usage in firmware");
  check(!has_identifier(src, "Ethernet"), "REQ-PWR-04", "no Ethernet usage in firmware");
  check(!has_identifier(src, "Bluetooth") && !has_identifier(src, "BLE"), "REQ-PWR-04",
        "no Bluetooth usage in firmware");
  bool bad_inc = false;
  size_t pos = 0;
  while ((pos = src.find("#include", pos)) != std::string::npos) {
    size_t e = src.find('\n', pos);
    std::string line = src.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
    if (line.find("Serial") != std::string::npos || line.find("WiFi") != std::string::npos ||
        line.find("Ethernet") != std::string::npos || line.find("SPI") != std::string::npos ||
        line.find("Wire") != std::string::npos || line.find("ESP") != std::string::npos)
      bad_inc = true;
    pos += 8;
  }
  check(!bad_inc, "REQ-PWR-04", "no network/bus headers included");
  check(!has_identifier(src, "delay") && !has_identifier(src, "delayMicroseconds"), "REQ-FW-01",
        "firmware source never references delay()");
  check(src.find("millis") != std::string::npos, "REQ-FW-01", "firmware uses millis()");
}

static void test_pins_used() {
  // Touched-pin set accumulated over every scenario in this binary.
  bool ok = true;
  for (int p = 0; p < MOCK_NUM_PINS; p++) {
    bool allowed = (p >= 2 && p <= 12) || p == A0 || p == A1;
    if (mock_touched[p] && !allowed) ok = false;
  }
  check(ok, "REQ-PWR-01", "only D2-D12, A0, A1 are used (no D0/D1 serial, no D13, no A2-A5)");
  check(!mock_touched[63], "REQ-PWR-01", "no out-of-range pin numbers used");
  check(!mock_touched[0] && !mock_touched[1], "REQ-PWR-04", "RX/TX pins D0/D1 never touched");
  bool all_used = true;
  for (int p = 2; p <= 12; p++) if (!mock_touched[p]) all_used = false;
  if (!mock_touched[A0] || !mock_touched[A1]) all_used = false;
  check(all_used, "REQ-PWR-01", "all 8 inputs, 4 LED outputs and relay pin are used");
}

int main() {
  mock_reset();
  mock_clear_touched();
  mock_set_millis(0);

  test_boot_and_config();
  test_input_to_led_mapping();
  test_haptic_basic();
  test_sweet_salty_no_haptic();
  test_retrigger_ignored_during_pulse();
  test_no_repulse_while_held();
  test_millis_rollover();
  test_source_scan();

  g_total_delay_calls += mock_delay_calls;
  check(g_total_delay_calls == 0, "REQ-FW-01", "delay()/delayMicroseconds() never called at runtime");
  test_pins_used();

  printf("RESULT tests=%d failed=%d\n", g_tests, g_failed);
  return g_failed ? 1 : 0;
}
