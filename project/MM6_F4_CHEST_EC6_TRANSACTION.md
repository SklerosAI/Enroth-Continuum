# MM6 `.ec6` CHST and first-map movement audit

Status, 2026-11-06: `CHST` v1 is the seventeenth **required** section of the offline known-save assembly. Old sixteen-section `.ec6` candidates reject before any owner assignment. This is a provisional schema revision; there is no live MM6 save or reload route.

`CHST` stores a version byte, the active lowercase `.dlv`/`.ddm` companion name, the MM6 title-local 32-bit chest LCG state, exactly 30 binary generated-artifact flags for IDs 400–429, and the full decoded active delta sidecar (`pIndoor->mm6DeltaBytes` or `pOutdoor->mm6DeltaBytes`), capped at 8 MiB. `ready` is a capture precondition, not an on-disk bit; validated restore sets it. Capture and restore reject foreign titles. Reset uses `resetChestTitleState`, clearing the whole owner. No `grng` or inherited MM7 `Party::isArtifactGenerated` value participates.

The active MAPS entry is a changed-state encoding and need not be byte-identical to the retained raw sidecar. The known-save decoder binds the sidecar's delta name to `HEAD`. The 67-map preflight decodes both the active MAPS delta and sidecar against the same base, checks their actor/sprite/chest counts (and indoor door count), and validates item IDs in both. The detached session stage returns an owned `ChestTitleState` and sidecar copy after map, visual and Party checks; it does not commit `vChests`, map globals or the process title owner. The chest map-load adapter still must stage all 20 runtime chests together with the sidecar and swap them only after complete validation. Matching record counts alone do not prove identical map provenance or all chest projection semantics; that remains a live reload gate.

The offline first-map builder derives `CHST.rawActiveDelta` from the decoded trusted default `oute3.ddm` entry and retains the supplied title-local LCG/flags. The caller must prove that seed and reset/initialization lifecycle before the live `saveNewGame()` guard can open. A synthetic seed in unit tests is only test data. The fixed `autosave.ec6` writer still preflights every map pair before disk mutation.

## First-map movement state still missing

`PART` currently saves position, view yaw/pitch and the active character, but not the movement state below. Source consumers show why a positional reload alone cannot guarantee the first outdoor frame or a later mid-air reload:

| Runtime field | Current source-backed role | Gate |
| --- | --- | --- |
| `Party::uFallStartZ` | `saveNewGame()` sets 340 for `oute3.odm`; outdoor fall damage compares it with current Z. | Preserve or set by an explicit verified first-map policy before unpausing. |
| `Party::velocity.z` | Outdoor gravity, jumping and landing update vertical speed; X/Y are recomputed from input each frame. | Save vertical speed or prove a safe reset policy. |
| `Party::sPartySavedFlightZ`, `bFlying` | Outdoor flight/landing code reads and updates both. | Preserve together with active flight buff and height policy. |
| `Party::_6FC_water_lava_timer`, `uFlags` | Engine applies timed water/lava and burning effects using both. | Preserve their coupled state or prove a reset rule. |
| `Party::uFlags2` | Running changes recovery speed. | Preserve or explicitly clear with a first-frame policy. |

`EntitySnapshots.cpp` serializes these inherited fields, which is evidence of existing runtime ownership but not proof of native MM6 save meaning. `Party::Zero()` and `ResetPosMiscAndSpellBuffs()` clear several of them; using either during reload without restoring the fields would alter gameplay. Floor-face ID and walking sound appear frame-local and need an explicit recompute decision. An MM6 movement codec and first-frame headless test are still required before live reload.

Remaining all-owner blockers: MM6-keyed SHOP/guild/bounty state, beacon map/image state, movement above, active world reconstruction and PID rebinding, chest `vChests`+sidecar atomic commit, transient action/UI policy, verified new-game chest seed, disk recovery and rendered save/quit/relaunch/load. The `SaveLoad.cpp` MM6 guard remains closed.

Focused evidence: the standalone native CHST suite passed 2/2 nonzero and late-failure cases; the freshly linked known-save suite passed 8/8, including old sixteen-section rejection. All changed C++ translation units passed arm64 syntax compilation with the integrated build commands, and cpplint/diff whitespace passed. The map-sidecar and first-map tests require the parent's clean integrated rebuild because the old static libraries carry the previous `KnownSaveFields` layout. No MM6 process or proprietary files were opened.
