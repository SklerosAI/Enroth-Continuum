# MM6 pre-event generated NPC and RNG replay gate

Status, 2026-11-07: detached read-only agreement helper. It does not publish
generated NPC rows or RNG state, install an Engine hook, alter the strict EC6
v2 manifest, or enable normal MM6 load.

`DoPrepareWorld` rebuilds the map and actors, then calls
`setNPCNamesOnLoad()`, `_461103_load_level_sub()`, and `onMapLoad()` in that
order. The level subroutine resets `uNewlNPCBufPos` and generates rows for
eligible peasant actors, consuming `grng`; `onMapLoad()` may execute random
event operations. A post-world restore is therefore too late. A future v3
replay must reconcile spawned actors first, then bypass only the NPC
generation loop while retaining chest, arena, decoration and cursor work in
the rest of `_461103_load_level_sub()`. It must publish saved GNPC rows and
RNGS before `onMapLoad()` and prove the first event observes them.

`stagePreEventGeneratedNpcReplay()` validates MM6 title, active map name,
selected RNG engine type, fresh generated-row cursor, both detached section
codecs, actor slot IDs and monster IDs, peasant eligibility, contiguous saved
row anchors, and existing static NPC links without mutating live owners. The
linked `MM6PreEventGeneratedNpcReplay.*` suite passed **2/2**, including
refusal of respawn-added actor mismatch and wrong load context.

This helper is not a live replay. A future exact versioned manifest and
pre-event hook must prove complete actor state, GNPC contents and RNG
continuation before any event; strict post-world owner validation remains
required. The pre-entry bootstrap v2 slot still refuses correctly.
