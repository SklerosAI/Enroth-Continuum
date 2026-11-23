---
type: analysis
up: "[[Agent-Handoff]]"
derived_from: ["[[Campaign-Status]]", "[[Runtime-Isolation]]", "[[MM7-Combat-Audit-Fixes]]"]
tags: [mm7, mm8, gaps, handoff]
---

# Open Questions

Open MM7 and cross-title questions. MM6 open questions live in `docs/open-questions.md` on `claude/mm6`.

## MM7 open questions — 2026-11-23

- **Full audit start.** The audit plan names the week of 2026-11-23, the delivery order puts the audit after the MM6 working gate. It has not started. Owner's call.
- **Strange Temple exit.** Event 501 carries an out-of-bounds Harmondale X, so the engine loads the map but skips the placement. A private native trace of the settled position decides between clamping, travel handling or no change. `MM7StrangeTemple` pins today's bounded landing.
- **5:00 AM sunlight.** MM7 keeps the `UpdateSunlightVectors` minute check that MM6 fixed behind `fix_sunrise_light` on `claude/mm6`. `src/Engine/Graphics/Outdoor.cpp`.
- **Wall pressure plates.** Original MM7 fires a pressure plate the party runs into. Upstream's fix for #2244 dropped wall plates along with the lift retrigger, and MM7 keeps upstream's behaviour.
- **Scroll targeting exits.** Cancel returns the scroll. Attacking, passing the turn, stealing, a full spell queue and turn-based wait still use it up.
- **2026-11-07 audit defects still open.** Wand charge ordering, Monster Haste and Day of Protection never read, Hour of Power reading Shield's power, `_SelectTarget`, the indoor summon floor, the Assassins' and Barbarians' enchantments, artifacts 530, 531 and 536, two grandmaster teacher requirements and the Disarm Trap caps. See [MM7 Combat Audit Fixes](MM7-Combat-Audit-Fixes.md).
- **Upstream issues without a fork fix.** #2068, #1560 and #2276, among others listed in the audit results.
- **In-game mod panel.** A read-only panel of the Atlas launch options is on the owner's backlog.

## Cross-title questions

- **Agent play and observation.** Once a title passes its gameplay and save gates, what can a controller use to act, observe and recover without changing hidden state or another title's profile? Today's screenshot, input and trace hooks are test-only, not an external interface. See the [knowledge and control contract](Agent-Play-Knowledge-and-Control-Contract.md) and the [roadmap](Agent-Play-and-Observation-Roadmap.md).
- **MM8.** No MM8 engine adapter exists. The Atlas detects staged MM8 data and offers no Play action.
- **Doc layouts.** `claude/mm6` keeps its docs in `docs/`, `codex/mm7` in `.llm-wiki/`, `project/` and `agent_comms/`. They are reconciled when the branches next meet on `master`.

See also: [Agent Handoff](Agent-Handoff.md), [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
