# claude.md: Context & Dogfooding Index

## 1. Architecture Summary
Project Paladar runs an offline Arduino Nano reading 8 pull-up tactile switches to trigger 4 distributed taste channels. Visual feedback illuminates a 20-LED interspersed matrix across a 3D tongue model to accurately represent receptor distribution. Haptic reflex feedback for sour and bitter profiles is actuated via an optically isolated 5V relay driving an independent 1.5V DC motor.

---

## 2. Knowledge Context Index (`/docs`)
Load the documents into the active context when necessary:

* **[`docs/product-vision.md`](docs/product-vision.md)**: Product goals, scope exclusions (no classroom/pedagogical materials), 5–6 target audience parameters, and multi-sensory presentation guidelines.
* **[`docs/architecture.md`](docs/architecture.md)**: BOM, ATmega328P pin mapping (D2–D9 inputs, D10–D12/A0 LED outputs, A1 relay), optocoupler galvanic isolation, and non-blocking `millis()` firmware logic.

* **[`docs/requirements.md`](docs/requirements.md)**: EARS requirements (`REQ-<AREA>-NN`) with frontmatter versioning and a REQ → task traceability table.
* **[`docs/tasks.md`](docs/tasks.md)**: Task checklist, one `## T-NN` block per task. **RAG rule:** read the `active-task` memory first, then load only that task's block and the REQ rows it cites. Never load the whole file.

---

## 2.1 Test Gate (commit/push)
Applies to every task in `docs/tasks.md` that adds or changes code (firmware, scripts, test tooling).

* **Test required:** each such task ships with a generated test covering the REQ IDs it cites.
* **Output:** the test run writes `.test-results/<T-NN>.json` (gitignored):
  `{ "task": "T-NN", "status": "pass" | "fail", "reqs": ["REQ-..."], "tests": <n>, "failed": <n>, "timestamp": "<ISO-8601>" }`
* **Gate:** before any `git commit` or `git push` for the task, the main loop reads that file. Proceed only if it exists, `status` is `pass` and `failed` is `0`, and it is newer than the last change to the task's code or test files.
* **Otherwise:** do not commit or push. Missing, stale or failing result → rerun the test, fix, and re-check. Report the failure to the user.
* **Exempt:** tasks that change only docs, or only hardware/verification steps with no code.

---

## 3. Concise Semantic Query Routing

| Intent / Semantic Query | Target Document | Relevant Sections |
|---|---|---|
| *Target users, educational boundaries, no lesson plans* | `docs/product-vision.md` | `## 2. Problem Statement`, `## 3. Target Audience` |
| *Biological accuracy, debunking tongue zones, receptor spread* | `docs/product-vision.md` | `## 4. Product Capabilities`, `## 5. Experience Mechanics` |
| *Pinout allocation, button pull-ups, LED resistor calculations* | `docs/architecture.md` | `## 2. BOM`, `## 3. Microcontroller Pin Mapping` |
| *Motor protection, optocoupler wiring, 1.5V rail isolation* | `docs/architecture.md` | `## 1. System Topology`, `## 5. Relay & Motor Interfacing` |
| *Firmware concurrency, debouncing, 200ms pulse clamping* | `docs/architecture.md` | `## 6. Real-Time Firmware Specification` |