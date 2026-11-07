---
type: synthesis
up: "[[MM6-F4-Generated-NPC-Identity-Save]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, save, rng]
---

# MM6 F4 Gameplay RNG Save

The [source-backed packet](../project/MM6_F4_GAMEPLAY_RNG_SAVE.md) defines a
detached `RNGS` candidate that preserves the selected gameplay random engine's
exact current state.

Mersenne Twister state is captured as bounded classic-locale text and parsed
into a temporary engine before restore; the sequential test engine retains its
32-bit counter. `EngineRandomComponent` captures the underlying selected
gameplay engine even when tracing is enabled and refuses a type mismatch on
restore. The versioned section validates malformed state and is recognized by
the container, while strict EC6 v2 rejects it as an extra tag. `vrng` remains
visual-only and is not in this gameplay capsule. Syntax, style and a standalone
MT continuation check passed; linked integration validation is pending.

The unresolved gate is **when** to restore. NPC generation and `onMapLoad()`
run before the post-world hook; a late RNG restore cannot undo a random event
or replace an identity that an event already observed. `GNPC` supplies a
detached identity candidate, but neither section is wired to live MM6 load.

See also: [Generated NPC Identity Save](MM6-F4-Generated-NPC-Identity-Save.md), [V2 Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md).
