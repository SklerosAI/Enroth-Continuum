---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[MM6-F4-Indoor-Runtime]]"]
tags: [mm6, f4, save, maps]
---

# MM6 F4 Map Save Fields

The [F4 map-save packet](../project/MM6_EC6_MAP_FIELDS_F4.md) describes a bounded `MAPS` section in the private `.ec6` container. It carries up to 67 named MM6 delta byte payloads and rejects duplicate or path-like names, invalid sizes, trailing data and container corruption before assigning a decoded candidate. The payloads remain opaque to this codec; the eventual whole-save transaction must validate every map against its original-matched base and atomically replace the user's save file.

The retained offline fixture covers 52 indoor and 15 outdoor deltas byte for byte. The active map still needs a changed-state capture, and the save menu, autosave and relaunch path remain disabled. Read the [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md) and [save specification](../project/MM6_SAVE_SPEC.md) before extending this seam. No MM6 process or original installation was opened here.

The ignored F3 research copy now has a private 67-name manifest derived from the F3 gate, capture, reconciliation, schema signoff and decoded-delta hashes. The public generator prints only counts and validity; its optional candidate-set check rejects missing, extra and duplicate names. C++ also provides an exact-set staging overload. Whole-save restore must load and verify that private manifest, then use the overload before mutating any state. The older shape-only helper remains insufficient for that transaction; source-code wiring, per-base-map validation and live verification are still open.

The MM6-only `MM6MapSaveValidation` pass decodes the sixteen known `.ec6` sections, checks the expected map set, and loads all 67 matching base maps through a caller-supplied trusted reader before returning or assigning state. Indoor deltas are checked against parsed base structure; outdoor base and delta are each parsed within bounds, while their content-level relationship remains unproved. Missing or malformed bases and malformed deltas reject the whole candidate. This pass is offline and not wired to SaveLoad, a user profile or live world mutation.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
