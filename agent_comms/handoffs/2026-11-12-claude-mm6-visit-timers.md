# MM6 visit record and timers handoff

- Owner: Claude Code lane VISIT-TIMERS, reviewed by the Claude lead.
- Branch: `claude/mm6-visit-timers` in the `mm6-indoor-light` worktree, from `master` `6156c7801`. Not pushed.
- Follows `2026-11-11-claude-mm6-visit-record.md` and closes four of its open items.
- Private trace with addresses: `local-game-data/private-wiki/mm6/VISIT_TIMERS_PE_TRACE_2026-11-12.md` (not in Git).

## Commits

1. `test(mm6): pin the Sorpigal fountain at 30 drinks`. Test only.
2. `fix(mm6): stamp the first map's visit into a first-map save`. `MM6V5LiveSaveCapture.cpp`, a fresh-process pair in
   `GameTests_MM6MapVisit.cpp` and a ninth profile in `tools/run_mm6_firstmap_gate.py`.
3. `fix(mm6): stamp the death map's visit before the week passes`. `MM6PartyDeath.cpp` and one test.
4. `test(mm6): pin the first alarm of a timer without an interval`. Test only.

No shared-core file changed. Every source change is in an MM6-owned file.

## What MM6.exe does

Static reads of `MM6.exe` with `llvm-objdump`, addresses virtual.

- Fountain. oute3 event 130 (OnLongTimer, daily at 00:00:01) sets map bytes 49 and 50 to 30. Event 131 compares byte
  49 with >= 1, subtracts 1 and adds 5 hit points. Map-byte Compare is `byte >= value` (0x44035d), Subtract stores the
  byte minus the value with no floor (0x4429b0), Set stores the low byte (0x44107a), Add saturates at 255 (0x441e79).
  The bytes live at 0x5b22fc. 30 drinks heal, the 31st is dry, as in the port.
- SaveGame 0x44f320 stamps the playing time into the current map's visit block on every call (0x44f462 indoor,
  0x44f47c outdoor) and, unless its second argument is set, writes the map (0x44f9e2).
- Party death 0x453bbd calls `SaveGame(0, 0)` at 0x453d42, before the deaths counter, the award and the 7 days
  (0x453d87), then loads oute3 if the party is elsewhere (0x453ef3, 0x454053). The death map keeps its state and is last
  visited at the death.
- Map timers 0x439940. Only an OnLongTimer without an interval can fire at once: when the last visit is zero or a day
  or more ago (0x439afd-0x439bce). Every other timer, OnTimer included, gets a calendar alarm from now: now + 336, 28 or
  7 days, or for a daily one tomorrow at the record's hour, minute and second (0x439bd3-0x439dad). The hour byte is
  used as is, so t4 events 16, 17, 18 (hour bytes 88, 208, 244) go off 88, 208 and 244 hours after the next midnight.
  The runtime check (0x43e939) adds one period per firing.

## Findings and fixes

1. The fountain counter was never wrong. The other 16 of the old test's 46 clicks hit the Statue decoration (#263) in
   the middle of the basin, because `pointMouseAtFace` aims at the centre of the water face's bounding box, which is the
   statue's foot, and the statue's sprite covers it on some frames. The test now aims 20 pixels lower, onto the near
   water, and checks that drink 30 heals and drink 31 is dry.
2. The first-map v5 capture now stamps the active delta's visit block with the playing time and the shown sky, as the
   v7 session capture does. The legacy capture in `MM6WorldSaveRuntime.cpp` (`captureKnownWorldFromActiveMap` and the
   `encodeMM6*ChangedState` writers it calls) has no caller outside that file and unit tests, so no player reaches it.
   It is left as is.
3. `respawnDeadParty` parks the death map right after the losing movie, before the award and `applyPartyDeath`, so its
   last visit is the time of death. The map's monsters, chests and bytes were already parked as they were at death.
4. Confirmed the timer rule the previous lane wrote. The new test enters t4 (Temple of the Sun) through its door on
   outd2 at 09:00 on day 0 and ticks 123,600 frames. The temple stays dark until 16:00 on day 4, when event 16 turns
   both lights on and the window to `orair256`. Every t4 load sets the night state, because event 19 (OnMapReload)
   compares the hour of day with 2100, 2000, 600 and 500, which never matches in MM6's hour-equality compare
   (0x43feca), so it falls through to the night branch. Event 15 (21:00) sets the same night state and shows nothing.

## Checks

- Revert and fail, per fix:
  - Fix 2: with the stamp removed the reload child loads oute3 with a zero last visit and the fountain heals.
  - Fix 3: with the park back after `applyPartyDeath`, d02's last visit is the waking time and the test fails.
  - Test 1: with the old aim the 31st drink still heals.
  - Test 4: with the MM6 branch of `registerTimerTriggers` disabled, all four t4 timers fire at the first timer check
    after entry, and the temple tape goes night, sunset, night, lit, night before day 4.
- Full MM6 suite (`MM6*.*`, one process, on `a1e90b5e7` plus the t4 test): 190 passed, 2 skipped (the two
  `MM6ZSaveHeadless` research tests). The t4 test took 335 s there under load.
- `tools/run_mm6_firstmap_gate.py --skip-build` after commit 2: every child passed, the new first-map visit pair and the
  three MM7 controls included.
- `Run_UnitTest`: 1122 passed, 45 skipped.
- `check_style` clean. `check_tidy` not run, no clang-tidy 22 here.
- MM7 controls beyond the gate's three not run, no shared-core file changed.
- Capture `mm6-visit-timers` (New Sorpigal gate on New Game): scene, sky, portraits, bars and buttons look right. The
  food and gold counters next to the apple and the coins are empty, no numbers at all. Not caused by this branch, none
  of its files draw the HUD.

## Data checked

- `oute3.evt` events 130 and 131, `d02.evt` chest events (12, 27, 28, 29), `t4.evt` events 13 to 23, `outd2.evt` event
  92 (the t4 door) and 219, `d01.evt` 64 and 65, `d04.evt` 56 and 57. Every OnTimer and OnLongTimer record in all 83 MM6
  `.evt` files was listed again. `mapstats.txt` rows for d02, t4 and outd2 (refill 672, 672 and 224 days).
- `outd2` event 219 (weekly) only sets map byte 9, and no outd2 event reads it, so that timer has no visible effect.
  `d01` event 65 re-arms event 64's Wizard Eye cast, which MM6 casts with no effect. Of the seven OnTimer triggers
  without an interval only t4 16, 17 and 18 change something a player sees.
- MM7 data was not scanned, no MM7 path changed.

## Open

- `MM6MapByteState` throws where MM6 wraps (Subtract below zero) or saturates (Add above 255). No shipped script was
  checked for either case.
- The new t4 test ticks 123,600 frames and takes about three and a half minutes in Debug.
- The port's `checkTimer` still catches up in one go where MM6 adds one period per check.
- The chest window logs a missing `ib-bcu-a` icon when the d02 chest opens. Not looked into.
- MM6 fog is still not implemented, the visit block's fog dwords are carried.

Privacy: addresses, map stems and event numbers only. No game data, saves, captures, logs or disassembly in Git.
