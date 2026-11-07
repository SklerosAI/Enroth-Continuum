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
decoration index, descriptor, cog, assigned event variable ID, exact position
bits, yaw and trigger range. Restore
requires exact active map, native reset header and rebuilt decoration
bindings. Native DDM/DLV 200-byte event data remains independently retained
in `MAPS`.

The original index/descriptor/cog/slot binding could not distinguish two
same-type decorations swapped in position. EVRT inner payload **v2** now
binds position bits/yaw/trigger range and rejects unpublished payload v1;
a focused same-type swap test refuses. This closes that specific identity
gap, while complete decoration/event replay remains unproved.

The exact EC6 v4 manifest has 23 required sections: v3's 22 plus `EVRT`.
V1–v3 exact decoders remain strict. The test-only MM6 scope validates a
no-respawn reload before copying the two arrays ahead of outdoor or indoor
decoration preparation. It refuses safely before events/input on mismatch;
production SaveLoad and Atlas do not arm it.

Linked focused container, codec and restore checks passed **12/12**, and
an MM7 headless control passed **1/1**. A detached outdoor-only v4
full-session stage now joins v3 owners with active EVRT map/header/bindings
using caller-supplied rebuilt decorations. Its linked positive and refusal
suite passed **5/5** after a heap-backed preflight fix. A strict opt-in
three-process first-map sequence passed **1/1** for each separate process:
v2 seed, atomic v4 candidate writer, and fresh-child Main Menu QuickLoad
replay. The reader checked exact EVRT arrays, native event bytes, decoration
flags, ACTR/GNPC/RNGS, CHST item provenance and represented Party/cursor/
recovery owners before aborting to Main Menu without input. The v2 seed and
v4 candidate were unchanged. Marker-free chest replay uses a test-only
validated projection to attach MM6 raw item records while preserving saved
CHST LCG and DDM bytes. Focused full-session/atomic checks passed **8/8**,
chest route checks passed **8** with one private-reference skip, and an MM7
headless control passed **1/1**. A separate strict fresh-child test continued
the verified reload into the real null-renderer game loop, reached first
input, turned and walked once, and quit with both slots unchanged (**1/1**).
A post-action second write, indoor/later-map replay and production
SaveLoad/Atlas remain closed.

The attempted second v4 write after the bounded turn/walk refused before
disk mutation because a gameplay-updated ground sprite had fractional X/Y
coordinates. Native DDM stores integer sprite position; the current strict
capture will not truncate it. An exact full-list runtime sprite owner is
under design for a later schema, with native slot and MM6 raw item
provenance checks. No post-action candidate or reload has passed. See the
[V5 Sprite Runtime](MM6-F4-EC6-V5-Sprite-Runtime.md) and the
[source-backed packet](../project/MM6_F4_EC6_V4_EVENT_RUNTIME.md).

See also: [V3 Post Entry Map and Pre Event](MM6-F4-V3-Post-Entry-Map-and-Pre-Event.md),
[V3 Full Session Preflight](MM6-F4-EC6-V3-Full-Session-Preflight.md),
[Map Bytes](MM6-F4-Map-Bytes.md).
