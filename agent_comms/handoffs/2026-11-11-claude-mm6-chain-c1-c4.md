# MM6 campaign chain C1-C4 handoff

- Owner: Claude Code lane C1-C4, reviewed by the Claude lead. Recipient: the lead for integration, then whoever picks up C2.
- Branch: `claude/mm6-chain-c1-c4` in the `mm7-monster-combat` checkout, from `master` `98cb96ab3`. Not pushed.
- Stopped after C1 at the coordinator's request (owner wrapping up for the week). C2-C4 were not started.

## Commits

1. `19543c608` Give a new MM6 party the opening letter. `src/GUI/UI/UIPartyCreation.cpp`, MM6 branch only. The
   original creation finalizer adds item 505 to the first character after that character's skill items (private
   Quick Start PE audit). Without it Potbello and Humphrey never get past their refusal lines, so the main quest
   could not start. Test `MM6NewGameHeadless.PotbelloPaysForTheLetter`. It failed with the fix reverted (no gold, no
   bit 82, Potbello's refusal line) and passes with it. Three `MM6Houses` tests that list the first character's
   backpack now expect the letter at its end.
2. `24a0baff1` Park the map when an MM6 game test teleports off it. `src/Engine/Components/Control/EngineController.cpp`,
   MM6 only. A cross-map `teleportTo` skipped `mm6::leaveMap`, so a teleport to an unvisited map stopped at the MM6
   chest generation guard. Test `MM6MapTravel.TeleportToUnvisitedMap`. Without the fix the same teleport to outb2
   threw "MM6 chest random-item generation is not implemented" in the C1 exploration run.
3. `71ee852cd` Add MM6 campaign chain chapter C1, councils A and C. Chapter, helpers and CMake line (see below).
4. This handoff.

## C1, councils A and C (passing)

`GameTests_MM6CampaignChainC1.cpp`. From C0's `save000.ec6` on oute3 to `save001.ec6` on outd1:

- Andover Potbello in A Lonely Knight, topic The Letter: 1000 gold, bit 82.
- Coach from the New Sorpigal stable to outd3 by input.
- Castle Ironfist (outd3 event 43, the castle asks, Yes, house 154): Wilbur Humphrey takes the letter (award 58,
  5000 gold) and gives the Kilburn quest (bit 86).
- Castle Fleise (outd1 event 34, house 162, Loretta Fleise is the only resident so the house opens into her topics):
  topic 79 sets bit 116.
- The nine stables by their door events (outd1 10, outc1 31, outb1 8, outb2 10, outc2 14 and 16, outc3 8, outd3 8,
  oute3 15) and the price fixing line (bits 147-155, the last sets 117).
- Kilburn's chest on outb2 (event 68, chest 1, bits 182 and 304), disarmed by the fixture's skill. A click on the
  shield's grid cell picks it up and a click on the first portrait puts it in that backpack. Space does not take
  items out of an MM6 chest.
- Humphrey topic 11 takes the shield: award 2, reputation +200. Fleise topic 80: award 6, reputation -200.

Exit tapes assert the next chapter's entry state: awards {2, 6, 58} on character 0, bits 117, 147-155 and 304 set,
82 (cleared by Humphrey's letter topic), 86, 116 and 182 cleared, the letter and the shield gone, reputation back to
0, no character at 0 HP. C1 runs in about 50 s of test time.

Fixtures (all in C1's first comment):
- Party strength, the one documented fixture: level 100, might, accuracy, speed and endurance 200, Disarm Trap 30
  master, full health, on every character. Without it the outb2 chest trap killed the C0 party (seen in the
  exploration run). It is kept by the later saves.
- No item or kill fixture. The shield comes out of its chest by input.

Teleports: to Castle Fleise on outd1, to the stables of outd1, outc1, outb1, outb2, outc2, outc3 and oute3, to Kilburn's
chest on outb2 and back to Castle Ironfist, plus same-map teleports to door faces. The only real travel is the coach
from New Sorpigal to Castle Ironfist.

New helpers in `MM6CampaignChain.{h,cpp}`: `useMM6CampaignChainEventFace` (loads the map with a teleport when needed,
then stands 150 units in front of the event's outdoor face and presses Space), `enterMM6CampaignChainCastle`,
`talkToMM6CampaignChainNpc` (by the "Converse with" label, or nothing when the house opened into that NPC),
`selectMM6CampaignChainTopic` (by global event id) and `leaveMM6CampaignChainHouse`.

## Split check against the scripts

Read from the private `global.evt` and map scripts with a throwaway decoder in the session scratchpad. Council
letters as the plan uses them: A Humphrey (award 2, Kilburn's shield), B Newton (award 3, Hourglass of Time), C Fleise
(award 6, stables), D Stone (award 5, Prince of Thieves), E Stromgard (award 7, the Hermit), F Temper (award 4, Devil
Plans). Every council hand-in ends by checking awards 32 and 2-7 and setting bit 167 (Oracle admission) only when all
are present. The traitor step (global 380, Slicker Silvertongue, NPC 304 in house 165) needs the Letter from Zenofex
(item 502) and gives award 32 and bits 167 and 168. The plan's split C1 A+C, C2 B, C3 D+E, C4 F+traitor holds.

## Where C2 starts and what it needs

C2 starts from `save001.ec6` on outd1 with awards 2 and 6. Council B, Albert Newton (NPC 5, house 156 behind Castle
Newton, oute2 event 30, the same castle Yes prompt as C1):
- Reach oute2: the outd3 ship (house 58) sails there by input on some weekdays (`MM6Transport.BoatToMistAndBack`), or a
  teleport.
- Newton topic 51 sets bit 110, topic 52 needs the Hourglass (item 433) and gives award 3.
- Key to Gharik's Laboratory (item 487): Silver Helm Outpost d07 (oute2 event 90), chest event 16 (chest 1, sets bits 11
  and 199), behind a switch and secret door.
- Hourglass: Gharik's Forge d18 (oute3 event 103, on an island no land reaches, so a teleport to its door). The lever
  at d18 event 16 needs the key or bit 376, then the bridge and lever puzzle, then chest event 48 (chest 5, sets bit 183).
- Not tried, so no blocker is known yet. Risks from the stage 2 probe: d18 timer 54 has a damage kind the MM6 guard
  refuses, gated on map byte 105, and indoor MM6 levers, doors and chests have not been driven by a chain chapter yet.

C3 (Stone, Stromgard) has known blockers from the probe: the Prince of Thieves is SpeakNPC sewer:8 (no MM6 street
dialogue window) and the sewer is reached through Free Haven house exits (lane X). The Hermit's hut on outb1 needs Fly
or a teleport fixture. C4 needs the Devil Outpost fight (zdwj02) and the t7 keys for the traitor letter.

## Checks

All one process at a time, with the load gate.

- `check_style`: clean.
- Unit tests: 1,120 passed, 45 skipped.
- MM6 chain through the runner from a fresh profile, `--title mm6 --through 1`, twice: both passed C0 and C1 with pass
  markers (about 2.5 minutes per run). Both profiles removed with `--remove-profile`, and the exploration copies too.
- MM6 suite `MM6*`: 118 passed, 2 skipped (the private-research and scoped-save tests that always skip).
- MM7 controls `MM7*:VendorRefreshIntegration.*`: 107 passed, 2 skipped.
- Revert checks: the letter test failed without the creation fix. The teleport test's scenario threw without the
  controller fix in the exploration run, it was not rerun separately with the fix reverted.
- Capture: `local-game-data/progress-captures/20261111-145443-mm6-chain-c1-outd1.png`.
- Owner profiles: one new file appeared, `Profiles/MM6/logs/openenroth_2026_11_11_19_29_31.log` at 14:29, written by
  a binary at revision `cb179e000` from the main checkout. My runs use this worktree's binary at `71ee852cd` and the
  disposable profiles, so it is another session's run. No other file there changed.

## Notes

- Seen once in exploration, not investigated: after the outb2 chest trap killed the unfixtured party, the death
  respawn to oute3 threw the same chest generation guard. That was with the old teleport path, the party had reached
  outb2 by a teleport that skipped `leaveMap`. Worth a recheck on master with a party death after a real map change.
- Item 505 is `ITEM_ARTIFACT_YORUBA` in the MM7 item enum, so the code uses the raw id with a comment.

Privacy: no game data, saves, captures or logs in Git. The decoder and run logs stay in the session scratchpad.
