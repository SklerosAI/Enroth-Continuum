---
type: concept
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Event-Framing]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, events, skills]
---

# MM6 F4 Skill Event Variables

The [source packet](../project/MM6_F4_SKILL_EVENT_VARIABLES.md) describes
the title-specific MM6 skill event adapter. Raw EVT IDs 56–86 address 31
source skill slots; the last maps to the shared runtime's Learning skill.
The active retained operands use Compare and Set. The MM6 interpreter now
routes those operations through a checked adapter instead of the colliding
MM7 variable enum. Unsupported Add/Subtract and malformed or Grandmaster
Set values refuse before mutation.

Focused unit checks and a null-renderer interpreter scenario passed. The
active 68-resource parser check and an MM7 headless control also passed.
This establishes a bounded command path. Original quest behavior, broader
training rules, production MM6 save/load and visible play remain open.
