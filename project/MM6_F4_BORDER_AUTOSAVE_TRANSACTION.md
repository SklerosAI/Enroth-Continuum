# MM6 outdoor border autosave transaction

Status: **read-only design audit; no production travel save** (2026-11-07). This packet follows the detached [border route trace](MM6_F4_OUTDOOR_BORDER_TRACE.md), the seven-process [later-outdoor save cycle](MM6_F4_V5_LATER_OUTDOOR.md), and current source. It contains no original game assets or private save data.

## Current source boundary

- [`startMapTransition`](../src/Engine/Engine.cpp) stops audio, calls `autoSave()` before scheduling a different map, then sets the pending destination. The MM6 test-only intercept used by the no-write location-change check skips that call. The check proves target world switching and bounded input, **not** old-map persistence.
- [`UIMSG_OnTravelByFoot`](../src/Application/Game.cpp) uses its own MM7 path: it closes dialogue, stops audio, pauses the timer, calls `autoSave()` directly, then changes location and applies time/food/rest effects. It bypasses `startMapTransition`. Both entry points need one MM6 transaction owner before live border travel is enabled.
- Creating [`GUIWindow_Transition`](../src/GUI/UI/UITransition.cpp) pauses the timer. The existing [`captureV5FirstMapLiveSession`](../src/Engine/MM6/MM6V5LiveSaveCapture.cpp) requires a settled, unpaused first-map state. It cannot simply be called from a paused travel confirmation or a later outdoor map.
- MM6 [`saveGame`](../src/Engine/SaveLoad.cpp) throws before writing. The inherited `autoSave()` returns void; its MM7 write path can swallow an autosave disk failure. MM6 travel needs a required success result before changing Party, map cache, food, time, UI or pending transition. Do not route it through the MM7 `createSaveData(false)` path, which mutates map/cache state while preparing a save.
- [`writeAtomicV5SaveFileForTest`](../src/Engine/MM6/MM6AtomicSaveFile.cpp) is restricted to named candidates in owner-only disposable profiles. It validates a v5 shell and semantic callback, rejects foreign-version primary/backup files, validates prior same-version files and uses write, sync, readback and rename. Its fault tests do not make it a production autosave API. A failure after primary rename may leave a valid new primary despite a reported sync error; a backup may change before primary replacement. Define a clear durable-result and recovery policy before using that sequence in gameplay.
- The current v5 container has singular active-map actor, sprite and decoration runtime sections. A later-map save could lose runtime owners from a previously visited map unless a visited-map representation or lossless native-delta projection parks them. The no-write transition check confirms the old `pMapDeltas` entry remains unchanged across the transition, so an in-session return can be stale. A detached in-memory owner now survives that test transition, but has no v5 disk section or live return publisher.

## Original confirmation effects

The bounded accepted-executable trace shows confirmation refusing nonzero flying state, rechecking the neighbor, closing transition UI/audio, then advancing time by the hireling-adjusted days. With any food, the party takes a full rest/heal; if food is insufficient, it marks all four characters Weak and increments the no-rest counter once, then deducts food down to zero. With zero food, it skips full rest/heal, marks Weak, increments the counter once and deducts no food. The original writes the current game time to each Weak clock even if already Weak, and plays the Weak reaction only when that clock was zero. The original branch then selects the target, wraps X and Y independently by one map width, recalculates ground height and transitions. Rejection and the distinct cancel message both play click feedback, clamp both axes and close the travel UI without time, food, condition or map changes.

This differs materially from inherited [`UIMSG_OnTravelByFoot`](../src/Application/Game.cpp), which calls `restAndHeal(Duration)` regardless of food. That helper itself invokes `Party::restAndHeal`, so zero-food travel heals in the inherited path. Its [`Character::SetCondition`](../src/Engine/Objects/Character.cpp) returns early for an existing Weak condition and does not refresh the timestamp. The inherited confirmation also lacks the MM6 coordinate wrap. Its separate cancel handler already matches the core clamp/close/no-save behavior. Do not reuse the inherited confirm time/food/rest/Weak effects for MM6.

## Required transaction shape

The detached [`stageBorderOldMapParty`](../src/Engine/MM6/MM6OutdoorBorderPrecommit.h)
now establishes one narrow precommit owner: for an MM6 cardinal geometric
candidate with the original offer flags clear and no flying, it copies the
Party and clamps only that copy's X/Y coordinates to the original border
bounds. The live Party, destination, disk and map cache are untouched. Wrong
title, invalid route, disallowed state and unverified corner crossing return
typed refusals without a copy. Even a staged copy returns
`MissingWorldAndSourceProof` and `canWriteCheckpoint() == false`: it has no
source archive/terrain agreement, active and visited map owners, or durable
write result. This is not a complete old-map checkpoint or a live confirmation
handler. Focused new code compiled with the integration unit-test flags. The
rebuilt integration unit target passed 11/11 border checks: three new
precommit tests and eight existing route/eligibility controls.

