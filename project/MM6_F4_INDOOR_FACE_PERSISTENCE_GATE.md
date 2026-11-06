# MM6 indoor face persistence gate

Status: **save/reload semantics unresolved; keep the MM6 transaction closed** (2026-11-06).

The [indoor facet correction](MM6_F4_INDOOR_FACET_EVENTS.md) makes opcode 11 change one indoor face texture and opcode 23 set or clear one indoor face's attribute bits. This packet follows those changes into map loading and the proposed `.ec6` changed-state path. It is a source and retained-resource audit, not a save/reload or graphical result.

## Observed data flow

- `deserializeMM6IndoorBase` reads a face record array and a ten-byte-per-face texture-name array from the BLV base. `IndoorLocation::Load` reconstructs that base afresh on each MM6 indoor entry.
- `deserializeMM6IndoorDelta` reads reset state, visibility, actors, ground items, chests, doors, a 200-byte opaque persistent block and a 56-byte opaque tail. It has no explicit face record or face texture array. The opaque blocks' runtime meanings are not established; the loader preserves them.
- `reconstructMM6IndoorDelta` applies actors, objects, chests, doors and visibility, but does not apply face attributes or textures. `encodeMM6IndoorChangedState` reads the same supported live-state families and does not inspect `location.faces`.
- After map load, `Engine.cpp` invokes `onMapLoad()`, which runs `EVENT_OnMapReload` triggers from the active map's EVT resource. A script may reconstruct an appearance from persistent variables. The current source does not guarantee that every face mutation is replayed.

The F3-matched retained active EVT corpus has 215 texture and 32 face-bit records in 17 listed maps. Six of those maps have no `OnMapReload` record. The other eleven have at least one reload record, but its presence alone does not prove that it reaches every mutated face or reconstructs the exact saved state. Counts here are aggregate; exact records and operands remain in ignored local research.

## Required decision before enabling save

A changed face currently has no demonstrated round trip through the MM6 DLV writer or the proposed `.ec6` map-delta staging path. Do not infer that the opaque DLV blocks encode face edits, and do not mark an indoor map save durable because its DLV bytes round trip. The first check is a title-local, headless mutation → stage → reload fixture for one texture change and both set/clear face-bit operations, including a map without a reload trigger. Confirm the resulting live face index, attribute bits and resolved texture. Then check one scripted reload case so direct state restoration and event replay do not conflict. F5 must inspect a representative graphical face after quit/relaunch.

If original MM6 relies on event replay for a given change, retain that behavior when the relevant variables are durable. If a mutation must survive without replay for the user's `.ec6` save, use a separate, versioned MM6 face-override section keyed by map identity and bounded face index, applied after BLV/DLV reconstruction and before or with a defined precedence relative to `onMapLoad()`. Capture only changed attributes and texture identity, validate count/index/name limits, and reject malformed or unsupported entries. Keep the original BLV/DLV bytes intact and MM7's path untouched. This is a proposed schema, not an implementation claim.

Until that choice and fixture pass, face persistence is an explicit MM6 save gate. The currently disabled MM6 SaveLoad path should stay closed for this reason as well as its other outstanding fields.

See [F4 event framing](MM6_F4_EVENT_FRAMING.md), [save transaction](MM6_ATOMIC_SAVE_F4.md), and [F2 event seams](MM6_EVENT_EXECUTION_SEAMS_F2.md).
