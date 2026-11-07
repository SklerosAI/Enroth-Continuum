---
type: synthesis
up: "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"
related: ["[[MM6-F4-Pre-Event-GNPC-Replay]]", "[[MM6-F4-EC6-V3-Outdoor-Actor-Map]]"]
tags: [mm6, f4, save, reload, event, headless]
---

# MM6 F4 Pre Event Checkpoint

The [source-backed packet](../project/MM6_F4_PRE_EVENT_CHECKPOINT.md)
records a one-shot MM6 test-only checkpoint after map/actor rebuild and
before generated NPC creation and `onMapLoad()` events.

Its linked unit checks passed 4/4. A disposable v2 starter writer and
separate real Main Menu QuickLoad child passed 1/1 each; the child observed
the fresh pre-event state and aborted before timer unpause or input without
changing its slot. An MM7 headless control passed 1/1. No ACTR/GNPC/RNG
owner was published, and no v3 gameplay session was admitted.

See also: [Pre Event GNPC Replay](MM6-F4-Pre-Event-GNPC-Replay.md), [EC6 V3 Outdoor Actor Map](MM6-F4-EC6-V3-Outdoor-Actor-Map.md), [Post-World Abort Gate](MM6-F4-Post-World-Abort-Gate.md).