## Second outdoor no-write checkpoint probe

The production [`captureV5FirstMapLiveSession`](../src/Engine/MM6/MM6V5LiveSaveCapture.cpp)
remains limited to the first outdoor map, an unpaused settled timer, one
first-map event owner, and its hardcoded native DDM/decoration anchors. It
cannot run during the paused travel dialogue or capture a later outdoor map.
The established test-only [later outdoor writer](../test/Bin/GameTest/GameTests_MM6V5LaterOutdoorCandidate.cpp)
has a separate source-anchored v5 capture for the second outdoor map. It now
also accepts a detached Party override for a **no-write** border candidate.

In a fresh owner-only temporary profile, the v2 seed, v4 baseline and
modified later-map writer each passed **1/1** headlessly. After the writer's
existing disposable v5 slot was accepted, a copied Party was placed just
outside one cardinal edge for this fixture. The precommit stage clamped only
the copied old-map Party; the test confirmed both source and destination base
resources, encoded a separate candidate in memory, and ran the full v5
semantic preflight and exact live map/event-owner comparison with the Party
override. The candidate's map identity and saved in-bounds Party position
matched the loaded source, while the live Party core and all existing
disposable slots remained unchanged. The candidate was never written. The
temporary profile was removed after verification.

The border position was constructed on a copy, not reached by actual
movement. This probe does not prove a real confirmation handler, source
terrain safety at the clamped coordinate, visited-map runtime preservation,
old-map reload from this candidate, or a durable-write outcome. The
`canWriteCheckpoint()` gate remains false, and MM6 travel/production SaveLoad
remain closed.

## Detached visited outdoor owner across one transition

[`VisitedOutdoorRuntimeOwner`](../src/Engine/MM6/MM6VisitedOutdoorRuntimeOwner.h)
copies one **already preflighted** MM6 outdoor source map into an in-memory
capsule: exact native DDM and chest delta, ACTR anchor, ARTR actors, GNPC rows,
SPRT slots, DECR decorations and EVRT entry. Its validator requires one exact
map identity across those owners, equal MAPS/CHST native bytes, actor and
sprite anchor agreement, and decoration agreement with the loaded source base.
It has no production publisher and always refuses checkpoint write.

The [no-write location-change fixture](../test/Bin/GameTest/GameTests_MM6LocationChangeLifecycle.cpp)
now captures a full first-map v5 candidate at settled input, preflights it
against the loaded MM6 source and stages this capsule before crossing to the
second outdoor map. After the intercepted transition, it revalidates the
detached old-map owner and confirms the live old-map cache stayed unchanged.
Injected native-delta, event-map and sprite-map faults each refuse. In a fresh
owner-only disposable profile, the v2 seed, v4 baseline and modified
location-change processes each passed **1/1** headlessly; the profile was
removed. The source candidate was held in memory and no slot was written by
the transition fixture.

This proves that the source owner can be retained while another outdoor map
loads. It does **not** encode visited runtime into EC6 v5, restore it on an
in-session return, prove a second-map border confirmation, or establish a
durable old-map checkpoint. The detached outdoor v6 schema below now carries
per-map runtime and native anchors, but return and fresh-child replay remain
unproved. The
post-rename sync-failure result also needs a defined commit/recovery policy
before live travel can rely on a write.

## Detached outdoor v6 visited-map schema

[`V6OutdoorSaveFields`](../src/Engine/MM6/MM6V6OutdoorSaveFields.h) is a
test-only EC6 v6 outdoor candidate. Its first 26 sections are the exact
active outdoor v5 payload. A required, versioned `VIST` section adds ordered
prior-map capsules; each entry owns one native DDM, chest record, ACTR anchor,
GNPC rows, EVRT entry, ARTR actors, SPRT sprites and DECR decorations. The
codec bounds count and lengths, rejects duplicates or unsorted maps, refuses
an active/visited map collision and canonicalizes every inner owner. It does
not change v5 or indoor v6 manifests. Source-base decoration agreement is a
separate admission check after decode; a successful wire parse alone cannot
publish a returned map.

The no-write location-change fixture captures the first-map capsule at
settled input, then uses an MM6-only test scope in `PrepareWorld` to capture
the second-map v5 owner **after map preparation and before first input**.
Capturing the latter after two ordinary ticks was correctly refused because
an actor position had become fractional and could not fit the native ACTR
anchor; the width guard was retained. The fixture round-trips a `VIST` section
with both source-backed map capsules, then wraps the first map as visited
beside the second map's active v5 data. Decode/re-encode is exact, and the
peeled active v5 bytes are identical to the input. Bad VIST version,
truncation, changed outer map identity and active/visited collision refuse.
The linked headless run passed **1/1** after a fresh v2 seed and v4 baseline
had each passed **1/1** in the same owner-only disposable profile. The
temporary profile was removed. No v6 file was written.

