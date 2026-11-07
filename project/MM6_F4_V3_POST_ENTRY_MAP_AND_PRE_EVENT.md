# MM6 EC6 v3 post-entry map and pre-event replay seam

Status, 2026-11-07: bounded map capture and test-only replay seam. The
production v3 writer, SaveLoad load route, menu and Atlas Play remain closed.

## Post-entry outdoor delta

`captureOutdoorDeltaWithActiveActors()` starts from the retained native MM6
DDM, checks loaded-map identity and record counts, and preserves original
actor records, chest records, persistent bytes and opaque tail. It updates
the respawn header, visibility and supported ground-sprite header fields.
Full active actor state belongs to `ACTR`; the native actor prefix must retain
its original slot and identity anchors. Changed ground item payloads or
opaque tails, chest data, actor identity, unsupported sprite scalars and
particle transients refuse instead of projecting a lossy save.

One first-map sprite had a fractional height that the DDM integer position
cannot store directly. The engine's outdoor loader calls
`arrangeSpriteObjects()` on each load; for pickable, nondropped objects it
sets height from `ODM_GetFloorLevel`. The capture helper now reconstructs
the candidate from the exact DDM integer position, applies that floor step
and accepts the live position only if the full replayed vector matches.
Dropped, unpickable or nonreplaying positions still refuse. This is a
narrow replay-equivalence rule, not blanket rounding.

The private-backed, null-renderer
`MM6V3PostEntryMapOwnersHeadless.DISABLED_FirstMapRetainedDDMMatchesLiveOwners`
passed **1/1** after this change. The diagnostic compares the captured DDM
against live reset, visibility, sprite, chest and opaque owners. It does not
write a v3 slot, leave and reload the map, or prove later map/item mutations.

A separate no-write full-session candidate probe now attempts the complete
detached v3 semantic stage after first-map entry. A stage marker confirms
that `stageV3FullSessionPreflight()` returned successfully. The later event
classifier confirmed exact active native 200-byte event data and zero generic
`mapVars`, then refused because inherited generic `decorVars` were nonzero
after map load: `event-decor-bound-high` (**0/1**). The nonzero value is bound
to an interactive decoration's assigned high `eventVarId`, rather than an
unbound transient. This supersedes the earlier `event-legacy-decor`,
`map-preflight` and broad `persistent-event-join` classes. Source order
explains the divergence: first entry with a zero respawn day calls
`RespawnGlobalDecorations()` before `PrepareDecorations()`, filling the
generic `decorVars[0..123]`; some decoration event draws consume RNG. A
stamped no-respawn DDM reload skips that call. Zeroing these values would
lose live state, while an exact mapping to MM6's native 200 event bytes has
not been proved. The detached preflight does not admit the live session.
The candidate also guards against unrepresented commerce and cursor state.
No v3 atomic write was attempted.

## Before map events

`ScopedV3PreEventReplayForTest` is a test-only context around the existing
checkpoint after map/actor rebuild and before generated NPC rows and map
events. It requires a completely staged v3 ACTR/map/GNPC/RNG agreement,
fresh native actor anchors, the selected RNG type, scoped MM6 chest opt-in,
ready CHST owner and exact active DDM bytes. After all validation, it swaps
in ACTR actors and generated NPC rows. `_461103_load_level_sub()` retains its
chest, arena, decoration and cursor work while skipping only the generated
NPC reset/loop. The helper checks the chest owner and active DDM again,
restores RNGS immediately before `onMapLoad`, and requires that ordering
before events run. The context is one-shot and clears on scope exit.

Linked `MM6V3PreEventReplayForTest.*` plus
`MM6PreEventCheckpointForTest.*` passed **7/7**. A disposable v2 writer and
separate fresh-child Main Menu QuickLoad abort each passed **1/1** after the
shared Engine changes, and an MM7 headless `Prs.Pr2354` control passed
**1/1**. These controls do not run a v3 child replay: no production caller
arms this scope, and no v3 save is written.

A disabled, strict-input-only fresh-child v3 GameTest now exists. It requires
a separately admitted disposable v3 slot and has not run because no such
slot exists. Its presence is a test harness, not replay evidence.

## Next admission work

Before a disposable v3 write, resolve the live `event-decor-bound-high` refusal, join the
bounded map capture with all live v3 session owners, prove writer-side
BEAC/IFOV policy and pass the required semantic callback. Then use a fresh
child to stage v3, publish all pre-event
owners, compare final post-world resources/events/actors/party/chests and
abort before input on any mismatch. Only a save, quit and reload with
first-input admission can support a durable gameplay claim. ACTR still
refuses actor items and nondefault buffs.

See [full-session preflight](MM6_F4_EC6_V3_FULL_SESSION_PREFLIGHT.md),
[outdoor actor-map agreement](MM6_F4_EC6_V3_OUTDOOR_ACTOR_MAP.md),
[pre-event checkpoint](MM6_F4_PRE_EVENT_CHECKPOINT.md) and
[v3 atomic boundary](MM6_F4_EC6_V3_ATOMIC_BOUNDARY.md).
