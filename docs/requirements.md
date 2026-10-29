---
title: Project Paladar — Requirements (EARS)
version: 0.1.1
status: draft
created: 2026-10-29
updated: 2026-10-29
source: docs/architecture.md
owner: Maria Rosa
related: docs/tasks.md
changelog:
  - version: 0.1.1
    date: 2026-10-29
    change: REQ-LED-01 series resistor 220 Ω → 680 Ω to match T-01 current design.
  - version: 0.1.0
    date: 2026-10-29
    change: Initial EARS requirements derived from architecture.md; added corrective REQ-LED-05/06 (pin current) and REQ-FW-02 (debounce).
---

# Requirements

EARS patterns: **Ubiquitous** (`The system shall`), **Event** (`When`), **State** (`While`), **Unwanted** (`If … then`), **Optional** (`Where`).
Each requirement is one testable sentence. `Src` = section of `docs/architecture.md` unless stated.

## INP — Inputs
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-INP-01 | Ubiquitous | The system shall read 8 momentary tactile switches on pins D2–D9, each wired between its pin and GND and configured as `INPUT_PULLUP`. | §2, §3 |
| REQ-INP-02 | Ubiquitous | The system shall map two switches to each taste: Sweet D2/D3, Salty D4/D5, Sour D6/D7, Bitter D8/D9. | §3 |
| REQ-INP-03 | Event | When either switch of a taste reads LOW, the system shall treat that taste as active. | §6 |

## LED — Visual feedback
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-LED-01 | Ubiquitous | The system shall provide 20 LEDs, 5 per taste, each with its own 680 Ω series resistor. | §2 |
| REQ-LED-02 | Ubiquitous | The system shall drive each taste's LED group from one output: Sweet D10, Salty D11, Sour D12, Bitter A0. | §3 |
| REQ-LED-03 | State | While a taste is active, the system shall drive its LED output HIGH. | §6 |
| REQ-LED-04 | State | While a taste is inactive, the system shall drive its LED output LOW. | §6 |
| REQ-LED-05 | Ubiquitous | The system shall limit the current sourced by each MCU output pin to 40 mA or less. | §3 (corrective) |
| REQ-LED-06 | Ubiquitous | The system shall limit the total current drawn through all MCU I/O pins to 200 mA or less. | §3 (corrective) |
| REQ-LED-07 | Ubiquitous | The system shall place the 5 LEDs of each taste across the anterior, lateral and posterior areas of the tongue model. | §4 |

## HAP — Haptic reflex
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-HAP-01 | Event | When Sour or Bitter changes from inactive to active and no pulse is running, the system shall energize the relay and start the motor pulse. | §6 |
| REQ-HAP-02 | Event | When 200 ms have elapsed since the pulse started, the system shall de-energize the relay. | §6 |
| REQ-HAP-03 | State | While a pulse is running, the system shall ignore new haptic triggers. | §6 |
| REQ-HAP-04 | State | While only Sweet or Salty is active, the system shall keep the relay de-energized. | §6 |
| REQ-HAP-05 | State | While Sour or Bitter remains active after a pulse, the system shall not start another pulse until the input has gone inactive and active again. | §6 |

## REL — Relay & motor interface
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-REL-01 | Ubiquitous | The system shall switch the motor through an optocoupled 5V relay module (`SRD-05VDC-SL-C`) with VCC to 5V, GND to GND and IN to A1. | §2, §5 |
| REQ-REL-02 | State | While A1 is LOW, the relay shall be energized. | §3, §6 |
| REQ-REL-03 | Ubiquitous | The system shall connect the 1.5V cell (+) directly to the motor (+), the motor (−) to relay NO, and relay COM to the cell (−). | §5 |

## PWR — Power & isolation
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-PWR-01 | Ubiquitous | The system shall power the logic and LEDs from a 5V USB source. | §1, §2 |
| REQ-PWR-02 | Ubiquitous | The system shall power the motor from a dedicated 1.5V AA cell. | §2, §5 |
| REQ-PWR-03 | Ubiquitous | The system shall keep the motor/cell ground electrically disconnected from the Arduino GND. | §1, §5 |
| REQ-PWR-04 | Ubiquitous | The system shall operate with no network or host connection beyond USB power. | CLAUDE.md §1 |

## FW — Firmware behavior
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-FW-01 | Ubiquitous | The firmware shall time all intervals with `millis()` and shall not call `delay()`. | §6 |
| REQ-FW-02 | Event | When a switch reading changes, the firmware shall accept the new state only after it has been stable for a debounce interval between 20 ms and 50 ms. | CLAUDE.md §3 (gap) |
| REQ-FW-03 | Ubiquitous | The firmware shall compute elapsed time with unsigned subtraction so that `millis()` rollover does not alter pulse length. | §6 |
| REQ-FW-04 | Event | When a debounced switch state changes, the firmware shall update the matching LED output within 100 ms. | §6 |

## SAF — Safe state
| ID | Pattern | Requirement | Src |
|---|---|---|---|
| REQ-SAF-01 | Event | When the system powers on, the firmware shall drive all four LED outputs LOW. | §6 |
| REQ-SAF-02 | Event | When the system powers on, the firmware shall hold A1 HIGH from the moment it becomes an output, so the relay never energizes at boot. | §6 (gap) |

## Traceability
| Requirement | Tasks |
|---|---|
| REQ-INP-01, REQ-INP-02 | T-03, T-06, T-09 |
| REQ-INP-03 | T-03, T-04, T-09 |
| REQ-LED-01, REQ-LED-07 | T-07 |
| REQ-LED-02, REQ-LED-03, REQ-LED-04 | T-03, T-09 |
| REQ-LED-05, REQ-LED-06 | T-01, T-02, T-10 |
| REQ-HAP-01, REQ-HAP-02, REQ-HAP-03, REQ-HAP-04, REQ-HAP-05 | T-03, T-11 |
| REQ-REL-01, REQ-REL-03 | T-08 |
| REQ-REL-02 | T-05, T-11 |
| REQ-PWR-01, REQ-PWR-04 | T-03, T-09 |
| REQ-PWR-02, REQ-PWR-03 | T-08, T-12 |
| REQ-FW-01, REQ-FW-03 | T-03, T-11 |
| REQ-FW-02, REQ-FW-04 | T-04, T-09 |
| REQ-SAF-01, REQ-SAF-02 | T-05, T-09 |