This establishes a bounded wire shape for two maps, not a complete checkpoint
transaction. The old live `pMapDeltas` entry still stays stale on transition;
there is no v6 visited-map publication or in-session return. The border
precommit flag remains false, and production MM6 travel and SaveLoad remain
guarded.

## Disposable outdoor v6 write and fresh-child read

The [atomic save helper](../src/Engine/MM6/MM6AtomicSaveFile.cpp) now admits
only `candidate-v6-outdoor.ec6` in an owner-only `mm6-restart-*` system-temp
profile. It checks the exact 27-section v6 outdoor shell, requires a semantic
callback for both active and visited maps, refuses foreign-version primary or
backup files, validates existing same-version slots and uses the existing
bounded write, sync, readback and rename sequence. Its matching preflight
selects a primary or backup read-only and also requires full semantic
validation. Neither path publishes a map, repairs a backup or enters normal
MM6 SaveLoad.

The [opt-in writer](../test/Bin/GameTest/GameTests_MM6LocationChangeLifecycle.cpp)
builds the v6 candidate from the pre-input second-map v5 and the detached
first-map owner. Before the successful write, malformed VIST and cross-map
candidates refuse, and an injected fault just before primary replacement
leaves every source slot unchanged. After writing, the exact v6 primary passes
read-only preflight. The [fresh-child reader](../test/Bin/GameTest/GameTests_MM6V5LaterOutdoorFreshChildReplay.cpp)
decodes v6 and stages the active v5 through the existing full source-backed
outdoor replay. It validates the visited owner against its source base before
any world publication, then checks it again after the active map rebuild;
the child aborts before input. The visited owner remains detached and is not
published into `pMapDeltas`.

In a fresh owner-only disposable profile, five separate headless processes
passed **1/1 each**: v2 seed, v4 baseline, an existing second-map v5 control
writer, outdoor v6 writer, and outdoor v6 fresh-child reader. The v6 writer
and reader required the v5 control slot and compared its exact bytes after
their work; the seed and v4 files also stayed unchanged. The profile was
removed. This proves a test-only v6 disk round trip for one active and one
visited outdoor map. It does **not** prove a returned old-map world, a
visited-map publication transaction, real border confirmation, or durable
success after a post-rename sync failure. Production checkpoint writing and
travel remain closed.

## Read-only visited-map return plan

The [return plan](../src/Engine/MM6/MM6VisitedOutdoorReturnPlan.h) now stages
the source-backed visited map as a detached owner. It takes the old map's
native DDM and actor, NPC, event, sprite and decoration runtime from `VIST`,
but builds the target CHST with the **current active-map** global chest LCG
and artifact ledger plus the visited map's native DDM. Reusing the old map's
CHST ledger could roll back global artifact generation after time spent in
another map. The plan requires an exact active/visited map pair and source
base; it refuses an absent Party arrival with a typed result. Even a bounded
proposed arrival cannot authorize publication until source terrain and a
failure-atomic runtime transaction are proved. `canPublish()` and
`canWriteCheckpoint()` remain false.

The outdoor v6 fresh child stages this plan during read-only preflight and
again after rebuilding the active second map. It checks the current global
ledger was retained and verifies Party core, global chest state and the stale
first-map cache remained untouched. A fresh five-process headless chain (v2,
v4, existing v5 control, v6 writer, v6 reader) passed **1/1 each** after this
addition; the temporary profile was removed. This is not a returned map.

The source order explains the remaining publication blocker: a normal
location change calls `PrepareWorld(0)` and `DoPrepareWorld(false)`, while the
test-only EVRT, V3 actor/GNPC/RNG and V5 actor/sprite/decoration replay hooks
run only on `DoPrepareWorld(true)` fresh loads. A return needs a separately
validated pre-event replay path for `false`, live arrival/terrain agreement,
and rollback for a partially prepared target. No such hook was added here.

## Source-backed detached return arrival

[`stageVisitedOutdoorReturnArrival`](../src/Engine/MM6/MM6VisitedOutdoorReturnPlan.h)
now obtains the target base through the admitted MM6 `LoadedMapSource`, joins
the exact active/visited map pair and visited owner, and decodes its ODM before
staging a Party copy. The [arrival helper](../src/Engine/MM6/MM6OutdoorReturnArrival.h)
requires a cardinal border route to that target and eligible MM6 offer/flying
state. It applies the original one-width coordinate wrap and samples the
target height map with the engine's terrain triangle interpolation. It refuses
nonfinite/out-of-bounds coordinates, corners, a wrong target, and any arrival
whose Party radius overlaps a target model bounding box, where a model face
could supersede terrain height. The copied Party receives the target position,
zero velocity and fall-start height, matching the represented placement
fields. No live Party, map cache, chest owner or save slot is changed.

