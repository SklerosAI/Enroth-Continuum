# MM6 EC6 v5 runtime actor and sprite owners

Status, 2026-11-07: detached v5 `SPRT` and `ARTR` full-list codecs, exact
25-section manifest, test-only pre-event scopes and atomic candidate shell
passed focused linked checks **11/11**. No v5 slot or live replay yet.
A bounded v4 first-map reload and real-loop turn/walk passed, but
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

1. Required `SPRT` and `ARTR` sections now wrap v4's 23 sections in an exact
   25-section v5 manifest. V1 through v4 decoders and refusal behavior remain
   strict; linked manifest and atomic checks passed.
2. The detached codec encodes every live sprite in ordered PID slot sequence, with exact float
   bits for all three vectors, every runtime scalar and timer, particle
   clocks, spell/caster/target fields, and containing-item provenance. Reject
   unsupported nonempty or mutated item forms instead of narrowing them.
   Its linked fractional round-trip and malformed/provenance refusals passed.
3. Bind the section to `HEAD`, the active native DDM ground record count and
   each stable source slot/identity. Preserve native 100-byte records and
   28-byte MM6 item records separately. Initially refuse slot compaction,
   reuse and append until the source relationship is proved.
4. Preflight native `MAPS`/`CHST` independently from exact runtime `SPRT`.
   The native integer coordinates are source anchors, not post-action
   runtime coordinates.
5. `ARTR` owns the full ordered runtime actor list with exact float vectors
   and widened values that ACTR v1 narrows. The v4 ACTR payload remains an
   unchanged source baseline bound to native DDM slots. After real-loop
   movement, the v5 writer passed SPRT capture but ACTR v1 refused a
   fractional actor position; the new ARTR path retains that state without
   weakening ACTR v1.
6. Stage fully and validate before test-only publication. A mismatch aborts
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

The first live v5 post-action writer with ARTR and SPRT now reaches a later
strict refusal: a decoration flag changed during gameplay, while v4 EVRT
records event arrays and bindings but not all final decoration flags. No v5
slot was written. A separate ordered decoration runtime owner is under
audit; it must bind stable base identities and restore after decoration
preparation but before map-event registration. The v5 manifest is still
unpublished and may gain this required owner before any accepted slot.

See [v4 event runtime boundary](MM6_F4_EC6_V4_EVENT_RUNTIME.md),
[active actor capture](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md) and
[save specification](MM6_SAVE_SPEC.md).
