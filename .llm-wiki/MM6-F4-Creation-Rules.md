---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f4, creation, rules]
---

# MM6 F4 Creation Rules

The first offline MM6 creation-rule seam uses an owner-private profile manifest generated from the corrected F3 PE v2 report. Tracked code stores only the schema and class/skill mapping; the six stat vectors, skill-code vectors and HP/SP factors stay out of Git. The engine loads and validates that manifest before MM6 party creation, while MM7 never reads it.

MM6-only consumers now use six three-tier class families, two granted plus nine eligible creation skills, the shared 50-point stat pool and the executable-backed mana-attribute groups. The six class choices and original class/skill text shapes are wired into Create Party. Source skill ID 30 maps to the runtime Learning slot rather than inherited MM7 Dodge. The MM6 Cleric class no longer takes the inherited no-mana path.

Bodybuilding and Meditation bonuses now use the private MM6 HP/SP class factors instead of the inherited four-ID MM7 factor arrays. [GrayFace’s mechanics reference](https://grayface.github.io/mm/mechanics/) independently supports multiplying Endurance’s effect by the class HP factor and Intellect/Personality effects by the mana factor. The later [max-mana condition trace](MM6-F4-Max-Mana-Condition-Trace.md) maps the outer original formula and corrects highest-priority-condition stat selection for MM6 only. Item/skill helper equivalence, promotion behavior and a character-sheet check remain open.

A bounded read-only trace of the F3-identity-matched MM6 executable confirmed a **minimum maximum health of 1** after its other terms. The MM6 consumer now applies that floor. The exact trace is retained only in the ignored private companion; the other formula terms are still open.

The source table arrays and mana groups passed offline F3 joins; five changed C++ units passed syntax checks and the exporter passed three synthetic tests. A later [Quick Start static audit](MM6-F4-Quick-Start.md) recovered its opening branch and four characters' class, face, name reference, stats and skills; complete inventory, voice, spell state and activation remain open. The [HP/SP initialization audit](MM6-F4-Creation-HP-SP-Init-Gate.md) proved the original Quick Start refill follows starting items and applied that MM6-only timing to current creation, preserving MM7 order. The exact SP bonus formula, promotion/training rules, full UI behavior and `.ec6` persistence remain open. No MM6 process was launched. See the [implementation packet](../project/MM6_F4_CREATION_RULES_FOUNDATION.md), [F3 executable evidence](../project/MM6_F3_EXECUTABLE_FOLLOWUP.md) and [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md).
