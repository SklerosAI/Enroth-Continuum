---
type: analysis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, map-state]
---

# MM6 F4 Map Bytes

The [map-byte evidence packet](../project/MM6_MAP_BYTES_F4_EVIDENCE.md) separates established EVT ID and wire widths from the bounded F4 runtime mapping and unproved persistence/arithmetic edges.

MMExtension names MM6 EVT IDs 105–204 as MapVar0–99 and exposes `Map.Vars[0..199]` as bytes, with barrel/contest variables described as starting at byte 75. The pinned level editor and project codecs frame a 200-byte DDM/DLV block after the object records. Both pinned editor readers have a copy error at that block, so they do not validate nonzero mappings. The original-matched shipped EVT corpus uses only 51 MapVar IDs, with indices 0–70; all 67 shipped persistent blocks and reset headers are zero.

The lower active range fits a first-75-byte interpretation. The F4 interpreter now applies IDs 105–175 once per event to bytes 0–70 of the retained DDM/DLV persistent section. It uses unsigned `>=` comparison and bounded byte Set/Add/Subtract. Overflow, underflow, and out-of-byte operands fail closed; IDs 176–204 and other title-specific families remain guarded. Offline synthetic tests verify nonzero mutation and preservation of upper persistent bytes and the tail. The bounded outdoor DDM changed-state writer starts from this current sidecar and preserves the MapVar bytes; its SaveLoad integration remains open. The mapping and relation remain source-backed candidates rather than executable MM6 proof. The 100-ID namespace may overlap decoration bytes at indices 75–99; no source here proves the exact address. Changed-state save, leave/re-entry, timed reset, exact arithmetic edges and live campaign verification remain outstanding.

The [detached persistent event boundary](../project/MM6_F4_PERSISTENT_EVENT_BOUNDARY.md) now stages only proved bytes 0–70 from validated active DLV/DDM and exposes a zeroed inherited 75/125 live-array shape without publishing it. It rejects nonzero upper 129 wire bytes and nonzero generic-array writes on capture. This is a fail-closed adapter for the currently evidenced state, not proof that the MM6 bytes correspond to MM7's `mapVars` and `decorVars`. Decoration visibility, `EVENT_ChangeEvent`, and outdoor overlay effects still consume or mutate the generic arrays, so their MM6 addresses and reset behavior gate production publication.

The represented v2 session preflight now requires this boundary before selecting a primary or backup. A primary with an unproved nonzero upper byte falls back to a valid backup without disk repair; the integrated persistent/raw checks passed **13/13**. This still does not assign the inherited arrays or prove upper-byte semantics.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Variable Dispatch](Source-MM6-Variable-Dispatch-F2.md), [MM6 World Field Map](Source-MM6-World-Field-Map-F2.md), [Agent Handoff](Agent-Handoff.md).
