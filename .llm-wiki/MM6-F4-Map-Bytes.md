---
type: analysis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, map-state]
---

# MM6 F4 Map Bytes

The [map-byte evidence packet](../project/MM6_MAP_BYTES_F4_EVIDENCE.md) separates established EVT ID and wire widths from unproved runtime storage and arithmetic rules.

MMExtension names MM6 EVT IDs 105–204 as MapVar0–99 and exposes `Map.Vars[0..199]` as bytes, with barrel/contest variables described as starting at byte 75. The pinned level editor and project codecs frame a 200-byte DDM/DLV block after the object records. Both pinned editor readers have a copy error at that block, so they do not validate nonzero mappings. The original-matched shipped EVT corpus uses only 51 MapVar IDs, with indices 0–70; all 67 shipped persistent blocks and reset headers are zero.

The lower active range fits a first-75-byte interpretation. The 100-ID namespace may overlap decoration bytes at indices 75–99; no source here proves the exact address. MMExtension documents a common `>=` comparison and an MM6 increment example, but does not define overflow, underflow, out-of-range Set, selector repetition or vanilla reset/save behavior. The existing title-specific fail-closed guard remains correct. A future implementation must first establish those rules with changed-state evidence or executable dispatch, then verify every active operation and persistence route offline and in F5.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Variable Dispatch](Source-MM6-Variable-Dispatch-F2.md), [MM6 World Field Map](Source-MM6-World-Field-Map-F2.md), [Agent Handoff](Agent-Handoff.md).
