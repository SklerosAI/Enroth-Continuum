# MM6 generated NPC pool handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: the integration lane, or Codex on its return.
- Branch: `claude/mm6-npc-pool` in the `mm7-save-durability` checkout, commit `c4663d0d6` from `master` `805f5ac1d`. Not pushed.
- Shared core: `src/Engine/Engine.cpp` (a separate MM6 branch of the street NPC loop, MM7 unchanged). Otherwise MM6-owned files and tests.

## Cause and original policy

MM6 ran the MM7 rule for which actors get a generated street NPC, judging MM6 monster IDs by MM7's ID ranges. Six maps then needed 102-179 rows and refused to load. The original MM6 executable keeps 100 rows, rebuilds them on every map load, and gives a row only to actors whose monster family index is 40, 41 or 44 (the peasant families) and whose map NPC link is nonzero. It clears every other actor's link. It has no overflow check. Under the original rule no shipped map needs more than 58 rows (64 of 67 maps scanned). Addresses are in the commit message.

## Fix

The eligibility check is corrected, a separate MM6 loop follows the original, the capacity preflight counts only street NPCs, and every MM6 owner that encoded the MM7 rule now uses the MM6 one. That covers the post-map links, GNPC validation and replay, the IART identity check, the replay helpers and the post-rebuild PID anchor. The GNPC wire format is unchanged at version 1.

## Checks

- `outa3`, `outc3`, `d17` and `t3` now load and finish. `outc2` loads and then reaches the award-animation crash, fixed on `claude/mm6-award-anim`. `sewer` loads and then reaches its event spell timer.
- New `MM6StreetNpcs.Outc2LoadsWithItsStreetNpcs` (creation party, 58 street peasants pinned) and new unit cases. Revert-and-fail on the game test, the eligibility check and the PID anchor.
- Unit tests 1,089 passed. MM6 suite 94 passed. MM7 `MM7*:VendorRefreshIntegration.*` 107 passed. MM6 first-map gate passed with the MM7 controls. `check_style` clean.
- Not run: the opt-in v5 later-outdoor and v6 indoor chains, whose checks were edited.

## Follow-ups

The generated row contents still come from MM7 rules. `isPeasant` with MM7 ranges is still applied to MM6 IDs in `Actor.cpp` and `UIPopup.cpp`. `project/MM6_F4_POST_MAP_GENERATED_NPC_LINKS.md` and `project/MM6_F4_NATIVE_SIGNED_NPC_MAP_LOAD.md` describe the old rule and need updating.

Privacy: addresses, IDs and map stems only. No game text or data in Git.
