---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f4, creation, rules]
---

# MM6 F4 Creation Rules

The first offline MM6 creation-rule seam uses an owner-private profile manifest generated from the corrected F3 PE v2 report. Tracked code stores only the schema and class/skill mapping; the six stat vectors, skill-code vectors and HP/SP factors stay out of Git. The engine loads and validates that manifest before MM6 party creation, while MM7 never reads it.

MM6-only consumers now use six three-tier class families, two granted plus nine eligible creation skills, the shared 50-point stat pool and the executable-backed mana-attribute groups. The six class choices and original class/skill text shapes are wired into Create Party. Source skill ID 30 maps to the runtime Learning slot rather than inherited MM7 Dodge. The MM6 Cleric class no longer takes the inherited no-mana path.

The source table arrays and mana groups passed offline F3 joins; five changed C++ units passed syntax checks and the exporter passed three synthetic tests. The exact HP/SP bonus formula, Quick Start party, promotion/training rules, starter items/spells, full UI behavior and `.ec6` persistence remain open. No MM6 process was launched. See the [implementation packet](../project/MM6_F4_CREATION_RULES_FOUNDATION.md), [F3 executable evidence](../project/MM6_F3_EXECUTABLE_FOLLOWUP.md) and [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md).
