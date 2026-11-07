# MM6 generated NPC identity save candidate

Status, 2026-11-07: detached, versioned `GNPC` section with **3/3 linked
synthetic integration checks**; **not** in the strict nineteen-section EC6 v2
manifest and not a live save/load route. The current source and linked results outrank this
packet. No private NPC names, saves or map contents are recorded here.

## Why v2 cannot preserve identity

`Engine::_461103_load_level_sub()` rebuilds generated NPC rows for eligible
peasant actors before `onMapLoad()`. `NPCStats::InitializeAdditionalNPCs()`
draws a name, portrait, reputation and profession from `grng`. EC6 v2 `RESC`
stores the mutable 501 source NPC rows and groups, but not the 100-row
`pAdditionalNPC` pool or its actor-to-row mapping. A fresh child therefore can
construct different generated people. The title-specific pre-write pool bound
prevents overflow; the final link-shape check cannot reconstruct identity.

## Detached GNPC contract

`MM6GeneratedNpcSaveFields` captures a canonical `.blv`/`.odm` map name and up
to 100 active generated rows. Each row includes the actor slot, monster ID and
the complete existing bounded logical `NPCData` record. Capture requires
contiguous actor links `5000 + row index` and an equal generated cursor. The
v1 section codec rejects a wrong title, missing/optional/duplicate section,
bad map name, unordered slots, invalid monster/row semantics, oversized or
truncated records and trailing bytes before returning fields. A read-only
post-map check compares saved map and actor/monster links; it does not publish
rows. The container recognizes `GNPC` as a known required tag, while
`decodeRawSave()` still rejects it as an extra v2 section. This keeps every
existing v2 slot's meaning unchanged.

The linked suite covers a round trip with map/actor binding, late row/actor
divergence, malformed record refusal and the strict v2 extra-section boundary.

The standalone GNPC capsule has no live-restore semantics yet. To admit a
future version, join it to `HEAD` and the active map actor snapshot, stage all
rows on a detached NPC owner, and restore them at a source-backed point before
any event or dialogue reads them. A post-`onMapLoad()` rewrite alone can be too
late because reload events have already run. Save files must not advertise the
new version until that order is proved in a fresh child.

## Game RNG policy gate

`Engine::MM7_Initialize()` seeds both RNG endpoints from the platform clock.
The default gameplay engine is Mersenne Twister; test/debug sessions can select
the sequential engine, and optional tracing wraps the gameplay endpoint.
The base `RandomEngine` interface provides `seed`, draws and `peek`; the
selected concrete engine is owned by `EngineRandomComponent`. Its new
capture/restore seam records the full current engine state, whereas an initial
seed or one peek cannot reconstruct it after arbitrary draws. GNPC is an
identity capsule; RNGS owns the separate gameplay stream.

The detached [RNGS candidate](MM6_F4_GAMEPLAY_RNG_SAVE.md) now supplies exact
capture/restore for the selected gameplay engine and a bounded save section.
It does not establish the **load order**: the inherited map loader regenerates
NPCs and runs `onMapLoad()` before the post-world hook. Restoring saved RNG
state only at that hook would preserve later draws but cannot undo any event
that already consumed the fresh stream or saw different NPC rows. A future
load must either publish the saved rows and restore the relevant RNG state
before replay-sensitive events, or document and test a deliberate alternate
entropy policy for those events. Until one is verified, a nonempty generated
pool remains a v2 reload refusal. Do not borrow chest LCG as `grng`.

See [the post-map link gate](MM6_F4_POST_MAP_GENERATED_NPC_LINKS.md),
[the raw v2 owner gap](MM6_F4_V2_FULL_SESSION_OWNER_GAP.md), and
[the post-world owner validator](MM6_F4_POST_WORLD_OWNER_VALIDATOR.md).
