---
type: synthesis
up: "[[MM6-F4-Spawned-Actor-Owner-Decision]]"
related: ["[[MM6-F4-Raw-V2-Bootstrap-Respawn]]", "[[MM6-F4-Pre-Event-GNPC-Replay]]"]
tags: [mm6, f4, save, actor]
---

# MM6 F4 Spawned Actor Section

The [source-backed packet](../project/MM6_F4_SPAWNED_ACTOR_SECTION.md)
records detached `SPAW` v1 for newly spawned MM6 runtime actors at exact PID
slots. It includes MM6 resistances and runtime-only state alongside a
canonical fixed actor snapshot; it is not a native DDM record.

The linked unit suite passed 4/4, including the canonical zero-row form and
unsupported-state refusals. A private-backed, fresh-process headless
first-map capture passed 1/1 for encode/decode/re-encode without writing a
profile. This establishes a narrow pristine-spawn capture only. Items,
buffs, later actor mutations, complete save/reload, v3 manifest admission and
pre-event publication remain open.

See also: [Spawned Actor Owner Decision](MM6-F4-Spawned-Actor-Owner-Decision.md), [Pre Event GNPC Replay](MM6-F4-Pre-Event-GNPC-Replay.md), [Bootstrap Respawn](MM6-F4-Raw-V2-Bootstrap-Respawn.md).
