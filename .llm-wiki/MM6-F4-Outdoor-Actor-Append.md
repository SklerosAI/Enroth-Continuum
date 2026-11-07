---
type: synthesis
up: "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"
related: ["[[MM6-F4-Raw-V2-Bootstrap-Respawn]]", "[[MM6-F4-Post-World-Owner-Validator]]"]
tags: [mm6, f4, save, outdoor, actor]
---

# MM6 F4 Outdoor Actor Append

The [source-backed packet](../project/MM6_F4_OUTDOOR_ACTOR_APPEND.md)
records a detached native DDM actor-section splice that preserves original
actor and opaque suffix bytes while appending complete supplied records.

The codec accepts a zero or already matching source respawn day, bounded final actor
slots, exact 548-byte supplied records and matching per-slot native/runtime
anchors. It changes the actor count and respawn day and re-decodes the
result. Its linked synthetic suite passed 3/3. The disposable native v2
writer and fresh-child guarded refusal passed 1/1 each after integration.

No full native runtime capture adapter exists for newly spawned actors, so
this structural slice cannot yet save a populated map. The strict v2
manifest, generated NPC and RNG publication order, and other final-owner
checks remain open. The [bootstrap refusal](MM6-F4-Raw-V2-Bootstrap-Respawn.md)
is still the correct result for a pre-entry save.

See also: [Bootstrap Respawn](MM6-F4-Raw-V2-Bootstrap-Respawn.md), [Post-World Owner Validator](MM6-F4-Post-World-Owner-Validator.md), [Agent Handoff](Agent-Handoff.md).
