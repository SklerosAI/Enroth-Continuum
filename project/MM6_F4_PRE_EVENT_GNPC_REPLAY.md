# MM6 pre-event generated NPC and RNG replay gate

Status, 2026-11-07: this detached read-only agreement helper remains in use.
A later [test-only v3 replay scope](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md)
now publishes rows and RNG at guarded points; no normal MM6 load uses it.

`DoPrepareWorld` rebuilds the map and actors, then calls
`setNPCNamesOnLoad()`, `_461103_load_level_sub()`, and `onMapLoad()` in that
order. The level subroutine resets `uNewlNPCBufPos` and generates rows for
eligible peasant actors, consuming `grng`; `onMapLoad()` may execute random
event operations. A post-world restore is therefore too late. The test-only v3
replay reconciles actors first, then bypasses only the NPC
generation loop while retaining chest, arena, decoration and cursor work in
the rest of `_461103_load_level_sub()`. It must publish saved GNPC rows and
RNGS before `onMapLoad()` and prove the first event observes them.

`stagePreEventGeneratedNpcReplay()` validates MM6 title, active map name,
selected RNG engine type, fresh generated-row cursor, both detached section
codecs, actor slot IDs and monster IDs, peasant eligibility, contiguous saved
row anchors, and existing static NPC links without mutating live owners. The
linked `MM6PreEventGeneratedNpcReplay.*` suite passed **2/2**, including
refusal of respawn-added actor mismatch and wrong load context.

This helper is not itself a live replay. The later test-only scope has
synthetic order coverage but no v3 fresh-child session. Strict post-world
owner validation remains required. The pre-entry bootstrap v2 slot still
refuses correctly.
