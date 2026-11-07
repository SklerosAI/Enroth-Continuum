---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Facet-Events]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, events, indoor, saves]
---

# MM6 F4 Indoor Face Overrides

An optional, versioned IFOV section provides a detached MM6-only fork extension for indoor face changes that the current DLV writer does not save.

IFOV is keyed by canonical `.blv` map name and bounded face index. It stores only attributes and bitmap identity changed from the deterministic pre-event base, plus a base-face fingerprint. That base includes fixed load-time repairs such as `repairClickableFaces`; otherwise they would become spurious overrides. The original BLV and DLV bytes stay untouched. The codec is outside the eighteen required known-save sections and is not wired to live SaveLoad. The underlying original MM6 save rule remains unknown.

The defined fork order is BLV/DLV reconstruction, map `OnMapReload`, then saved IFOV fields. Saved changed fields win; untouched fields retain script effects. Detached capture, full-container staging and application reject malformed or duplicate entries, a different base, and late failure before destination mutation. Three focused linked arm64 unit tests passed. Live texture identity capture, animated faces, retained-map `.ec6` replay and graphical acceptance remain open.

The v2 post-world audit found no safe final-face shortcut: `onMapLoad()` runs
`OnMapReload` before the hook, and the strict nineteen-section candidate carries
neither observed IFOV nor a verified pre-event face base. Final faces alone
cannot separate script edits from unsaved player edits. See the [raw v2
admission packet](../project/MM6_F4_RAW_V2_BEAC_IFOV_ADMISSION.md).

A later detached IFOV writer and separate fresh-child reader passed against
retained private indoor BLV/DLV data. A scoped headless world load also captured
the real indoor face baseline immediately before `OnMapReload` (**1/1**). This
supersedes the earlier lack of a live pre-event base. The checks still do not
write or publish a live indoor `.ec6` slot; indoor gameplay replay,
texture/animation behavior and graphical acceptance remain open.

A strict detached EC6 v6 indoor shell now stages raw v2 with GNPC/RNGS/EVRT,
native actor/sprite/decoration anchors and optional nonempty IFOV. It accepts
25 sections without changed faces or 26 with IFOV and explicitly refuses
gameplay admission. The focused container, v6 stage and atomic suite passed
**25/25**. A fresh disposable v2 seed and separate real-data indoor v6 stage
then passed **1/1** each. The stage used the actual pre-event face baseline,
constructed a candidate only in memory, checked its native anchors and kept
seed bytes unchanged. The earlier test-thread stack crash was resolved by
staging on the established game thread. A later disposable three-process
sequence passed v2 seed, atomic indoor v6 writer and independent fresh-child
reader at **1/1** each. The reader rebuilt the pre-event face base, re-staged
the exact candidate and kept gameplay admission false. Live owner publication,
first input and graphical acceptance remain open.

See also: [MM6 Indoor Facet Events](MM6-F4-Indoor-Facet-Events.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), and [implementation packet](../project/MM6_F4_INDOOR_FACE_OVERRIDE_SECTION.md).
