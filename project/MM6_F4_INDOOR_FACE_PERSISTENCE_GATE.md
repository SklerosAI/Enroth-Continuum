# MM6 indoor face persistence gate

Status: **synthetic headless DLV/reload order verified; original save/reload semantics unresolved, so keep the MM6 transaction closed** (2026-11-06).

The [indoor facet correction](MM6_F4_INDOOR_FACET_EVENTS.md) makes opcode 11 change one indoor face texture and opcode 23 set or clear one indoor face's attribute bits. This packet follows those changes into map loading and the proposed `.ec6` changed-state path. It is a source and retained-resource audit, not a save/reload or graphical result.

## Observed data flow

- `deserializeMM6IndoorBase` reads a face record array and a ten-byte-per-face texture-name array from the BLV base. `IndoorLocation::Load` reconstructs that base afresh on each MM6 indoor entry.
- `deserializeMM6IndoorDelta` reads reset state, visibility, actors, ground items, chests, doors, a 200-byte opaque persistent block and a 56-byte opaque tail. It has no explicit face record or face texture array. The opaque blocks' runtime meanings are not established; the loader preserves them.
- `reconstructMM6IndoorDelta` applies actors, objects, chests, doors and visibility, but does not apply face attributes or textures. `encodeMM6IndoorChangedState` reads the same supported live-state families and does not inspect `location.faces`.
- After map load, `Engine.cpp` invokes `onMapLoad()`, which runs `EVENT_OnMapReload` triggers from the active map's EVT resource. A script may reconstruct an appearance from persistent variables. The current source does not guarantee that every face mutation is replayed.

The F3-matched retained active EVT corpus has 215 texture and 32 face-bit records in 17 listed maps. Six of those maps have no `OnMapReload` record. The other eleven have at least one reload record, but its presence alone does not prove that it reaches every mutated face or reconstructs the exact saved state. Counts here are aggregate; exact records and operands remain in ignored local research.

## Headless fixture result

`MM6IndoorFacetEvent.ChangedFacesRequireReloadReplayOrSeparateSaveState` uses synthetic faces and DLV bytes in the null-renderer MM6 GameTest. It executes face-bit set, clear and set again plus a texture change on one face. The current changed-state writer emits byte-identical DLV data. Resetting the faces to base state and reconstructing that DLV leaves the face unchanged. With no reload trigger, `onMapLoad()` also leaves it unchanged. A synthetic reload script then re-applies the bit and texture after DLV reconstruction. Both indoor facet GameTests passed in a separately linked, headless arm64 test executable; source and test compilation passed. No installed save, private map, graphical frame or original executable was involved in this fixture.

## Required decision before enabling save

A changed face has a demonstrated **negative** round trip through the current MM6 DLV writer: the synthetic headless fixture loses the edit when no script replays it. Do not infer that the opaque DLV blocks encode face edits, and do not mark an indoor map save durable because its DLV bytes round trip. Next, stage a real retained indoor map through `.ec6`, with one active no-reload case and one scripted-reload case. Check original MM6 save/load behavior or the executable's DLV writer before choosing parity versus a deliberate fork extension. F5 must inspect a representative graphical face after quit/relaunch.

If original MM6 relies on event replay for a given change, retain that behavior when the relevant variables are durable. If a mutation must survive without replay for the user's `.ec6` save, use a separate, versioned MM6 face-override section keyed by map identity and bounded face index, applied after BLV/DLV reconstruction and before or with a defined precedence relative to `onMapLoad()`. Capture only changed attributes and texture identity, validate count/index/name limits, and reject malformed or unsupported entries. Keep the original BLV/DLV bytes intact and MM7's path untouched. This is a proposed schema, not an implementation claim.

Until that choice and fixture pass, face persistence is an explicit MM6 save gate. The currently disabled MM6 SaveLoad path should stay closed for this reason as well as its other outstanding fields.

See [F4 event framing](MM6_F4_EVENT_FRAMING.md), [save transaction](MM6_ATOMIC_SAVE_F4.md), and [F2 event seams](MM6_EVENT_EXECUTION_SEAMS_F2.md).
