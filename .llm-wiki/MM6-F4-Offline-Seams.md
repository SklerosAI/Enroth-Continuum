---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f4, implementation, handoff]
---

# MM6 F4 Offline Seams

F4 implementation is underway after the [F3 original-data gate](../project/MM6_F3_GATE_REVIEW.md). The source tree has **partial offline seams**, not a playable campaign. No partial MM6 game was launched or Atlas Play action enabled for these changes.

| Seam | Code/evidence now present | Next blocking work |
| --- | --- | --- |
| Separate Mac runtime | The `MM6.app` arm64 target has its own bundle identifier and `mm6`-locked CLI. Its default Mac data lookup targets the private MM6 app folder; MM7's target and bundle identity still build. | Finish title-specific rules and `.ec6` saves, then F5 play before Atlas stages this helper. The build target itself is not a working installation. |
| Games LOD order and travel map lookup | `LodReader` retains on-disk order; the MM6 map lookup uses checked one-based directory positions. The corrected private PE v2 report joins 36/36 static destinations with this order. | Load actual MM6 travel records, weekday/quest/fare/blank-slot policy and UI dispatch; observe travel in F5. |
| Monster descriptor tail | MM6 retains all 173 descriptor rows; the [actor fixture](../scripts/mm6_f4_check_actor_descriptors.py) checks the original-matched baseline ID 173 reference across 67 deltas. | Actor runtime decode, encounter behavior, AI/loot and F5 visual/save checks. |
| Vanilla portraits | The [portrait seam](MM6-F4-Portrait-Seam.md) uses twelve face families and 53 frames per face, with title-specific cycling. | Original creation defaults, expressions, voices, rendered portraits and save persistence. |
| Creation and class rules | The [creation packet](MM6-F4-Creation-Rules.md) uses a private F3-derived profile manifest for six starting families, stats, skills and mana dispatch. | Exact HP/SP, Quick Start, promotion, training and graphical creation checks. |
| Indoor and outdoor files | Bounded wire readers preserve opaque bytes for 52 indoor and 15 outdoor retained map/delta pairs; the outdoor base adapter now preflights model bounds. | Convert MM6 delta fields to runtime objects, track changes and persist them in `.ec6`; inspect maps in F5. |
| EVT resource framing | The [event packet](../project/MM6_F4_EVENT_FRAMING.md) records 68/68 active retained resources parsed in C++. The zero-mastery skill-check branch has a provisional MM6 rule. Twelve texture and six all-faces facet targets were checked against retained outdoor models; the executor applies them to runtime faces. A Question prompt and two-answer continuation path is implemented offline. | Remaining opcode effects, 127 variable IDs, Question graphical/branch checks and campaign persistence tests. Texture and facet changes are not yet saved or reapplied. Three unknown operations and monster spawn still reject execution explicitly. |
| EVT variable guard | The [variable packet](../project/MM6_F4_VARIABLE_GUARD.md) classifies all 127 active IDs by MM6 range. Title-specific IDs cannot fall through to MM7's colliding executor; 100 map-byte IDs have checked zero-based indexes. | Implement each active meaning and operation, prove the MM6 persistent-byte layout, then connect `.ec6` and F5 branch/save checks. Shared-looking IDs 1–45 remain provisional. |
| `.ec6` envelope | The [container packet](../project/MM6_EC6_CONTAINER_F4.md) defines a bounded versioned title marker and checked sections; three focused unit tests pass. | Per-field state schema, mandatory sections, atomic file writes, slot listing and F5 save/quit/relaunch/load. |

The integrated native `MM6.app` and unit targets built on this Mac. Nineteen focused C++ tests passed across event framing, the zero-mastery branch and Question answer matching, the variable guard, indoor/outdoor wire readers, container integrity, monster descriptor bounds and directory order; the event test includes all 68 active retained EVT resources and the indoor test checks all 52 map pairs when supplied with ignored local input. Three creation exporter tests and the private actor fixture also passed. Original game data and exact per-ID reports remain ignored under `local-game-data/`.

For a fresh agent, start with [Agent Handoff](Agent-Handoff.md), the [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md), [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md), the [private report registry](Private-Companion-Wiki.md), and the relevant packet above. Use a separate worktree for concurrent code; the protected MM7 and reserved MM8 checkouts are not implementation scratch space. A parser pass, successful build or synthetic fixture does not satisfy F5.
