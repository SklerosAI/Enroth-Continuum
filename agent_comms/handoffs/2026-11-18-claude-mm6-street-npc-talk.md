# Claude MM6 street NPC talk lane, 2026-11-18

Branch `claude/mm6-street-npcs`, rebased onto master `23f6eb0c9`. Picks up the unfinished street NPC work that
`agent_comms/messages/2026-11-16T19-54-52Z--from-codex--to-claude--ecv0.1.0-and-mm6-integration.md` parked.

## Commits

- `cb05e58f7` WIP: MM6 street NPC talk, unfinished. The earlier lane's checkpoint, kept as it was for the integrator.
  Weekday topic, Join and news topics, npcBTB.txt greetings, the conversation mode from fame and reputation.
- `9c4397080` fix(mm6): fill %02 and %08 in NPC text the way MM6 does. %02 no longer eats the two characters after the
  name ("Zoltan.  I don't" was "Zoltan I don't"). %08 no longer asserts in MM6: one of the talker's 28 addressing
  awards (0x4C29A8), picked once per conversation (reset at 0x43BE5F), or npctext.txt 56 "performed daring deeds".
- `f83e95ddf` feat(mm6): beg, threaten and bribe street NPCs. Rules below. Also removes `ZzProbe`, reconciles the
  `npc260` expectation and moves the Join sub-screen into the same window.

## The rules, traced in MM6.exe (GOG build, md5 c6b4fad8f63daec3a8dd494c688a2828)

- Mode at conversation start (0x43BF39): hired or house < 0 shows topics. Fame <= NPC fame shows the fame refusal.
  State (flags & 0x7F) 0 or 1: NPC reputation 0, or party and NPC on the same side with the party further out, shows
  topics, else Beg, Threat and Bribe. State 2 (begged) and 3 (bribed) show Beg, Threat and Bribe, 4 (threatened)
  shows topics. Modes 2 and 3 create the same three buttons (0x4196C6).
- msg 136 (0x4A3B10): displayed topic := param, flags 0 -> 1, switch by param (table 0x4A4298, byte map 0x4A42C8).
  22 Beg 0x4A3E19, 23 Threat 0x4A3F16, 24 Bribe 0x4A4077.
- Acceptance: npcBTB.txt rows Beg, Bribe, Threat are loaded (0x468172) to +26, +0, +13 of 0x6B9880, by personality
  (column remap 0x4C1034). Personality is profession record +0 (table 0x6B5DD0, stride 76).
- Beg: state 2 already -> displayed topic 0, nothing else. Accepts -> state 2, reputation -= max(0, 10 - D), topics
  22/23/24 -> 12/13/14, speech 53. Refuses -> speech 54.
- Threat: accepts -> state 4, reputation -= max(0, 50 - D), relabel, speech 55. Refuses -> speech 56.
- Bribe: accepts -> price = max(10, (100 - D) * (bribes + 1) * 50 / 100) (bribes is party +220, 0x908C4C). Gold >=
  price -> take it (0x487680), state 3, bribes + 1, reputation -= max(0, 20 - D), relabel, speech 57. Else
  displayed topic 0, speech 38, "You don't have enough gold" (global 155). Refuses -> speech 58.
- D (0x4852D0): (Diplomacy level + 4 hired Counselor 23 + 8 hired Barrister 24 + 4 hired Negotiator 49) times 4 for
  master, 3 for expert, else 2. Reputation is not clamped here.
- Answer text (draw 0x43ADAE, 0x43ADFE, 0x43AE4E): npcBTB.txt 19/20 beg, 23/24 threat, 21/22 bribe, accepted or
  refused by the same acceptance bit. Labels (0x43B745..): global 27 "Beg", 226 "Threat", "%s %lu %s" with 31
  "Bribe", the price and 97 "Gold".
- %04 in dialogue text is the bribe price (0x489E7A), used by row 4, the bribed NPC's greeting.
- Join (0x4993C0) recreates the window with More Information (only with profession benefits) and Hire.
- New day (0x4881E4) clears only the used-ability fields, street NPC states persist.

Data scanned: MM6 `npcBTB.txt` (all 27 rows, all 13 personality columns), `npcprof.txt` personalities, `global.txt`
ids 27, 31, 97, 155, 226, `npctext.txt` 56, `proftext.txt` rows 13, 23, 49. GrayFace 2.5.7 has no hook touching
Beg, Threat or Bribe (searched the patch sources and decoded tables). Only MM6 was in scope.

## Fixtures

Default party through `enterNewSorpigal`. Marianne the Teacher (scholar, reputation 200: begs and threats work,
bribes don't), Ginger the Negotiator (official, reputation -300: only bribes work) and Maria the Scholar (reputation
0, talks) are reached by teleport and Space. `StreetNpcBribeDiplomacy` sets Zoltan's Diplomacy to expert 2. The
news topic label changes with test order, so the tests tape topic ids instead of that label.

## Test results

- MM6NpcDialogue 15/15 and MM6Hirelings 2/2 after the last change.
- Full `MM6*` run before the MM6Hirelings fix: 265 passed, 2 skipped (private research env), 1 failed
  (`MM6Hirelings.HireTwoAndRefuseAThird` clicked Hire on the first screen, now all three hires go through Join and the
  test passes).
- `OpenEnroth_UnitTest`: 1125 passed, 45 skipped.
- MM7 (shared `UIDialogue.cpp`, `NPCTopics.cpp`, `GUIWindow.cpp`, `GUIDialogues.h`, `Party`): Prs.Pr2354,
  Prs.Pr2956, Issues.Issue840, 202, 248, 1115, 1273, 1462, 1718, 2464a, 2931, 11/11 passed.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed.
- Revert and fail: without the action branch every Beg/Threat/Bribe test fails. Without the begged-twice check
  `StreetNpcBegAgain` fails, without the acceptance check the three refusal tests fail, without the bribe count
  `StreetNpcBribeAgain` fails, with diplomacy 0 `StreetNpcBribeDiplomacy` fails. Old %02 fails
  `StreetNpcGreetingName`, old %08 aborts `StreetNpcGreetingDeeds`, the old Join path aborts `StreetNpcPanel` and
  `StreetNpcHire`.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/street-npc-talk/before.png` and `after.png`, Marianne's conversation
before and after clicking her first topic. The temporary capture test is removed.

## Open

- The bribe count isn't in the MM6 save (PART has no field for it), same as the tavern state.
- The status bar keeps showing "Beg" after the click, while the button under the mouse is now the weekday topic.
- A street NPC's news label differs depending on which tests ran before in the same process, so some NPC state
  survives a new game. Not traced.
- With no active character the talker falls back to the first character. MM6 reads index -1 there, not traced.
- Speech 53..58 show as portrait reactions in OE, MM6 voices don't load.
- The `9c4397080` state was committed without its own build, the next commit was built and tested.
