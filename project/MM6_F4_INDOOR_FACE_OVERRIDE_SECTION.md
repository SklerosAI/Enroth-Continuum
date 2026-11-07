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

The first strict EC6 v6 indoor shell staged a validated raw v2 indoor
session with GNPC/RNGS/EVRT, native indoor actor/sprite/decoration anchors and
an optional nonempty IFOV section. That initial shape accepted 25 sections
without changed faces or 26 with IFOV, preserved v1–v5 decoding, and set
`canEnterGameLoop=false`. Container, v6 stage and atomic-slot focused checks
passed **25/25**. A separate real-data probe used a disposable v2 seed and
loaded one retained BLV/DLV through the actual pre-event checkpoint. It
constructed an indoor v6 candidate in memory, validated its native owner
anchors and controlled IFOV change, and kept the seed bytes unchanged; both
headless children passed **1/1**. Its first attempt crashed on the narrow
GTest-thread stack while staging large raw owners. Running the detached stage
on the established game thread resolved that test-only stack boundary. It
still sets `canEnterGameLoop=false`; atomic v6 write, fresh-child replay and
graphical face behavior remain open.

A subsequent opt-in three-process sequence passed: disposable v2 seed,
atomic test-only `candidate-v6-indoor.ec6` writer, and an independent fresh
child reader, each **1/1**. The reader rebuilt the source DLV and pre-event
face base, re-staged the exact v6 bytes and checked the represented event/NPC
owners and detached IFOV precedence. The v2 seed remained unchanged, and the
v6 slot existed only in the owner-only disposable profile. Both writer and
reader keep `canEnterGameLoop=false`; live owner publication, first input,
production SaveLoad and graphical face behavior remain open.

The current v6 schema adds a required ordered `IDRT` decoration runtime
section, so it now accepts **26 sections** without changed faces or **27 with
IFOV**. It binds each live descriptor, flag word and event ordinal to the
source BLV, EVRT and native IDCR slot; only those mutable fields can be
reapplied to a detached pre-event decoration list. Descriptor zero is a valid
indexed table sentinel when the source name resolves there, not an arbitrary
missing descriptor. Focused linked v6 save-field checks passed **6/6**. The
disposable v2 seed, v6 writer and independent fresh-child reader passed
**1/1 each** with an intentional invisible-bit change and exact detached
replay. Older v1–v5 manifests stay strict, and indoor v6 still refuses
gameplay admission. A new MM6-only one-shot hook applied staged IDRT mutable
fields to live indoor decorations after world preparation and before
`OnMapReload`. A fresh-child reader then recaptured IDRT after map events and
required exact byte equality; the same disposable three-process chain passed
**1/1 each**, with focused linked v6 checks **7/7**. Its first attempt hit
the existing MM6 chest-generation guard, so the test-only reader now stages
the candidate's exact chest owner and arms the already bounded chest map-load
scope. The production guard remains closed. Actor/sprite owner publication,
first input, production SaveLoad and graphical face behavior remain open.

See [indoor face persistence gate](MM6_F4_INDOOR_FACE_PERSISTENCE_GATE.md), [known save sections](MM6_EC6_MAP_FIELDS_F4.md), and [atomic save](MM6_ATOMIC_SAVE_F4.md).
