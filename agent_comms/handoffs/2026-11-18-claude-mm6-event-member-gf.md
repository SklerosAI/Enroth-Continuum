# Claude MM6 event-member-gf lane, 2026-11-18

Branch `claude/mm6-event-member-gf`, off `claude/mm6` at `9eb81732f` (master `23f6eb0c9` plus the shop-unconscious
lane). GrayFace backlog item W2 in `project/MM6_GRAYFACE_PATCH_REFERENCE.md`. Owner's choice 2026-11-18: match the
GrayFace patch.

## Commits

- `38fa7b089` Run MM6 events with nobody active for one member who can act. `canRunInteractionEvent()` in
  `Interaction.cpp` lets an MM6 click (`Engine.cpp`) or Space press (`DoInteractionWithTopmostZObject`) on an event face
  or decoration run its event with nobody active, as MM6.exe does. MM7 still refuses. `EvtInterpreter::iterateCharacters`
  (now a member) resolves the active target, when nobody is active in MM6, to one member who can act, picked on the
  first command that needs it and kept for the run. Flag `gameplay.fix_inactive_party_events`, default on. Off gives
  the vanilla per-command re-roll, not the old refusal. The W2 rows of the reference doc are rewritten.

## Verified against MM6.exe and the data

- MM6.exe (owner's GOG copy, disassembled with llvm-objdump): click handler 0x4206D0 is called by the click and
  interact messages at 0x42D4E6 and 0x42D501. Its event calls 0x4209C1, 0x420A41, 0x420A9B and 0x420ADC have no
  CurrentMember (0x4D50E8) read before them. The one read at 0x420B3A is on the actor path. The processor entry
  0x43C7C0 falls into 0x43C87C, which sets the target to 4 with a CurrentMember and 6 without.
- GrayFace 2.5.7 `InactiveMemberEventsHook` / `InactiveMemberEventsProc` (`MM6Patch/Hooks.pas:464-507`, hooked at
  0x43C7C0): a Delphi `Random(n)` pick among members for which `_Character_IsAlive` (0x4876E0, CanAct) is true, member 1
  when none, written to CurrentMember for one processor run and cleared after. No ini switch. Readme line 258 "You
  could drink from fountains multiple times if all party members are inactive."
- MM6 oute3 events 110 (Well, face 921: Luck >= 15 refuses, else MapVars[52] >= 1 takes a drink and adds 2 Luck,
  monthly refill to 8) and 150 (face 534, sets a might bonus of 10), dumped with `EvtInstruction::toString` in a
  throwaway test. Space can't reach the well's water face from any of the positions tried (it picks wall face 935),
  so the Space test uses face 534. Only MM6 data was in scope.

## Fixtures

None. Everything is input: a new game through `startMM6NewGame`, a teleport in front of the face, ten frames for the
monthly refill, A four times so every member recovers, then left clicks or Space.

## Test results

- New `MM6EventMember.ClickWellWithNobodyActive` (two iterations, member active and nobody active, eight clicks each)
  and `MM6EventMember.SpaceAtMightFaceWithNobodyActive`. With the source fix reverted both fail, the nobody-active
  iteration shows only the refusal and no luck, might or water change. With the flag off the nobody-active iteration
  fails the vanilla way: the second member goes 16 to 18 and the third 13 to 19.
- All MM6 game tests `MM6*`: 259 passed, 2 skipped (`MM6ZSaveHeadless`, private research inputs), 0 failed, 126
  disabled.
- `OpenEnroth_UnitTest`: 1125 passed, 45 skipped (private-fixture env inputs not supplied), 0 failed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840` plus `MM7GiantsWishWell.*`: 6/6 passed.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/event-member-gf/before.png` (refusal line) and `after.png` ("+2 Luck
permanent", the fourth member's portrait reacting), from a temporary `MM6OffscreenFrame.DISABLED_EventMemberCapture`
(removed).

## Found, not fixed

- GrayFace's CurrentMember also steers the other active-member reads of the run. Ours doesn't: where an event moves a
  held item when it gives a new one (`MM6GiveItemEvent.cpp:34`, the `VAR_PlayerItemInHands` add in
  `EvtInterpreter.cpp`) still sees nobody active.
- Space can't drink from the New Sorpigal well, it picks the well's wall face 935. Not checked against MM6.exe.
