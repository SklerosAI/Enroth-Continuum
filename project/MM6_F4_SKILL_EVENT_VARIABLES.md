# MM6 active skill event variables

Status, 2026-11-07: the MM6 event interpreter handles the active skill
Compare and Set operands by title. The retained active EVT inventory uses
only these two operations in the MM6 skill range; its 68/68 active resources
still parse. The focused skill, variable and private-corpus units passed 15/15.
A headless MM6 interpreter scenario passed 1/1, and an MM7 headless control
passed 1/1.

MM6 raw event IDs 56–86 map to its 31 source skill slots. The shared
`Character` runtime uses the same first 30 indices, but Learning moved to
`SKILL_LEARNING`; the title-specific adapter maps source slot 30 explicitly.
The retained active operations include a level comparison and a novice skill
grant. The adapter compares plain level operands to the raw skill level,
retains joined mastery ordering for mastery thresholds, and rejects
Grandmaster or malformed Set values. Add and Subtract remain explicit
refusals because no active retained operand establishes their MM6 effects.
It does not invoke the inherited MM7 event variable enum for these IDs.

This is a bounded execution seam, not proof of the original quest outcome or
all skill-training behavior. F5 must observe the branch and grant in game,
then verify their save/reload state. The skill fields are represented by the
current `.ec6` character owner, but normal MM6 SaveLoad remains guarded.

See [variable dispatch contract](MM6_VARIABLE_DISPATCH_F2.md),
[event framing](MM6_F4_EVENT_FRAMING.md), and
[the active event interpreter](../src/Engine/Evt/EvtInterpreter.cpp).
