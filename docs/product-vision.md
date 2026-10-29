# Product Vision: Project Paladar (Interactive Sensory Demonstrator)

## 1. Executive Summary & Vision Statement
Project Paladar is an offline, tangible physical computing demonstrator designed to introduce preschool children (ages 5–6) to the distributed biology of the human gustatory system. Engineered strictly as a live, presenter-guided demonstration unit rather than a classroom curriculum tool, Paladar combines tactile food triggers, an illuminated 3D tongue representation, and sensory vibration feedback to deliver an immediate, multi-sensory learning experience without network reliance.

## 2. Problem Statement & Scope Definition
* **Static Sensory Explanations:** Explaining abstract receptor-level biology to 5–6-year-olds using flat 2D graphics or lectures fails to build intuitive understanding.
* **Perpetuation of Scientific Myths:** Educational props often reinforce the debunked "tongue map" myth (isolated taste zones), missing the chance to demonstrate how taste receptors are actually distributed across the tongue.
* **Explicit Scope Exclusions:** Paladar is **not** a student-facing kit for unguided exploration, nor is it an instructional toolkit for classroom teachers. Consequently, lesson plans, curricular matrices, and teacher manuals are out of scope. The unit is designed solely for structured, presenter-led demonstrations.
* **Operational Independence:** The demonstrator must function with zero boot delay, zero network configuration, and no reliance on Wi-Fi, Bluetooth, or external software platforms.

## 3. Target Audience & Presentation Context
* **Primary Spectators / Participants:** Preschool children (ages 5 to 6 years) engaging in guided interactive sessions.
* **Operator / Facilitator:** Presenters and exhibitors conducting hands-on sensory demonstrations.
* **Pedagogical & Experiential Goals:** Direct cause-and-effect reasoning, somatic awareness through vibration, and visual debunking of localized taste compartments.

## 4. High-Level Product Capabilities
* **Offline Physical Interaction:** Autonomous operation requiring only local power.
* **Multi-Sensory Latency Standard:** Imperceptible delay (< 50 ms) between tactile trigger engagement and concurrent visual/haptic output.
* **Biologically Accurate Feedback:** Visual illumination must demonstrate that taste reception happens across the entire tongue via distributed receptor populations, rather than isolated geographical quadrants.
* **Somatic Reflex Accentuation:** Tactile vibration must reinforce high-intensity taste categories (such as sourness and bitterness) to represent natural physiological protective responses.

## 5. Experience Mechanics
1. **Interactive Presentation Console:** A durable physical surface presenting distinct, recognizable food illustrations categorized by the primary taste families.
2. **Tactile Inputs:** Momentary mechanical triggers positioned beneath each food graphic, sized for young children.
3. **Physical 3D Tongue Substrate:**
   * **Interspersed Optical Matrix:** A distributed light network representing distinct receptor cell types across all lingual regions (anterior, lateral, and posterior).
   * **Enclosed Haptic Mechanism:** An internal physical vibration element providing transient haptic pulses.
4. **Demonstrator Behavior:**
   * Activating any food trigger illuminates the matching receptor network across the whole tongue surface, demonstrating distributed reception.
   * Selecting foods associated with defensive or intense gustatory reflexes (sour/bitter) triggers an accompanying mechanical vibration pulse.

## 6. System Safety & Operating Constraints
* **Complete Actuator Isolation:** Any electromechanical vibration element must be galvanically isolated from sensitive control logic to prevent electrical brownouts or component strain.
* **Physical Child Safety:** All mechanical moving parts and high-current paths must remain fully encapsulated within the structure to eliminate pinch points and external contact hazards.
* **Input Concurrency Handling:** Control firmware must handle erratic, rapid, or simultaneous button presses predictably without interface lockups.

## 7. AI Governance & Lifecycle Separation
* **Claude (Internal / Dogfooding via CAG):** Context-Augmented Generation for reviewing hardware implementations, enforcing electrical isolation requirements, and auditing architecture against product goals.
* **Gemini (External to Codebase):** Confined to visual asset conceptualization, booth layout ideas, and presenter demo narrative scripts, remaining strictly separated from repository version control.