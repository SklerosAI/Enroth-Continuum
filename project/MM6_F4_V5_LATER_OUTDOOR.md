# MM6 v5 later-outdoor admission

Status, 2026-11-07: the source and direct world-load gates passed for the
shipped outdoor maps. A test-only later-map `.ec6` candidate passed an atomic
write and a separate fresh-child replay. That reader aborts before input;
production MM6 SaveLoad and Atlas Play remain closed.

The v5 outdoor preflight, runtime actor/sprite/decoration replay scopes and
map identity check now admit the exact 15 shipped outdoor grid names. They
retain title, slot, source-map and native-anchor checks; v1–v4 rules are not
relaxed. The read-only headless source audit decoded all 15 outdoor base/delta
pairs, and focused v5 later-map replay checks passed 3/3. A direct headless
`oute2.odm` world preparation from an unsaved creation Party reached the
requested outdoor map with 20 chests (1/1). This establishes a map-load route,
not a gameplay transition or save/reload cycle.

The first direct load exposed an MM6-only map-table mistake: the encounter
loader used display labels in place of authored monster stems. The corrected
column selection passed a synthetic table check and the existing seven-process
first-map v5 cycle. The full-map encounter audit then found one authored stem
without corresponding ranked monster entries. Source-backed reachability
checks showed that encounter family has neither a positive random spawn
chance nor a reference from the map's actor spawn indices. The all-67 map
headless gate therefore validates every *reachable* encounter family against
the loaded monster/descriptors (1/1); production lookup remains strict and
does not invent a special name alias.

A separate owner-only, test-only `candidate-v5-oute2.ec6` slot passed atomic
isolation checks with the rest of the atomic suite (18/18). The first strict
writer then passed after separate disposable v2 seed and v4 baseline children,
each **1/1**. It captured oute2-specific native DDM/actor/EVRT anchors and
full represented owners, rechecked the candidate before an atomic write, and
kept the earlier slots unchanged. An authored native decoration ID 0 is now
accepted only when its name matches row 0 of the loaded MM6 decoration table;
unknown or mismatched identities still refuse. Spawned actor names are cleared
only in a detached capture copy when both match the loaded static monster row;
mutated names still refuse and live actors are untouched. Focused source,
DECR and actor-name checks passed **7/7**. The first-map v5 capture API remains
first-map only.

A separate fresh child selected the oute2 candidate, replayed represented
owners before map events, checked them again after world preparation, and
aborted before input. In a disposable profile the v2 seed, v4 baseline, oute2
writer and fresh-child reader each passed **1/1**. The reader's first strict
PID check refused one native-to-runtime NPC ID: map setup assigns a generated
NPC link to an actor whose native DDM link is zero. The v5-only reader now
requires that exact native zero, a matching GNPC row and contiguous runtime
link, exact ARTR owner, and unchanged ordered native actor/sprite anchors;
the four-process chain passed after this source-backed check. Earlier save
slots remained byte-identical. The next gate is bounded first input and a
second save/reload. The writer's direct map setup is not a campaign travel
transition, and the current checks do not establish later-map playability.

See [v5 runtime ownership](MM6_F4_EC6_V5_SPRITE_RUNTIME.md),
[local status](LOCAL_STATUS.md), and [the F4 path](plan.md).
