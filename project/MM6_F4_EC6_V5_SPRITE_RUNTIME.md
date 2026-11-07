# MM6 EC6 v5 runtime sprite owner

Status, 2026-11-07: source-backed design gate; no accepted v5 codec, slot or
replay yet. A bounded v4 first-map reload and real-loop turn/walk passed, but
the attempted second v4 write refused before disk mutation when a live ground
sprite had fractional X/Y position. The native 100-byte DDM sprite record
stores integer position. The current v4 writer correctly rejects narrowing.

## Authority and order

`SpriteObject` holds float position, velocity and initial position, runtime
timers/particle clocks, spell and PID fields, and an `Item`. The inherited
`SpriteObject_MM7` snapshot narrows positions to integer and velocity to
16-bit coordinates. `SpriteObject::updateObjectODM()` changes live floats on
gameplay ticks. `SpriteObject::Remove()` clears a descriptor but can leave
coordinates. Waiting for apparent rest is therefore not an exact save rule.

MM6 `MAPS` and `CHST` must retain the original native ground records and
their opaque MM6 item bytes as source anchors. A v5 capture path must update
supported reset, visibility and chest fields without projecting live sprite
floats back into native DDM. The current
`captureOutdoorDeltaWithActiveActors()` is a v4 strict writer and should
remain unchanged for v4.

Outdoor loading reconstructs DDM sprite objects, then
`arrangeSpriteObjects()` post-generates item data and floor-snaps map-placed
objects. The test-only runtime overlay belongs after that arrangement and
before `_461103_load_level_sub()` and `onMapLoad()`, alongside ACTR/GNPC
pre-event replay. Restore RNGS at its existing proven point before map
events. Check the full sprite list again after events and before input.

## Conservative v5 contract

1. Add a required `SPRT` section to a new exact manifest that wraps v4's 23
   sections. V1 through v4 decoders and refusal behavior remain strict.
2. Encode every live sprite in ordered PID slot sequence, with exact float
   bits for all three vectors, every runtime scalar and timer, particle
   clocks, spell/caster/target fields, and containing-item provenance. Reject
   unsupported nonempty or mutated item forms instead of narrowing them.
3. Bind the section to `HEAD`, the active native DDM ground record count and
   each stable source slot/identity. Preserve native 100-byte records and
   28-byte MM6 item records separately. Initially refuse slot compaction,
   reuse and append until the source relationship is proved.
4. Preflight native `MAPS`/`CHST` independently from exact runtime `SPRT`.
   The native integer coordinates are source anchors, not post-action
   runtime coordinates.
5. Stage fully and validate before test-only publication. A mismatch aborts
   before events/input; normal MM6 and all MM7 routes remain unarmed.

## Verification gates

- Exact encode/decode/re-encode with float bit fidelity, item provenance,
  count/order/anchor and foreign-schema refusals.
- Post-action first-map writer with a strict v5 semantic callback and atomic
  disposable slot; no v4 fallback or silent projection.
- Fresh-child pre-event SPRT overlay, post-event full-list equality, saved
  movement-state check, and preserved source slots.
- MM7 headless control, owner-only temporary profile, and unchanged v2/v4
  source files. Only after this passes consider production save/load admission,
  indoor and cross-map work.

See [v4 event runtime boundary](MM6_F4_EC6_V4_EVENT_RUNTIME.md),
[active actor capture](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md) and
[save specification](MM6_SAVE_SPEC.md).
