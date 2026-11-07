---
type: synthesis
up: "[[MM6-F4-NPC-Resource-Identity]]"
related: ["[[MM6-F4-V2-Fresh-Child-Owner-Audit]]", "[[MM6-F4-Post-World-Owner-Validator]]"]
tags: [mm6, f4, npc, reload]
---

# MM6 F4 Post-Map Generated NPC Links

The [source-backed packet](../project/MM6_F4_POST_MAP_GENERATED_NPC_LINKS.md)
describes a read-only final actor-to-generated-NPC link check after MM6 map
preparation. It verifies contiguous links, the 100-row bound, cursor agreement
and the generated row fields set by the loader. Three synthetic tests cover
mixed actor classes and late refusals without mutation.

This does not establish saved NPC identity or restore the game RNG stream.
EC6 v2 lacks both, and the existing post-rebuild PID check conservatively
rejects generated actor IDs that differ from saved pre-generation anchors.
Do not use this helper to open MM6 SaveLoad or game-loop input.

See also: [NPC Resource Identity](MM6-F4-NPC-Resource-Identity.md), [Post-World Owner Validator](MM6-F4-Post-World-Owner-Validator.md).
