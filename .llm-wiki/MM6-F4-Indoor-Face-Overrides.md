---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Facet-Events]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, events, indoor, saves]
---

# MM6 F4 Indoor Face Overrides

An optional, versioned IFOV section provides a detached MM6-only fork extension for indoor face changes that the current DLV writer does not save.

IFOV is keyed by canonical `.blv` map name and bounded face index. It stores only attributes and bitmap identity changed from the BLV base, plus a base-face fingerprint. The original BLV and DLV bytes stay untouched. The codec is outside the eighteen required known-save sections and is not wired to live SaveLoad. The underlying original MM6 save rule remains unknown.

The defined fork order is BLV/DLV reconstruction, map `OnMapReload`, then saved IFOV fields. Saved changed fields win; untouched fields retain script effects. Detached capture, full-container staging and application reject malformed or duplicate entries, a different base, and late failure before destination mutation. Three focused linked arm64 unit tests passed. Live texture identity capture, animated faces, retained-map `.ec6` replay and graphical acceptance remain open.

See also: [MM6 Indoor Facet Events](MM6-F4-Indoor-Facet-Events.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), and [implementation packet](../project/MM6_F4_INDOOR_FACE_OVERRIDE_SECTION.md).
