# MM6 active world and overlay PID restore — F4

Status: **detached saved-record preflight only; live MM6 SaveLoad remains closed** (2026-11-06).

`stageKnownWorldAndVisuals()` now obtains the decoded actor count for the active indoor `.dlv` or outdoor `.ddm` during its existing 67-map base/delta validation pass. After the map and item checks and detached world copy, it rejects each active `VISU` overlay whose packed PID is not an actor PID or whose actor index is outside that active saved delta. The existing overlay-table check then runs. This rejects a late invalid reference without assigning any caller-owned NPC, map, timer, or overlay state. It does not dereference or rebind a live PID.

The bound is justified by current source: `Pid` packs type in the low three bits and actor index above them; `ActiveOverlayList::_4418B6` is currently called only through `Actor::AddOnDamageOverlay`, with an actor PID. This is a **current-runtime producer contract**, not proof of an original MM6 save field. Inactive overlay slots are ignored by this specific check.

## World reconstruction order and outstanding identity risk

The inherited `DoPrepareWorld` first resets the current level and transient resources, then loads map events, changes the current map ID, and calls `loadAndPrepareBLV` or `loadAndPrepareODM`. Those loaders reconstruct actors, sprites and chests from the selected base/delta. Afterwards `DoPrepareWorld` repairs faces, sets NPC names, calls `_461103_load_level_sub`, applies map-specific actor edits, clears the pending transition, and runs `onMapLoad`. This is a mutating sequence, not a detached constructor or an atomic commit.

Both MM6 `IndoorLocation::Load` and `OutdoorLocation::Load` can replace a saved delta with a fresh one on initial or timed respawn; an invalid delta is caught and also leads to respawn in the current loader. Spawn and initialization paths can change actor or sprite arrays. `_461103_load_level_sub` can clear actors for the `NoActors` debug setting, rewrites generated NPC IDs, and invokes the still-gated MM6 chest generator. `onMapLoad` can execute events that mutate the world. Indoor/outdoor `arrangeSpriteObjects` changes item/floor state in place; it does not itself compact the sprite list, but other paths can change that list. A PID bounded by the saved actor count can therefore refer to a different actor or no actor after load.

Do **not** apply staged `VISU` overlays to reconstructed actors or open MM6 `SaveLoad.cpp` on this preflight alone. A full restore needs title-specific respawn and event policy, active-world reconstruction that can fail before touching live owners, post-reconstruction PID identity checks, chest generation, all remaining mandatory Party/Character owners, and a single failure-atomic commit. The inherited MM7 loader mutates Party/timer/overlays/NPC/map deltas before world preparation, so it is not the `.ec6` commit route.

Native synthetic tests cover accepted nonzero actor PIDs and out-of-range actor PIDs for both indoor and outdoor active deltas, a wrong PID type, all 67 base reads before late rejection, and unchanged caller owners. Retained fixture coverage remains in the existing 67-map tests. No MM6 game process or original installation was opened.
