```markdown
# Architecture Specification: Project Paladar (Interactive Demonstrator)

## 1. System Topology & Electrical Isolation
The demonstrator employs two galvanically isolated power domains sharing an optical control boundary to protect the logic controller from inductive kickback, electrical noise, and voltage drops during motor spin-up[cite: 1, 2].


```

+---------------------------------------------------------------------------------+
| PRIMARY LOGIC DOMAIN (5V DC via USB Bus)                                        |
|                                                                                 |
|  [USB 5V Supply] ───> Arduino Nano 5V Bus ───+───> [8x Push Buttons]            |
|                                              │     (D2 - D9, INPUT_PULLUP)      |
|                                              │                                  |
|                                              +───> [20x LEDs via 220Ω Resistors]|
|                                              │     (D10 - D12, A0)              |
|                                              │                                  |
|                                              +───> [Relay Logic VCC / GND]      |
|                                                    (A1 Digital Control Line)    |
+---------------------------------------------------------------------------------+
│ Optical Isolation Barrier (Optocoupler)
▼
+---------------------------------------------------------------------------------+
| SECONDARY ACTUATOR DOMAIN (1.5V DC Isolated Rail)                     |
|                                                                                 |
|  [1.5V AA Battery (+)] ──────────────────────────────> [Motor (+) Terminal]     |
|                                                        (Coreless ERM Unit)
|                                                              │                  |
|  [Relay NO Contact]    <─────────────────────────────────────+                  |
|                                                                                 |
|  [Relay COM Contact]   <──────────────────────────────> [1.5V AA Battery (-)]   |
+---------------------------------------------------------------------------------+

```

---

## 2. Hardware Bill of Materials (BOM)
* **Microcontroller:** 1x Arduino Nano (Microchip ATmega328P, 16 MHz, 5V logic).
* **Operator Inputs:** 8x Momentary tactile switches with integrated plastic caps (2 buttons per taste category)[cite: 3].
* **Optical Receptors:** 20x 5mm standard through-hole LEDs (5x Sweet, 5x Salty, 5x Sour, 5x Bitter).
* **Current-Limiting Resistors:** 20x 220Ω (1/4W) carbon/metal film resistors (one discrete resistor per LED).
* **Isolation & Relay Stage:** 1x Single-channel 5V optocoupled relay module (`SRD-05VDC-SL-C`)[cite: 2].
* **Haptic Actuator:** 1x 1.5V sub-miniature DC coreless motor with an eccentric rotating mass (ERM)[cite: 1].
* **Power Units:**
  * Logic & LEDs: Standard 5V regulated USB source / power bank.
  * Haptic Motor: 1x 1.5V AA single-cell battery holder[cite: 1].

---

## 3. Microcontroller Pin Mapping (Arduino Nano)

### Input Subsystem (Cardboard Presentation Console)
All tactile inputs interface directly between the assigned digital pin and `GND`, utilizing ATmega328P internal pull-up resistors (`INPUT_PULLUP`)[cite: 3].

| Pin | Functional Trigger | Taste Group | Hardware Configuration | Default State | Triggered State |
|---|---|---|---|---|---|
| **D2** | Food Cutout 1 | Sweet | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D3** | Food Cutout 2 | Sweet | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D4** | Food Cutout 1 | Salty | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D5** | Food Cutout 2 | Salty | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D6** | Food Cutout 1 | Sour | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D7** | Food Cutout 2 | Sour | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D8** | Food Cutout 1 | Bitter | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |
| **D9** | Food Cutout 2 | Bitter | Direct switch to GND[cite: 3] | HIGH (5V) | LOW (0V) |

### Output Subsystem (LED Array & Actuator Trigger)
Analog pins `A0` and `A1` are mapped as digital output lines.

| Pin | Controlled Element | Electrical Circuit | Maximum Current Draw | Output Active State |
|---|---|---|---|---|
| **D10** | Sweet Receptors | 5x parallel branches (each branch: 220Ω + LED) | ~20 mA total | HIGH |
| **D11** | Salty Receptors | 5x parallel branches (each branch: 220Ω + LED) | ~20 mA total | HIGH |
| **D12** | Sour Receptors | 5x parallel branches (each branch: 220Ω + LED) | ~20 mA total | HIGH |
| **A0** | Bitter Receptors | 5x parallel branches (each branch: 220Ω + LED) | ~20 mA total | HIGH |
| **A1** | Relay Signal (`IN`) | Logic optocoupler input[cite: 2] | < 5 mA | LOW (Active LOW)[cite: 2] |

---

## 4. Physical 3D Tongue Interspersed LED Matrix
To counter the biological misconception of localized tongue zones, the 5 LEDs of each flavor are distributed uniformly across anterior, lateral, and posterior areas of the 3D model.


```

```
                  +-----------------------------+
                  |       Tongue Base           |
                  |  [Bitter 1]       [Sour 1]  |
                  |  [Sweet 1]       [Salty 1]  |
                  |                             |
                  |  [Sour 2]       [Bitter 2]  |
                  |  [Salty 2]       [Sweet 2]  |
                  |                             |
                  |  [Sweet 3]       [Salty 3]  |
                  |  [Bitter 3]       [Sour 3]  |
                  |                             |
                  |  [Salty 4]       [Sweet 4]  |
                  |  [Sour 4]       [Bitter 4]  |
                  |                             |
                  |  [Sweet 5]       [Salty 5]  |
                  |  [Bitter 5]       [Sour 5]  |
                  |       Tongue Tip            |
                  +-----------------------------+

```

```

---

## 5. Relay & Motor Interfacing Details
* **Relay Logic Terminal (Arduino Side)[cite: 2]:**
  * `VCC` $\rightarrow$ Arduino Nano 5V rail[cite: 2].
  * `GND` $\rightarrow$ Arduino Nano GND rail[cite: 2].
  * `IN` $\rightarrow$ Arduino Nano Pin A1[cite: 2].
* **Relay Load Terminal (Isolated 1.5V Side)[cite: 1, 2]:**
  * `COM` (Common) $\rightarrow$ 1.5V AA Battery Negative (-)[cite: 1, 2].
  * `NO` (Normally Open) $\rightarrow$ Motor Negative (Black wire)[cite: 1, 2].
  * 1.5V AA Battery Positive (+) $\rightarrow$ Motor Positive (Red wire) directly[cite: 1].
  * The motor and battery ground are completely disconnected from the Arduino GND rail to maintain full galvanic barrier integrity[cite: 1, 2].

---

## 6. Real-Time Firmware Specification

```cpp
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

```