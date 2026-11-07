---
type: synthesis
up: "[[MM6-F4-EC6-V3-Manifest]]"
related: ["[[MM6-F4-V3-Post-Entry-Map-and-Pre-Event]]", "[[MM6-F4-EC6-V3-Full-Session-Preflight]]", "[[MM6-F4-Map-Bytes]]"]
tags: [mm6, f4, save, event, ec6, reload]
---

# MM6 F4 EC6 V4 Event Runtime

The [source-backed packet](../project/MM6_F4_EC6_V4_EVENT_RUNTIME.md)
records a detached `EVRT` runtime owner and a test-only restore point before
decoration preparation. It responds to the no-write v3 candidate's
`event-decor-bound-high` refusal without asserting a mapping to native MM6
event bytes.

`EVRT` stores exact 75 `mapVars` and 125 `decorVars` bytes for each visited
map, native respawn count/day anchors and source-order bindings for the first
124 interactive decorations without explicit event IDs. The bindings retain
decoration index, descriptor, cog and assigned event variable ID. Restore
requires exact active map, native reset header and rebuilt decoration
bindings. Native DDM/DLV 200-byte event data remains independently retained
in `MAPS`.

The current index/descriptor/cog/slot binding cannot distinguish two
same-type decorations swapped in position. Exact decoration identity must
be strengthened or proved before a durable v4 admission.

The exact EC6 v4 manifest has 23 required sections: v3's 22 plus `EVRT`.
V1–v3 exact decoders remain strict. The test-only MM6 scope validates a
no-respawn reload before copying the two arrays ahead of outdoor or indoor
decoration preparation. It refuses safely before events/input on mismatch;
production SaveLoad and Atlas do not arm it.

Linked focused container, codec and restore checks passed **11/11**, and
an MM7 headless control passed **1/1**. There is no v4 full-session
preflight, disposable slot or fresh-child v4 reload; no production save,
load or Atlas Play is enabled. Writer-side complete capture and final
event/RNG agreement remain open.

See also: [V3 Post Entry Map and Pre Event](MM6-F4-V3-Post-Entry-Map-and-Pre-Event.md),
[V3 Full Session Preflight](MM6-F4-EC6-V3-Full-Session-Preflight.md),
[Map Bytes](MM6-F4-Map-Bytes.md).
