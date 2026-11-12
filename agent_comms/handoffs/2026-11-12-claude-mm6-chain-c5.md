# MM6 campaign chain C5 handoff

- Owner: Claude Code lane C5, for the Claude lead. Branch `claude/mm6-chain-c5` in the `mm6-chain-c4` worktree, from
  `master` `7024a995c`. Not pushed.

## Commits

1. `ec2315c7e` Add campaign chain chapter C5, the Oracle and Memory Crystal Alpha. Test only: the chapter, the CMake
   line and a new helper `useMM6CampaignChainDecoration` (stands next to a decoration on an indoor map and clicks it).
2. This handoff.

No engine change was needed, so there is no fix to revert.

## C5 route (passing)

`GameTests_MM6CampaignChainC5.cpp`, from C4's `save004.ec6` on outc2 to `save005.ec6` on outc2.

- High Council (outc2 event 49, house 165). Its exit "Enter Oracle of Enroth" shows with bit 167 and is taken by
  input (2DEvents row 165, exit to mapstats row 49, restriction 167).
- oracle.blv: the Oracle face (event 3, house 170) sits behind door 2. The floor plate at event 2 slides the door away,
  the party walks over it with the forward key. The house opens straight into the Oracle (NPC 8).
- Topic "Crystal Locations" (global 73) shows npctext 88 and sets bits 162 to 165 (only if 162 is clear).
- Exit (oracle event 50, Yes prompt) to outc2.
- outd3 ship (house 59, decoration 128 on the dock): route 21 "21 days to Hermit's Isle" is offered every day with
  bit 168, so it works from the chain's save. Taken by input, arrives on outa3 at (-18486,-20342).
- outa3 event 90 (Yes prompt, house 196) into t6.
- t6 event 32 on decoration 132: sets bits 53 and 191 and puts Memory Crystal Alpha (item 550) in hand. No key,
  switch, bit or fight is needed. Clicked from next to it, then stashed in the first backpack.
- Way back: the temple's cylinder. Event 30 lowers its wall (door 21, about 5 s), event 31 is a floor plate that sets
  bit 176 and asks to go to the Free Haven sewer. The sewer ladder (event 20) leads to outc2.
- High Council again, Oracle again. The power panel (oracle event 14, face 461 by the start) is clicked, then module
  event 16 takes the crystal: bit 100 and the module's own bit 282 set, 191 cleared, item 550 gone, +100,000
  experience per character.
- Exit to outc2, save slot 5.

Exit tapes: maps outc2, oracle, outc2, outd3, outa3, t6, sewer, outc2, oracle, outc2. Bits (of 53, 100, 162-165, 167,
168, 176, 191, 282) end at {53, 100, 162, 163, 164, 165, 167, 168, 176, 282}. Awards 2-7, 32, 58 unchanged. Item 550
false, true, false. Oracle topics stay 73 and 74. Total experience +400,000. Reputation 1600 unchanged. No character
at 0 HP. The party took no damage in t6.

Fixtures: none beyond C1's party strength. Teleports: outc2 to the outd3 dock, to the outa3 temple door and to the
High Council door. Inside oracle to the plate approach, the Oracle face, module 16 and the exit. Inside t6 next to the
crystal and in front of the cylinder. In the sewer to the ladder.

## What the hand-in and the Oracle's later topics need (scripts read, not played)

- The hand-in is not a topic. Any of four module altars takes any crystal, with the power panel on: oracle events 5,
  15, 16, 17 (bits 280, 281, 282, 283 mark each module full). Beta 551, Delta 552 and Epsilon 553 set bits 101, 102,
  103 and clear 192, 193, 194. Bit 162 stays set after Alpha's hand-in.
- Only when bits 100-103 are all set does a module switch the Oracle's topics to 76 "Control Center" and nothing. So C6
  must bring all three castle crystals before the Oracle says anything new.
- Global 76, first time (bit 166 clear): award 33, set 166 (Control Cube quest), clear 162-165, Melian's message.
  Second time with the Control Cube (item 456): +500,000 exp, award 34, clear 166 and 195, take the cube, topics become
  77 "Kreegan" and 78 "Ancients", and the "access to the Control Center" message. C7 needs this.
- Global 77 "Kreegan": unless bit 305 or 96 is set, it sets 235 (Archibald), gives Nicolai (NPC 13) "The Third Eye"
  (topic 26) and sets bit 169. Bit 169 is what house 170's exit to sci-fi (mapstats row 50) needs. So the Control
  Center entry comes from the Kreegan topic, not from the cube hand-in. Bit 96 comes from Nicolai's Third Eye topic,
  which Kreegan unlocks, and bit 305 from the outd3 well (event 231) after it, so only the first ask sets anything. No
  other decoded script (all 83) sets bit 169. C7 asks Kreegan right after the cube, C8 then has bit 235 and topic 26.
- Global 78 "Ancients": text only.

## Checks

- MM6 chain from a fresh profile, `--title mm6 --through 5`: C0 to C5 passed with pass markers. Profile kept for C6
  at `/var/folders/qc/fg92hjk146s3g36x68b8n1lh0000gr/T/mm6-restart-chain-w2g2innk`.
- Unit tests: 1122 passed, 45 skipped.
- MM6 suite `MM6*`: 179 passed, 2 skipped (the private-research and scoped-save tests that always skip).
- `check_style`: clean. `check_tidy` not run, no clang-tidy 22 locally.
- MM7 controls: not run, no shared code changed.
- Capture: `local-game-data/progress-captures/20261112-185119-mm6-chain-c5.png`, the default first-map mode. The whole
  frame reads right (view, portraits, HP and SP bars, minimap, compass, buttons) except that the food and gold boxes
  are empty. The 12:50 `mm6-ground-grass` capture from the same mode has the same empty boxes, while the `--mm6-map`
  captures of other lanes (for example 18:09 `mm6-claude-mm6-spells-d04`) show 7 and 200. The first-map mode builds
  the frame from the party creation screen without starting a game, so this looks like the capture harness, and this
  branch changes no engine or HUD code. Not investigated further.

Script notes with the decoded events: private `local-game-data/private-wiki/mm6/ORACLE_CRYSTAL_SCRIPT_TRACE_2026-11-12.md`.
No MM6.exe tracing was needed. Privacy: no game data, saves, captures or logs in Git.
