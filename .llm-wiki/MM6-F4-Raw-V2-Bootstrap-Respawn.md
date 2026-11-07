---
type: synthesis
up: "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"
related: ["[[MM6-F4-Post-World-Owner-Validator]]", "[[MM6-F4-Raw-Save-Runtime]]"]
tags: [mm6, f4, save, reload, headless]
---

# MM6 F4 Raw V2 Bootstrap Respawn

The [source-backed packet](../project/MM6_F4_RAW_V2_BOOTSTRAP_RESPAWN.md)
separates the pre-entry native v2 New Game seed from a durable populated
first-map save.

A disposable fresh child selected the exact v2 primary through real Main Menu
QuickLoad, published represented owners only inside the test process, rebuilt
the first outdoor map, verified its empty cursor and aborted before input.
Writer and child each passed 1/1 with the slot unchanged. The final PID-slot
check refused because `lastRespawnDay == 0` causes initial encounter spawning
after the saved DDM actors are loaded. This is an expected guarded refusal,
not proof of playable v2 reload.

The current MM6 changed-state writer rejects new actor counts, and the native
actor projection cannot synthesize all bytes of spawned records. A durable
post-entry actor owner and no-respawn reload contract are the next gates.
The [detached actor append codec](MM6-F4-Outdoor-Actor-Append.md) solves only
the bounded native DDM splice for complete supplied records.
Generated NPC/RNG, chest/event and resource/UI admission remain separate.

See also: [Post-World Owner Validator](MM6-F4-Post-World-Owner-Validator.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Agent Handoff](Agent-Handoff.md).
