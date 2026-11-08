# MM6 map transition wiring plan

Status: **plan, 2026-11-07**. Written by Lane T after landing the detached
owners (`91d50de34`, `d600846b2`, `d9e392f0b`, `6f6aa1038`). Nothing below is
wired yet, so no MM6 cross-map move works live. The backlog item in
[SESSION_HANDOFF_2026-11-07.md](SESSION_HANDOFF_2026-11-07.md) owns this work.

## Detached pieces available

- `isMapIndoor(GameTitle, MapId)` and `isMapOutdoor(GameTitle, MapId)` in
  `src/Engine/MapEnumFunctions.h`. MM6 IDs 1-15 are outdoor, 16-67 indoor,
  anything else is neither. MM7 falls through to the existing functions.
- `MM6VisitedIndoorRuntimeOwner.h`: native DLV, chest record, event entry,
  optional face overrides, three native slot anchors, generated NPCs and
  actor, sprite and decoration runtime, with structural and loaded-BLV checks.
  Built from a staged v6 indoor save, taking the DLV from CHST because the
  MAPS cache entry for the active map can lag.
- `MM6VisitedMapCache.{h,cpp}`: one outdoor or indoor owner per map, at most
  66, sorted by name. Re-parking replaces. An invalid owner leaves the cache
  unchanged.
- `MM6VisitedMapSection.{h,cpp}`: a `VIST` section at payload version 2.
  `encodeSessionWithVisitedMaps` and `decodeSessionWithVisitedMaps` add it to
  an outdoor v5 or indoor v6 save and remove it, with active bytes byte-exact.
  A visited map equal to the active map is refused.
- `MM6OutdoorBorderConfirmation.h`: `stageBorderConfirmation(title,
  sourceMap, offered, const Party &, hirelings, fullRest)` on a Party copy.
  Refuses flying, a changed route and corners. Advances hireling-adjusted
  days, rests only with food, sets every Weak clock and bumps the no-rest
  counter once if food runs short, floors food at zero, wraps the arrival and
  returns the characters that need the Weak voice line. Rules come from the
  tracked trace packets.

## Shared call sites

Each site gets one title check that hands off to an MM6 entry point in a new
MM6 runtime file (for example `MM6MapTransition.{h,cpp}`) owning one
engine-lifetime `mm6::VisitedMapCache`. MM7 paths stay byte-identical.

1. `Engine.cpp` around 744 and 765: use `isMapIndoor(engine->gameTitle(),
   transitionMapId)`. Same for the asserts at `Outdoor.cpp` around 1852 and
   `Indoor.cpp` around 941.
2. `Outdoor.cpp` `getTravelDestination` (around 291-295): replace the MM6
   early return with `mm6::borderTravelDestination(*pParty,
   currentMapFileName(), hirelings())`, returning the destination map and
   arrival placement, or empty for a corner or blocked party flags. Set the
   default foot travel time to 5. The edge check around 969 needs an MM6 call
   to `borderOfferPartyFlagsPermit`, because the MM6 offer ignores the burning
   flag.
3. `Game.cpp` `UIMSG_OnTravelByFoot` (around 746): for MM6 call
   `mm6::confirmBorderTravel()` and continue. It stages the confirmation with
   a rest callback (refusal behaves like cancel), stages the old-map party,
   calls `mm6::departActiveMap(oldMapParty, &cache)` and aborts unless the
   durable result is committed, then commits: end dialogue, stop sounds, pause
   the timer, move the confirmed party into `*pParty`, play the Weak voice
   lines, set the location-change state and the pending transition. It must
   not call `autoSave()`.
4. `Game.cpp` `UIMSG_OnIndoorEntryExit` (around 715-718): add an MM6 preflight
   before `onMapLeave()`, because today map-leave events and timer clears run
   before `startMapTransition()` refuses. Only then enable the Yes button in
   Codex's MM6 transition dialog (C1).
5. `Engine.cpp` `startMapTransition` (around 1785): for MM6 replace
   `autoSave()` with `mm6::departActiveMap(*pParty, &cache)`. It captures the
   live map into a visited owner, parks it, writes the departing native delta
   back into `pMapDeltas` and writes the session through
   `encodeSessionWithVisitedMaps`. `startMapTransition` returns void, so it
   needs a result or a handled throw before the MM6 path goes live.
6. `DoPrepareWorld(false)` (around 717): three hooks, no-ops when the target
   isn't cached. `returnBeforeLoad` checks the delta and respawn policy before
   loading. `returnBeforeDecorations` installs the event entry before
   decorations. `returnBeforeMapEvents` replays actors, sprites, decorations
   and NPCs before map events, keeping the current global chest ledger and
   RNG. After post-world validation, take the owner from the cache. On
   failure, abandon to the main menu as the existing return probe does.
7. `SaveLoad.cpp`: the MM6 save path writes the active branch plus
   `encodeSessionWithVisitedMaps` through the atomic writer and returns a
   required success result. The load path decodes it, restores the active map
   through the existing per-branch loader, installs the cache and writes each
   visited owner's native delta back into `pMapDeltas`.

## Blockers to clear first

- No MM6 archive encoder exists (`decodeMM6ArchiveEntry` only).
  `pMapDeltas` holds encoded entries, so `encodeMM6ArchiveEntry(Blob)` is
  needed.
- `Party::restAndHeal` writes through the `pParty` global instead of `this`,
  and max HP/SP read buffs from the global party. A copy-safe version is
  needed.
- Only first-map outdoor live capture exists. Later-map outdoor and indoor
  live capture are needed.
- The original's four hireling indices (5, 6, 7, 44) are not proven to be
  Guide, Tracker, Pathfinder and Explorer.
- The per-frame day rollover may add to the no-rest counter again after the
  multi-day jump. Check at wiring time.
- Only the outdoor return plan exists, and neither owner kind is published.
  `canPublish()` and `canWriteCheckpoint()` stay false.
- Inferred, not run: the Weak timestamp uses the post-jump time, following the
  trace order. Arrival height is left to placement on the target map.
