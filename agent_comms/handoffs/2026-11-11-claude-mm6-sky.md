# MM6 outdoor sky handoff

- Owner: Claude Code lane SKY, reviewed by the Claude lead.
- Branch: `claude/mm6-sky` in the `mm7-death-regression` checkout, from `master` `6114f76e8`. Not pushed. Commits: `5d688405d` (fix and test), then this handoff.
- Files: new `src/Engine/MM6/MM6OutdoorSky.{h,cpp}` and its CMake library, the MM6 branch of the sky selection in `OutdoorLocation::Load` in `src/Engine/Graphics/Outdoor.cpp`, the removed `src/Engine/Graphics/OutdoorSkyResources.h`, a new test and a shared map-entry helper in `test/Bin/GameTest/GameTests_MM6MapSweep.cpp`, and two dropped assertions in `GameTests_MM6.cpp`. No renderer file changed.

## Cause

The MM6 outdoor loader ran MM7's sky rule. MM6 drops the 56-byte visit block at the end of its `.ddm` deltas and resets the last visit time to zero, so every map took MM7's first-visit sky, `plansky3`. MM6's `BITMAPS.LOD` has no `plansky3`, and a missing bitmap draws as the `pending` placeholder. Two maps escaped. `oute3` names `plansky2` in its `.odm` header, which an earlier preflight hook used. `oute1` drew `sky19` through OpenEnroth's winter snow hack, which only fires on certain calendar days.

## What MM6 does

From `MM6.exe` (GOG build, 851968 bytes), disassembled with `objdump`:

- The loader copies the 56-byte visit block of the delta into the live map, the last visit time at its start and the 12-byte sky name right after it. The `.odm` header goes to a separate field that the outdoor load never draws. Only the debug location and `Release()` (`sky043`) write it.
- After the delta, the sky name becomes `sky01` when the last visit time is zero. When the last visit's day modulo 28 differs from the current day of the month, it rolls `sprintf("sky%02d", ...)`. That is `rand() % 100 < 20` from the 7-entry table {2, 5, 10, 13, 16, 18, 19} and otherwise from the 9-entry table {1, 3, 6, 7, 8, 9, 12, 14, 15}. On the same day it keeps the stored name. The day is computed with the float 0.234375 (30/128), the same tick conversion as `Time::toDays()`.
- No snow branch exists in this code.

## Data checked

- All 15 MM6 outdoor maps in `Games.lod` (`outa1`-`oute3` `.odm`): only `oute3` names a sky (`plansky2`). The other 14 headers are empty.
- All 15 `.ddm` deltas in `Games.lod`: the 56-byte visit block is all zeros in every one, so every first visit shows `sky01` in the original.
- `BITMAPS.LOD`: has `plansky1`, `plansky2`, `sky01`-`sky19`, `sky_clo1`, `sky_nit1`, `sky_nit2`, `sky_sns1`. No `plansky3`. Every name the MM6 tables can produce exists.
- MM7 data was not scanned, the MM7 path is unchanged.

## Fix

`mm6::outdoorSky` implements the rule above from the delta's visit block, the current day of the month and `vrng`. `OutdoorLocation::Load` calls it for MM6 only, ahead of MM7's unchanged chain (snow, revisit roll, `plansky3`). MM7 and MM8 take exactly the branches they took before.

## Checks

- New `MM6OutdoorSky.FirstVisitSky`: New Game into `oute3`, then each of the 15 outdoor maps at its first shipped entrance. Checks the outdoor sky texture is `sky01` and resolves in the MM6 archive. Passes. With the fix reverted it fails on all 15 maps (`plansky3` on 14, `plansky2` on `oute3`).
- Captures: before `20261111-122658-sky-before-outa1.png`, `20261111-122702-sky-before-oute3.png`, `20261111-122708-sky-before-outc2.png`. After `20261111-123407-sky-after-outa1.png`, `20261111-123412-sky-after-oute3.png`, `20261111-123419-sky-after-outc2.png`, `20261111-123425-sky-after-oute1.png`, `20261111-123431-sky-after-outd3.png`. All under `local-game-data/progress-captures/`. The after images of `outa1`, `oute3`, `outc2` and `outd3` show a cloudy blue sky. The `oute1` entrance view is under a roof and shows only a sliver of sky, so it confirms nothing either way.
- MM7 capture `20261111-130807-sky-mm7-check-out02.png`: MM7's usual sky, as before.
- `check_style` clean. `check_tidy` isn't available here, the build has no clang-tidy 22. `Run_UnitTest` binary: 1120 passed, 45 skipped. MM6 suite `MM6*`: 116 passed, 2 skipped. MM7 controls `MM7*:VendorRefreshIntegration.*`: 107 passed, 2 skipped.

## Open

- OpenEnroth never writes the last visit time or sky into the MM6 visit block when the party leaves a map or saves. Every load is therefore a first visit and shows `sky01`. The revisit roll only matters once the MM6 save writer stamps that block, which is save-lane work. The rule is already in place for it.
- `pOutdoor->lastVisitTime` stays zero for MM6. The event processor reads it for timers, and that is not touched here.
- The near ground in `oute3`, and patches in `outd3`, draw white like snow in the captures. Not a sky issue, not investigated.
- `.llm-wiki/MM6-F4-First-Map-Preflight.md` and `project/MM6_F4_FIRST_MAP_PREFLIGHT.md` still say MM6 keeps the base map sky name. That claim is superseded by this handoff.

Privacy: no game data, saves, captures or logs in Git.
