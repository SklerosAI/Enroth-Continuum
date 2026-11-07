# MM6 post-world Armageddon resume plan

Status, 2026-11-07: detached, read-only plan and final agreement check. The
new source and test translation units pass AppleClang syntax checks; linked
tests and an actual v2 child handoff remain to be run in integration. This
does not open normal MM6 SaveLoad or Atlas Play.

`RCVR` v3 retains the current runtime's timer, damage and force tuple. The
MM6 Party stage restores all three before `Game::gameLoop()` calls
`DoPrepareWorld(true)`, but `Engine::ResetCursor_Palettes_LODs_Level_Audio_SFT_Windows()`
sets `Party::armageddon_timer` to zero. It leaves damage and force in place.
The next loop can use a nonzero timer for outdoor actor panic, lighting and
the eventual damage pulse, so the lost timer is observable.

`preparePostWorldArmageddonResume()` validates the saved `RCVR` range and an
explicit final MM6 level type. A nonzero timer requires an outdoor map, as
the current effect pulse asserts outdoor. It requires the post-world timer
to be zero, the Party clock to match `RCVR`, and damage/force to remain exactly
as staged. It returns the saved timer and its coupled values as a small plan
without changing the Party. `requirePostWorldArmageddonResumeAgreement()`
checks the full tuple and level type after a future all-owner publisher has
restored that one timer. The focused test includes an active outdoor timer,
quiet indoor state, wrong title/map, preexisting timer, changed clock/damage/
force and source nonmutation.

This is a policy for preserving the fork's current effect state across a
save/reload, not proof of original MM6 executable semantics. `onMapLoad()`
and map reconstruction can affect other owners before this hook; generated
spell sprites, actor panic and game RNG still need their own provenance.
No live timer is assigned by this helper. The existing pre-world
`WorldPrepareClearsArmageddonTimer` blocker must remain until a complete v2
child publishes the timer at the checked post-world edge and passes a first
frame with its effect state intact.
