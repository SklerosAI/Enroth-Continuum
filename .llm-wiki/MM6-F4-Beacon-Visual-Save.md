---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-Partial-Stage-Coordinator]]"]
tags: [mm6, f4, save, beacon, image]
---

# MM6 F4 Beacon and Thumbnail Save

The [implementation packet](../project/MM6_F4_BEACON_VISUAL_SAVE.md) describes a standalone `BEAC` v1 `.ec6` section for filename-keyed Lloyd beacons and exact RGBA thumbnail/image bytes.

The inherited MM7 snapshot's fixed Games LOD index table cannot identify an MM6 beacon map safely. The new section writes the lowercase MM6 base-map filename and resolves it against the current MM6 map table plus the caller's verified 67-delta set during detached staging. It carries ordered four-by-five slots, expiry, position, orientation, the current opaque word, 92×68 beacon pixels and an optional 150×112 save thumbnail. Map IDs are derived only after load; no MM7 numeric identity or renderer handle is serialized.

The F3 gate confirms MM6 source/archive identity and the map family, but not a native MM6 beacon/thumbnail whole-save layout. This is current-runtime preservation. Codec and stage reject malformed dimensions, names, nonfinite positions, missing/ambiguous maps, wrong title and truncated or unsupported data. A standalone native probe passed; linked focused tests remain for integration.

`KnownSaveFields` still requires its prior 18 sections and rejects `BEAC`; active beacons still fail closed. Capture of the full Party and thumbnail, expiry/core-clock join, renderer-owned image publication, stable transaction commit, Lloyd spell behavior and graphical save/relaunch/load remain open.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