Three focused unit tests passed **3/3** for flat and sloped terrain, target
identity, model-floor refusal and detached-only mutation. In the existing
fresh-child outdoor v6 reader, a synthetic eligible Party copy crossed one
cardinal edge into the visited source map; an eligible in-bounds copy refused
without a staged arrival. The reader checked live Party/chest/cache owners
remained unchanged. The fresh five-child seed, v4, v5 control, v6 writer and
v6 reader chain passed **1/1 each**. The synthetic boundary pose was not
reached by movement or a confirmation UI, and the visited map was not
entered. `canPublish()` and `canWriteCheckpoint()` remain false. A return on
`DoPrepareWorld(false)` still needs an owner publication order and rollback.

## Read-only in-session return floor probe

A separate opt-in [pre-event probe](../src/Engine/MM6/MM6VisitedReturnPreEventProbeForTest.h)
observes an MM6 outdoor `DoPrepareWorld(false)` after native target world
preparation, before `placeParty`, chest generation, `_461103` and `onMapLoad`.
The no-write fixture keeps the source-backed `VIST` owner and staged Party
arrival detached, intercepts the second-map-to-first-map transition autosave,
and checks the loaded target terrain height and full `ODM_GetFloorLevel` agree
with the staged Z. It also checks map identity, paused timer, pending
destination and unchanged live Party position. The probe throws to Main Menu;
the transition catch clears the pending destination and releases loading
progress. No target events or gameplay input are admitted. This abandons a
partial world; it does not roll back to the second map.

The initial probe scope was released while the loading screen was still
pending, so the later unarmed transition reached the existing MM6 chest guard
and correctly refused. Keeping the one-shot scope armed through that screen
fixed the fixture without changing chest behavior. The scope unit passed
**1/1**. In a fresh owner-only temporary profile, the v2 seed, v4 baseline
and return-probe children each passed **1/1**; the profile was removed. No
existing save slot changed. This proves one loaded target floor agrees with
the detached arrival before placement. It does not publish the visited native
DDM, actors, sprites, decorations, event state or global chest ledger; test
those owners after `_461103` and define failure recovery before any live
return. A safe `DoPrepareWorld(false)` replay still needs a staged all-owner
commit/abort policy and explicit post-rename durable result.

1. Choose a canonical production EC6 version, slot name and title-private profile root. Explicitly reject or migrate older autosave schemas; an existing v2 test seed cannot silently become a v5 production slot.
2. Stage an exact detached old-map save for any admitted outdoor map at a safe point before travel UI effects. Include Party and character state, inventory/buffs/hirelings, UI/cursor/shops, NPC and generated links, RNG, active and visited map deltas, chests/artifacts, actor/sprite/decoration runtime, event variables and owner identities. The old-map checkpoint must contain a reload-safe in-bounds **Party copy** when the live border position lies outside the map edge; keep live Party untouched until commit.
3. Revalidate destination geometry, source-map archive identity, the three-bit offer mask and confirmation flying state. Stage target map/resource readiness and a no-reentry capture. Give the disk writer a semantic validator and a required success result.
4. Only after the old-map save is durably accepted, publish staged old-map cache/visited owners, close the dialogue, stop audio, apply exactly-once MM6 travel time and conditional rest/food/condition effects, wrap each crossed coordinate, recompute target ground height and queue the destination. Do not invoke a second inherited autosave.
5. On precommit capture or disk refusal, preserve Party, time, food, map/cache, pending transition and UI state. Define the post-rename sync-failure outcome explicitly. On target-load failure, keep input closed in the partial world and return to a recoverable checkpoint or menu until full world rollback exists.

## Headless acceptance

- Inject capture, semantic, schema and atomic-write faults. Check no precommit Party/time/food/cache/pending/UI mutation, valid primary/backup outcomes for each fault point, and no temporary or unrelated slot damage.
- In separate children: reload the source, execute real border confirmation, write the old-map checkpoint, enter the target and move, then reload the old-map checkpoint and compare actor, sprite, chest, event and NPC owners at a safe in-bounds position. Add an in-session return to the source map.
- Invalid, flying, cancelled and corner cases must not write. Keep MM7 headless control in the same integration gate. Graphical travel acceptance follows only after the durable headless cycle.

No runtime route or production save hook was changed by this probe. See [atomic save design](MM6_ATOMIC_SAVE_F4.md), [save remainder](MM6_SAVE_REMAINDER_F4.md), and [local status](LOCAL_STATUS.md).
