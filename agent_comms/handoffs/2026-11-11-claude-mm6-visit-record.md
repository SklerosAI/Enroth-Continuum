# MM6 map visit record handoff

- Owner: Claude Code lane VISIT, reviewed by the Claude lead.
- Branch: `claude/mm6-visit-record` in the `mm7-death-regression` checkout, from `master` `fd3ae9a07`. Not pushed. Commits: `b7467f004` (fix, tests, gate pair), then this handoff.
- Files: new `src/Engine/MM6/MM6MapVisit.{h,cpp}` and its CMake library, `MM6OutdoorSky.cpp` moved onto it, `MM6MapTransition.{h,cpp}` (stamp on capture), one-line hooks in `Outdoor.cpp`, `Indoor.cpp` and the timer registration in `Evt/Processor.cpp`, the timer prediction in `GameTests_MM6MapSweep.cpp`, new `GameTests_MM6MapVisit.cpp`, and a VISIT pair in `tools/run_mm6_firstmap_gate.py`. No save format change, the visit block already rides inside every delta the EC6 v7 save holds.

## What MM6 does

Read from `MM6.exe` (GOG build, 851968 bytes, SHA-1 `b078dea4...`), disassembled with `objdump -d --x86-asm-syntax=intel`. Addresses are virtual.

- Live block. The outdoor location object is at `0x6297a8`, its 56-byte visit block at `+0x604d0` (`0x689c78`): int64 last visit time, 12-byte sky name at `+0x604d8`, then three dwords at `+0x604e4/e8/ec` (fog flags, near and far fog range) and 24 unused bytes. The indoor block is at `0x5f7d74` (indoor object `0x5f7aa8` `+0x2cc`). The outdoor loader copies the delta's 56 bytes in at `0x46dd70`, the indoor loader at `0x48c368`, right after the 200 persistent event bytes.
- Respawn. Both loaders read `new.lod` first. On a first or timed respawn (`lastRespawnDay` against the respawn interval, `0x46dab4`) they re-read the original delta from `games.lod` (`0x6104f8`, `0x46db59` and `0x48bf3a`) and take its visit block, so a respawned map is a first visit again. The port already does this, `fresh` keeps only the reset header and visibility. Respawn reads `lastRespawnDay`, never the visit time.
- Writer. `SaveGame` (`0x44f320`) does nothing on `zArena.blv`, otherwise stamps the party's playing time (`0x908d08`) into the current map's block (`0x44f462` indoor, `0x44f47c` outdoor) and copies the whole 56-byte block into the delta (`0x44fb74` indoor, `0x44fcb2` outdoor). The sky name and fog dwords are whatever the live block holds: the loader writes the sky it picked into `+0x604d8`, the fog roll (`0x47c2f1`-`0x47c3b1`) writes the fog dwords. A map change calls `SaveGame(1, 0)` when the target map name differs from the current one (`0x43de8b`-`0x43dea3`, the same at `0x43fa72`). The Save menu and the other 11 call sites go through the same function.
- Readers of the visit time. Only two: the outdoor sky pick (`0x46df0d`, rule in the sky handoff) and the map timer setup (`0x439940`, called once, from the map load routine at `0x456a41` after the map loader `0x469d70`).
- Timer setup, per `OnTimer` (`0x1f`) or `OnLongTimer` (`0x26`) trigger (jump table at `0x439e64`/`0x439e74`). An `OnLongTimer` without a half-minute interval goes off at once (alarm 0) when the last visit is zero, or when a whole day or more has passed since it (`0x439afd`-`0x439bce`, the year, month and week checks are covered by the day check). Every other non-interval timer gets an alarm one period from now: now plus 336 days, 28 days or 7 days for a yearly, monthly or weekly timer, and the timer's time of day tomorrow for a daily one (`0x439bd3`-`0x439dad`). Half-minute interval timers are a countdown that never reads the visit time. The runtime check (`0x43e939`) adds one period per firing.

## Data checked

- MM6 `Games.lod`: all 15 `.ddm` and all 52 `.dlv` deltas end in an all-zero 56-byte visit block.
- MM6 `icons.lod`, all 83 `.evt` scripts: 35 `OnTimer` triggers with a half-minute interval, 7 `OnTimer` without one (`d01` 65, `d04` 56, `outd2` 219 weekly, `t4` 15-18), and 23 `OnLongTimer` without one (9 daily, 9 monthly, 5 weekly), no `OnLongTimer` with an interval. `DBM1`-`DBM5.EVT` and `OUT.EVT` also hold 5- and 6-byte records with a timer opcode and no timer fields. My scan skipped them, I did not check what the engine makes of them. The `OnLongTimer` ones refill fountains and wells or reset map variables, e.g. `oute3` 130 sets the two Sorpigal fountain counters back to 30 each day.
- MM7 data was not scanned, the MM7 paths are unchanged.

