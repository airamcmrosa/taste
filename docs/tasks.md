---
title: Project Paladar — Tasks
version: 0.1.0
status: draft
created: 2026-10-29
updated: 2026-10-29
related: docs/requirements.md
active_task: T-04
rag: one `## T-NN` block per task; read only the active block + its REQ entries
---

# Tasks

| ID | Area | Title | Req | Status |
|---|---|---|---|---|
| T-01 | Hardware | Fix LED group drive current | REQ-LED-05, REQ-LED-06 | done |
| T-02 | Docs | Correct current figures in architecture.md | REQ-LED-05, REQ-LED-06 | done |
| T-03 | Firmware | Create sketch from architecture §6 | REQ-INP-01..03, REQ-LED-02..04, REQ-HAP-01..05, REQ-FW-01, REQ-FW-03, REQ-PWR-01, REQ-PWR-04 | done |
| T-04 | Firmware | Add non-blocking debounce | REQ-FW-02, REQ-FW-04, REQ-INP-03 | todo |
| T-05 | Firmware | Remove relay boot glitch | REQ-SAF-01, REQ-SAF-02, REQ-REL-02 | todo |
| T-06 | Hardware | Wire 8 switches | REQ-INP-01, REQ-INP-02 | todo |
| T-07 | Hardware | Assemble interspersed LED matrix | REQ-LED-01, REQ-LED-07 | todo |
| T-08 | Hardware | Wire relay and isolated motor | REQ-REL-01, REQ-REL-03, REQ-PWR-02, REQ-PWR-03 | todo |
| T-09 | Verification | Bench test inputs, LEDs, safe state | REQ-INP-01..03, REQ-LED-02..04, REQ-FW-02, REQ-FW-04, REQ-SAF-01, REQ-SAF-02, REQ-PWR-01, REQ-PWR-04 | todo |
| T-10 | Verification | Measure LED pin current | REQ-LED-05, REQ-LED-06 | todo |
| T-11 | Verification | Measure haptic pulse timing | REQ-HAP-01..05, REQ-REL-02, REQ-FW-01, REQ-FW-03 | todo |
| T-12 | Verification | Verify galvanic isolation | REQ-PWR-02, REQ-PWR-03 | todo |

## T-01 · Fix LED group drive current
- [x] status: done
- area: Hardware
- req: REQ-LED-05, REQ-LED-06
- action: replace direct pin drive with a transistor/driver per group, or raise resistor values
- done-when: calculated current per MCU pin ≤ 40 mA and total ≤ 200 mA
- result: raise the 20 LED resistors from 220Ω to 680Ω, no driver transistors
  - per branch: (5-2)/680 ≈ 4.4 mA (worst case Vf=1.8V: ≈ 4.7 mA)
  - per pin: 5 branches ≈ 22 mA (worst ≈ 23.5 mA) ≤ 40 mA (REQ-LED-05)
  - total: 4 pins ≈ 88 mA (worst ≈ 94 mA) ≤ 200 mA (REQ-LED-06)
  - assumption: all four groups lit at once, red/yellow-class LEDs, Vf≈2V
  - follow-ups: T-02 updates BOM/§1/§3 (220Ω→680Ω, ~20 mA→~22 mA); T-10 measures real pin current

## T-02 · Correct current figures in architecture.md
- [x] status: done
- area: Docs
- req: REQ-LED-05, REQ-LED-06
- depends: T-01
- action: update §3 output table ("~20 mA total") and §1 diagram to the T-01 design
- done-when: no figure in architecture.md contradicts the T-01 calculation; `updated` bumped in requirements.md

## T-03 · Create sketch from architecture §6
- [x] status: done
- area: Firmware
- req: REQ-INP-01..03, REQ-LED-02..04, REQ-HAP-01..05, REQ-FW-01, REQ-FW-03, REQ-PWR-01, REQ-PWR-04
- action: add `firmware/paladar/paladar.ino` with the §6 code
- done-when: compiles for Arduino Nano (ATmega328P) with no warnings

## T-04 · Add non-blocking debounce
- [ ] status: todo
- area: Firmware
- req: REQ-FW-02, REQ-FW-04, REQ-INP-03
- depends: T-03
- action: per-switch stable-time filter using `millis()`; `DEBOUNCE_MS` constant in 20–50
- done-when: simulated bounce under `DEBOUNCE_MS` produces no LED or relay change; no `delay()` in code

## T-05 · Remove relay boot glitch
- [ ] status: todo
- area: Firmware
- req: REQ-SAF-01, REQ-SAF-02, REQ-REL-02
- depends: T-03
- action: write A1 HIGH before `pinMode(A1, OUTPUT)`; drive LED outputs LOW at setup
- done-when: scope or relay click test shows no relay actuation on power-up or reset

## T-06 · Wire 8 switches
- [ ] status: todo
- area: Hardware
- req: REQ-INP-01, REQ-INP-02
- action: connect D2–D9 to GND via switches per §3 table (Sweet 2/3, Salty 4/5, Sour 6/7, Bitter 8/9)
- done-when: continuity check pin→switch→GND for all 8

## T-07 · Assemble interspersed LED matrix
- [ ] status: todo
- area: Hardware
- req: REQ-LED-01, REQ-LED-07
- action: mount 20 LEDs with one 220 Ω resistor each, positions per §4 layout
- done-when: each taste has 5 LEDs spread over anterior, lateral and posterior areas

## T-08 · Wire relay and isolated motor
- [ ] status: todo
- area: Hardware
- req: REQ-REL-01, REQ-REL-03, REQ-PWR-02, REQ-PWR-03
- action: relay VCC/GND/IN to 5V/GND/A1; cell (+) → motor (+); motor (−) → NO; COM → cell (−)
- done-when: wiring matches §5; no wire links cell or motor to Arduino GND

## T-09 · Bench test inputs, LEDs, safe state
- [ ] status: todo
- area: Verification
- req: REQ-INP-01..03, REQ-LED-02..04, REQ-FW-02, REQ-FW-04, REQ-SAF-01, REQ-SAF-02, REQ-PWR-01, REQ-PWR-04
- depends: T-04, T-05, T-06, T-07
- done-when: each of 8 switches lights only its taste group; LEDs off at boot; runs on USB power alone

## T-10 · Measure LED pin current
- [ ] status: todo
- area: Verification
- req: REQ-LED-05, REQ-LED-06
- depends: T-01, T-07
- done-when: measured per-pin ≤ 40 mA and total ≤ 200 mA with all four tastes active

## T-11 · Measure haptic pulse timing
- [ ] status: todo
- area: Verification
- req: REQ-HAP-01..05, REQ-REL-02, REQ-FW-01, REQ-FW-03
- depends: T-04, T-05, T-08
- done-when: pulse 200 ms ± 20 ms on Sour/Bitter; none on Sweet/Salty; one pulse per press when held

## T-12 · Verify galvanic isolation
- [ ] status: todo
- area: Verification
- req: REQ-PWR-02, REQ-PWR-03
- depends: T-08
- done-when: multimeter shows open circuit between cell (−)/motor and Arduino GND with relay both open and energized
