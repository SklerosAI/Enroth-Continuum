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
It has no encoder or production publisher and always refuses checkpoint write.

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
durable old-map checkpoint. A future schema must carry per-map runtime and
native anchors together, then prove return and fresh-child replay. The
post-rename sync-failure result also needs a defined commit/recovery policy
before live travel can rely on a write.

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
