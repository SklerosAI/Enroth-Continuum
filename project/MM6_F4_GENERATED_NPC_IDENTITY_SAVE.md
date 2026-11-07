# MM6 generated NPC identity save candidate

Status, 2026-11-07: detached, versioned `GNPC` section with focused synthetic
checks; **not** in the strict nineteen-section EC6 v2 manifest and not a live
save/load route. The current source and linked integration results outrank this
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
`RandomEngine` provides `seed`, draws and `peek`, but no state capture/restore;
`EngineRandomComponent` owns both concrete gameplay engines and switches the
global pointer. Recording only a seed or peeking one result cannot reconstruct
the current generator state after arbitrary draws. GNPC is an identity capsule,
not an RNG snapshot.

The safe next policy is explicit: either add a versioned, bounded snapshot for
the selected gameplay RNG implementation and prove that restoration and
generated-row publication precede any replay-sensitive map event, or document
and test an intentional fresh-entropy fork policy for every affected event and
first-frame outcome. Until one policy is verified, a nonempty generated pool
remains a v2 reload refusal. Do not borrow the separate chest LCG as `grng`.

See [the post-map link gate](MM6_F4_POST_MAP_GENERATED_NPC_LINKS.md),
[the raw v2 owner gap](MM6_F4_V2_FULL_SESSION_OWNER_GAP.md), and
[the post-world owner validator](MM6_F4_POST_WORLD_OWNER_VALIDATOR.md).