## Fix

- `mm6::mapLastVisit` and `mm6::mapVisitSky` read the block, `mm6::recordMapVisit` stamps the time and, outdoors, the shown sky. Its fog and spare bytes keep what the delta held, the port has no MM6 fog yet.
- `captureActiveMap` stamps the delta it parks with the party's playing time and `pOutdoor->weather.skyTextureName`. It runs on every map change (`leaveMap`) and on every v7 save (`captureSessionMaps`), the two places MM6 calls `SaveGame`. The live map is not changed, its block is only read on load.
- `OutdoorLocation::Load` and `IndoorLocation::Load` set `lastVisitTime` from the loaded delta instead of zero (outdoor) or a stale value (indoor).
- `registerTimerTriggers` takes `mm6::mapTimerFirstAlarm` for MM6 non-interval timers. MM7 keeps its branch. `checkTimer` is unchanged for both.

## Checks

- New tests in `GameTests_MM6MapVisit.cpp`, all through input and `game.tick`:
  - `MM6MapVisit.LaterDayReturnRollsTheSky`: New Game, west edge to `outd3` and back (five days each way). The sky tape is `sky01`, `sky01`, then `sky18` on the return, and `oute3`'s last visit is the time the party stood at the edge, five days before its arrival in `outd3`.
  - `MM6MapVisit.SameDayReturnKeepsTheSky`: the same trip, then into `d02` and back the same day. The rolled sky comes back and the last visit moves to the time at the `d02` door.
  - `MM6MapVisit.FountainRefillsADayAfterTheLastVisit`: drains the Sorpigal fountain (face 863, event 131) until it only says "Refreshing!", then a same-day `d02` trip leaves it empty and a ten-day `outd3` trip finds it healing again. It took 46 clicks to drain, I did not look into why it is not 30.
  - With the four source hooks reverted (`Outdoor.cpp`, `Indoor.cpp`, `MM6MapTransition.cpp`, `Processor.cpp`) all three fail: the sky stays `sky01`, the last visit stays zero, and the fountain heals on the same-day return.
  - `MM6MapVisitHeadless.DISABLED_SaveInDungeonAfterSkyRoll` and `DISABLED_LoadedSaveKeepsTheVisit`, a fresh-process pair in the first-map gate: save in `d02` after the sky roll, load in a new process, walk out. Checks the rolled sky, `oute3`'s last visit equal to the door autosave's time and `d02`'s equal to the save's time. Both pass.
- `tools/run_mm6_firstmap_gate.py --skip-build`: all 33 children pass, the new visit pair included, and the MM7 controls.
- `check_style` clean. `check_tidy` not available here (no clang-tidy 22).
- `Run_UnitTest` binary: 1120 passed, 45 skipped.
- MM6 suite `MM6*`: 119 passed, 2 skipped.
- MM7 controls `MM7*:VendorRefreshIntegration.*`: 107 passed, 2 skipped.
- `Run_GameTest_Headless_Parallel` not run, per the lane rules.
- No capture taken. The sky change shows only on a revisit, and `enroth-capture --mm6-map` enters a map fresh, so it would show `sky01` either way.
- The owner's MM6 and MM7 profiles under `Profiles/` were snapshotted before the runs and compared after: unchanged.

## Open

- The first-map v5 save (a session that never left `oute3`) writes through `writeV5FirstMapNativeLiveSave` and is not stamped, a load of it is still a first visit of `oute3`. MM6 would stamp the save time. Stamping it means touching the v5 capture and its equality checks against `pOutdoor->mm6DeltaBytes`.
- The legacy capture paths in `MM6WorldSaveRuntime.cpp` (`encodeMM6*ChangedState`) are not stamped.
- MM6 fog is not implemented (`OutdoorLocation::SetFog` returns for MM6), so the block's fog dwords are carried, not written.
- `OnTimer` without an interval no longer goes off on map entry, MM6 never fires it then. This changes `d01`, `d04`, `outd2` and `t4`. The map sweep predicts these alarms a day or a week out, so it ticks to its frame cap on those maps.
- The `game.tick(10)` waits for the first timer check after a load were not trimmed to the minimum.
- MM6's runtime check adds one period per firing, the port's `checkTimer` catches up in one go. Not touched.
- The party-death path calls `leaveMap` and so stamps the map it dies on. I did not read MM6's death path to see whether it saves the delta.

Privacy: no game data, saves, captures, logs or disassembly in Git.
