---
type: synthesis
up: "[[MM6-F4-Save-Remainder]]"
tags: [mm6, save, recovery, rest, calendar]
---

# MM6 F4 Recovery and Calendar

This page records the bounded `RCVR` section and the guarded restoration policy for current runtime recovery, rest days and daily spell-use counters.

`RCVR` v1 stores a playing-time anchor, last regeneration, days without rest, four character and four party turn-based recovery durations, and three daily-use counters for each character. It is a required exact-size section. The anchor must agree with `PART`; values are bounded and decoded before a destination is assigned.

The seven cached calendar members are not stored: current source recomputes them from playing time during timed updates, and `Party::Reset()` initially leaves several caches stale. Staging derives them from the already staged core clock, using the current runtime's zero-based month/week/day convention. This ensures source consistency; the correct MM6 calendar epoch and spell-day rules still need gameplay proof.

Capture and copy-staging require active MM6 and reject active rest/wait and turn-based mode. The rest UI's remaining duration, hourglass/window and turn queue are not in this field. The saved party must first receive `PART` clock state; then `RCVR` changes only a copy. It does not resume a rest or mutate the live UI/timer. The full save manifest and SaveLoad remain untouched.

The linked arm64 unit target built and two focused tests passed round-trip, clock/calendar agreement and malformed/wrong-title no-mutation checks. No partial MM6 game or original installation was opened. See the [implementation packet](../project/MM6_RECOVERY_CALENDAR_F4.md) for every field, source consumer, bound and F5 scenario.

See also: [Save Remainder](MM6-F4-Save-Remainder.md), [Core Save Fields](../project/MM6_EC6_CORE_FIELDS_F4.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
