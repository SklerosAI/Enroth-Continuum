# MM6 EC6 v3 outdoor actor-map agreement

Status, 2026-11-07: detached read-only v3 map-stage helper. The linked
`MM6V3OutdoorActorMapAgreement.*`, v3 manifest and ACTR suites passed
**10/10** in integration. No live actor list, map, save slot or profile is
mutated, and no first-input admission follows from this result.

`stageV3OutdoorActorMapAgreement()` first decodes the exact ACTR/GNPC/RNGS
v3 manifest and runs the existing 67-map raw/base/active-CHST validation.
It requires an outdoor active map; HEAD, GNPC and CHST name the same map.
The active native DDM and CHST sidecar must agree on actor count, respawn
count and a nonzero last-respawn day. The saved Party clock must select a
day that will not trigger initial or timed respawn in `OutdoorLocation::Load`.
ACTR must contain at least the native actor count and no more than the
caller-supplied runtime cap. For each native slot, the decoded DDM, retained
CHST sidecar and ACTR agree on embedded monster, descriptor monster and
initial position; appended ACTR slots remain contiguous. The helper returns
owned staged fields and actors without publishing them.

The comparison intentionally uses stable identity anchors. Mutable HP and
other gameplay fields may differ between ACTR and the old DDM; a positive
synthetic test proves such a difference is accepted. Native NPC IDs can be
regenerated, and this helper does not prove their final linkage, actor
items/buffs, resource state, event effects or graphical readiness. A future
fresh-child load must replay ACTR before GNPC/RNG and map events, then run
strict post-world owner checks before input. The current bootstrap v2 slot
still refuses as designed.
