# Claude MM6 GrayFace travel lane, 2026-11-18

Branch `claude/mm6-grayface-travel`, rebased onto `claude/mm6` at `2ce7c9c68` (street NPC follow-ups merged). Four
GrayFace backlog items from `project/MM6_GRAYFACE_PATCH_REFERENCE.md`, one commit each, each behind its own default-on
flag. The reference rows and the owner-decision rows for W1, W7, W8 and W9 are updated in the same commits.

## Commits

- `9e08ee753` W9, don't offer the map crossing while flying. `Outdoor.cpp` offers through the new
  `borderOfferPartyStatePermits` (flags plus the confirmation's flying check), flag `gameplay.fix_flying_border_offer`.
  Unit test `ConfirmationChecksFlyingSeparately` became `OfferRefusesFlyingLikeTheConfirmation`. Game test
  `MM6MapTravel.FlyingLowOverTheEdgeOffersNoTrip`: on the slope at (-22425, -13896) down to the oute3 west edge, walking
  opens the dialog, flying low (Fly cast from the spellbook, one frame of PageUp, which keeps the take-off height just
  above the falling ground) is held at x = -22528 with no dialog.
- `7763312f4` W7, autosave before the fare. `MM6Transport.cpp` keeps the fare and the trip days and pays them in
  `depart()` after `startMapTransition` wrote the autosave. Flag `gameplay.fix_travel_autosave`. Game test
  `MM6Transport.CoachAutosaveKeepsTheFare` takes the New Sorpigal coach and loads `autosave.ec6` from the Escape menu:
  200 gold, the pre-trip date, at the stable door.
- `f3d847ae9` W8, travel speech delay. `Character::playReaction` returns the voice `SoundId` it played (shared code,
  MM7 callers ignore it), `mm6::transportDepartureDelayMs` waits for its length capped by 1500 and 2500 ms, flag
  `gameplay.fix_travel_speech_delay`. Without a sample the vanilla delay stays (GrayFace leaves at once then).
- `fab0e8a74` W1, timers count from the last firing. `Processor.cpp` records each MM6 `OnLongTimer` firing per map and
  interval in `mm6::mapTimerFirings()` and schedules a map load from it (`mapTimerFirstAlarmAfterLastFiring`), with
  GrayFace's validate fallback to the last visit, its "missed alarm fires once, next one interval later" retrigger and
  its daily-`OnTimer`-today rule. Flag `gameplay.fix_map_timers`. The firings are saved in a new optional EC6 section
  `TIMR` after `PRKM` (`MM6MapTimerSection.cpp`), no version bump, so v7 saves without it still load and fall back.
  `decodeV7SlotShell` accepts the optional `TIMR`, `forgetParkedMaps` clears the firings for new and loaded games.

## Verified against the data and the patch

- GrayFace 2.5.7 source: `Hooks.pas` 1623-1651 (TravelGoldFixHook, TravelWalkTriggerFixHook), 2316-2445 (FixTimers),
  `MMHooks.pas` 3647-3690 and 4096-4100 (TravelDelayHook), `MMCommon.pas` 386 and 1905 (TMapExtra periodic timers),
  1353 (speak time limits). Vanilla timer setup from `local-game-data/private-wiki/mm6/VISIT_TIMERS_PE_TRACE_2026-11-12.md`.
- MM6 timers: a throwaway game test read every map's `.evt` in the MM6 map table. `OnLongTimer` without a half-minute
  interval: outa2, outb2, outb3, pyramid weekly, outc1 (2), outc2 (2), outc3, outd1 (2), outd2 monthly, oute3 weekly,
  monthly and daily (130, the fountain), outb1, outc2, outd1, outd3, oute2, d04 (12:00), d09, sewer daily. Daily
  `OnTimer` without an interval: d01, d04 (12:00), t4 (hours 21, 88, 208, 244). Weekly `OnTimer`: outd2 219. Same-kind
  timers on one map share one firing, as in GrayFace. MM7 and MM8 data were not scanned, the change is MM6 only.
- MM6 voices: the probe showed MM6 voice samples exist (5002 + 100 * face band), but the shared `speechVariants` table
  has no variant for speeches 71 and 72 in either title, so `playReaction` plays nothing for travel. W8 is unobservable
  until travel voices are wired, the cap applies, `MM6Transport.CoachLeavesAfterTheOriginalDelayWithoutSpeech` pins it
  (16 frames: the coach leaves at 1.5 s, the house closes a frame later).

## Behaviour changes to know

- The Temple of the Sun (t4) dawn timer, hour byte 88, now first goes off 88 hours after the arrival day's midnight,
  GrayFace's count, a day earlier than before. `TimerWithoutIntervalWaitsFromTheNextMidnight` became
  `TimerWithoutIntervalWaitsForItsTimeOfDay` with day 3 16:00. Later firings still step one day, GrayFace would step 88
  hours from each firing's midnight for this out-of-range hour. Not changed.
- With W7 the gold drops when the coach leaves, not on the click, as in GrayFace.

## Fixtures

The sorcerer gets Fly (Air 4 novice) and 100 SP for the W9 test. The fountain test sets `NoDamage` for the ten hours it
ticks in d02 (the dungeon's monsters refuse a rest there). Save tests use an owner-only `mm6-restart-*` temp profile.
Everything else is input.

## Test results (after the rebase onto `2ce7c9c68`)

- Revert-and-fail, before the rebase: each new test run with its flag set to false in the test body fails
  (`FlyingLowOverTheEdgeOffersNoTrip`, `CoachAutosaveKeepsTheFare`, `FountainRefillCountsDownWhileAway`). The fountain
  test also fails with `captureSessionSave` not writing the firings, so it needs the save round trip.
- `MM6*` game tests, MM6 data, no `--rules-path`: 298 ran, 296 passed, 2 skipped (`MM6ZSaveHeadless`
  private-research fixtures).
- `OpenEnroth_UnitTest`: 1124 passed, 42 skipped (private fixture env not set).
- MM7 controls `Prs.Pr2354`, `Prs.Pr2956`, `Issues.Issue840` 3/3, plus MM7 travel tests `Issues.Issue331` (foot
  travel) and `Issues.Issue2932` (stable travel line) 2/2.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/grayface-travel/before.png` and `after.png`, the party flying low into the
oute3 west edge. Before: the travel dialog ("5 days to Castle Ironfist") is offered. After: no dialog, the party is held
at the edge inside a tree's sprite. The travel dialog's Yes and No buttons draw as red prohibition signs, the icons
`x_ok_u` and `x_x_u` are missing from the MM6 `icons.lod` (pre-existing, not touched).
