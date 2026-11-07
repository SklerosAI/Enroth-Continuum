# MM6 indoor face override section

Status: **detached `.ec6` IFOV codec plus fresh-child data check and real pre-event face-base capture; live indoor save/application remains gated** (2026-11-07).

`IFOV` is an Enroth Continuum fork extension for indoor MM6 face changes. The [persistence gate](MM6_F4_INDOOR_FACE_PERSISTENCE_GATE.md) proved that the current DLV writer drops a synthetic texture and face-bit edit unless a reload script recreates it. Original MM6 save semantics are not established. This extension keeps the original BLV/DLV bytes intact and is not an assertion about the original game's file format.

## Version 1 contract

The optional section has its own version byte. It stores one to 52 maps in strict canonical `.blv` basename order. Each map records its base face count, a deterministic 64-bit fingerprint of the base attributes and texture identities, and sorted unique changed face indices. Each face entry stores changed raw attribute bits, a changed canonical bitmap identity, or both. Empty bitmap identity is representable. The fingerprint detects accidental base mismatch; the separate private 67-map manifest remains the source identity gate. No renderer pointer, original map bytes or texture bitmap is embedded.

| Payload field | Encoding |
| --- | --- |
| Header | version `u8=1`, map count `u8` |
| Per map | name length `u8`, canonical name bytes, base face count `u32`, base fingerprint `u64`, override count `u32` |
| Per changed face | index `u32`, flags `u8` (`1` attributes, `2` texture), optional raw attributes `u32`, optional texture length `u8` and canonical bytes |

Integers are little-endian. Map names and indices are strictly ascending; empty or duplicate changed-field entries reject. The fingerprint mixes each base face's raw attributes, texture length and canonical texture bytes in face-index order with FNV-1a 64-bit arithmetic. It is a mismatch detector, not a substitute for the verified private map manifest.

The codec bounds a map name to 16 bytes, a texture identity to 16 bytes, face count to 65,535, and section payload to 2 MiB. Only lower-case ASCII letters, digits, underscore and hyphen are accepted for map stems; bitmap identities additionally permit `$`. A read-only aggregate check of the 52 F3-retained indoor bases found a maximum of 5,385 faces and nine-byte texture identities; five base texture keys use `$`. The 215 active texture-event records use at most nine bytes after case normalization. Exact names and map operands remain ignored locally.

`captureIndoorFaceOverrides` compares detached base and current `IndoorFaceState` arrays and emits only changed fields. The base-state callback must produce the deterministic state immediately before `OnMapReload`, including `repairClickableFaces`; using raw BLV attributes would misclassify fixed load-time repairs as edits. A future live adapter must supply stable canonical texture identity, including for animated faces, or reject capture. `encodeIndoorFaceOverrideSection` and `decodeIndoorFaceOverrideSection` are separate from the eighteen required known-save fields. `loadIndoorFaceOverridesBeforeMutation` decodes the full container and validates every referenced base through a caller-supplied reader before assigning its detached destination. Malformed lengths, versions, flags, duplicate maps/indices, out-of-range indices, stale bases and late failures reject. No live engine object is modified by this path.

## Reload precedence

For this fork extension, BLV and DLV reconstruction run first, then the map's `OnMapReload` events, then the staged IFOV changes. A saved override wins for fields it explicitly changed from the base; a reload script keeps control of untouched fields. `applyIndoorFaceOverridesAfterMapReload` models this rule on detached face states and swaps the complete candidate only after validation. The live `IndoorLocation` adapter and all-owner SaveLoad transaction must preserve this order; they are not connected here. MM7 never reads IFOV.

The focused arm64 unit suite passed three tests: detached capture/section round trip/post-reload precedence, malformed and duplicate rejection, and late second-map failure without destination mutation. This does not prove asset resolution, animated-face identity, a real retained-map `.ec6` quit/relaunch, or a graphical face. Keep normal MM6 SaveLoad guarded until those and the remaining transaction fields pass.

A later detached writer and separate fresh-child reader passed against retained
private BLV/DLV data. A scoped headless world load captured the loaded indoor
face base immediately before `OnMapReload` (**1/1**), including the normal
load-time repair step. This supplies the missing timing anchor without
publishing a live save. Indoor v6 must still bind that baseline and IFOV to
the complete session transaction; older v1–v5 containers remain strict.

An initial strict EC6 v6 indoor shell now stages a validated raw v2 indoor
session with GNPC/RNGS/EVRT, native indoor actor/sprite/decoration anchors and
an optional nonempty IFOV section. It accepts 25 sections without changed
faces or 26 with IFOV, preserves v1–v5 decoding, and sets
`canEnterGameLoop=false`. Container, v6 stage and atomic-slot focused checks
passed **25/25**. This is detached synthetic staging; a real private indoor
stage probe, live save/write and fresh-child replay are still open.

See [indoor face persistence gate](MM6_F4_INDOOR_FACE_PERSISTENCE_GATE.md), [known save sections](MM6_EC6_MAP_FIELDS_F4.md), and [atomic save](MM6_ATOMIC_SAVE_F4.md).
