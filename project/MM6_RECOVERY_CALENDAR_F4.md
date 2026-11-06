# MM6 recovery, rest and calendar save field — F4

Status: bounded current-runtime `.ec6` field and copy-staging, 2026-11-06. This is an offline field seam; the partial known-save manifest now requires `RCVR`, while SaveLoad remains guarded.

## Source, consumers and lifetime

| Runtime field | Consumer and lifetime | Decision |
| --- | --- | --- |
| Four `Character::timeToRecovery` durations | Action gating in `Party.cpp`, `Character.cpp`, `UIGame.cpp` and turn engine; can span a save | Store exact nonnegative ticks. |
| Four `Party::pTurnBasedCharacterRecoveryTimes` durations | Spell, beacon and turn-engine handoff; can remain after a turn | Store exact nonnegative ticks. Reject capture/load while `bTurnBasedModeOn`, because the turn queue and pending actions are not in this field. |
| `Party::last_regenerated` | `RegeneratePartyHealthMana()` counts five-minute intervals from it | Store nonnegative ticks no later than playing time. |
| `Party::days_played_without_rest` | 3am day rollover, hunger/weakness and travel/rest paths | Store the current unsigned byte, including the full 0–255 representable range. Its eventual wrap is an inherited gameplay issue. |
| Three daily spell-use `char` counters per character | Divine Intervention and Armageddon checks, Fire Spike reset; 3am and rest reset | Store current nonnegative signed-char values 0–127. Negative/overflowed values fail closed. MM6 spell limits and Fire Spike semantics remain separate gameplay questions. |
| `PartyTimes.CounterEventValues[10]`, `HistoryEventTimes[29]`, `_s_times[20]` | `EvtVariables.cpp` reads/writes script counters, journal history and unknown-time events | Preserve all 59 raw `Time` ticks, including zero and expired `-1`, on a detached `PartyTimeStruct` copy. Current event-variable indices are not asserted to be native MM6 meanings. |
| Seven cached calendar members (`uCurrentYear`, month, week, day, hour, minute, second) | Outdoor light/season, shops/transport, spell time, dialogue | **Derived, not stored.** `Party::Reset()` initially sets only hour 9, so the cache can be stale before the first timed update. On stage, derive all seven from the saved `PART` clock via current `Time::toCivilTime()` and its zero-based month/week/day convention. This normalizes the cache without claiming MM6's calendar rules are correct. |
| `remainingRestTime`, `currentRestType`, rest window/hourglass | `UIRest.cpp`, `Party::restOneFrame()`, `Game.cpp` | **Not stored.** Active rest/wait is rejected before capture or stage; the user must leave the rest UI first. Resuming an in-progress rest needs a separate UI/action contract. |

The source audit uses [Party/Character declarations](../src/Engine/Party.h), [party rest updates](../src/Engine/Party.cpp), [timed effects and regeneration](../src/Engine/Engine.cpp), [rest UI](../src/GUI/UI/UIRest.cpp), and [the inherited MM7 snapshot](../src/Engine/Snapshots/EntitySnapshots.cpp). The ignored F3-matched content/schema reports cover game assets, not a native MM6 whole-save clock offset. No original installation was reopened and no native format claim is made.

## `RCVR` v2 contract

`MM6RecoverySaveFields.*` adds required section `RCVR` with an exact 572-byte v2 payload. It stores the former 100-byte prefix plus 59 signed 64-bit event timestamps in counter, history and unknown-time order. All integers use little-endian encoding. Event ticks may be `-1` (expired), zero or positive up to 10,000 elapsed years; this is a storage bound, not a claim about MM6 script behavior. The decoder rejects a missing/optional/duplicate section, bad shape/version/reserved byte, wrong title or container CRC before destination assignment. There are no live `.ec6` slots, so the prior offline v1 sample format is intentionally superseded.

The clock anchor must equal `PART.playingTicks`. `validateRecoveryAgainstCore()` provides this cross-section check; `stageRecoveryParty()` independently requires the incoming party copy to already have the same playing clock. Thus whole-save staging must apply core state first and validate both sections before committing the copy. On a valid stage, only regeneration, rest days, recovery, daily counts and derived calendar cache change. It does not alter live global rest UI, turn queue, OS/frame clock or MM7 runtime.

`captureRecoveryFields()` and `stageRecoveryParty()` require an active MM6 engine title, no active turn-based mode, and `currentRestType == REST_NONE` with zero `remainingRestTime`. These guards are intentional until the turn queue and rest action state have a complete save contract. The playing-time anchor is a consistency check, not a second source of truth. The smaller `captureEventClocks()` and `stageEventClocks()` functions take an explicit MM6 title and `PartyTimeStruct`; they validate first and return values or a copy without touching global party state. They are unit-testable without constructing a live `Party`.

Three focused C++ tests passed in the linked arm64 unit target: exact round-trip and derived initial calendar, cross-check against `PART`, nonzero event-clock capture/stage on a detached runtime owner, valid-container malformed shape/reserved/timer/daily/event fields, wrong-title rejection and no destination mutation. The seven known-save and five atomic-file tests also passed with the v2 payload. F5 must check real save/quit/relaunch/load at day rollover, after a short rest, with nonzero recovery and daily-use counts, plus rendered calendar, journal behavior and spell availability. No MM6 game process was launched in this lane.
