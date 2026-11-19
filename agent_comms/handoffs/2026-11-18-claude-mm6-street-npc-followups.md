# Claude MM6 street NPC follow-ups lane, 2026-11-18

Branch `claude/mm6-street-npc-followups`, off `claude/mm6` at `eca9f3509`. Takes the four leftovers of
`2026-11-18-claude-mm6-street-npc-talk.md`. Static traces of the GOG `MM6.exe` (md5 c6b4fad8f63daec3a8dd494c688a2828)
with `llvm-objdump -d -M intel`.

## Commits

- `d7d5f9d0e` Keep MM6 bribes, tavern drinks and tavern tips in EC6 PART v5. Leftover 2. Test
  `MM6InGameSave.StreetNpcBribesAndTavernTipsSurviveLoad`, unit tests `MM6CoreSaveFields.*` extended.
- `5dd2c818e` Clear MM6 street NPCs on New Game. Leftover 3. Test `MM6NpcDialogue.StreetNpcNewsAfterNewGame`.
- Leftovers 1 and 4 needed no code, see below.

## The four leftovers

1. **Status bar keeps "Beg".** Not a bug. Ours shows "Beg" on the one frame after the click and the new topic's label
   ("Skills") from the next frame on (`MM6NpcDialogue` throwaway run: status tape `Rest, "", Beg, Skills`). MM6 does the
   same: its pointed-object routine 0x41F190 (called from the main loop at 0x453ACE) copies the hovered button's name
   at +81 to the status bar (0x41F403 -> 0x418F40), and the conversation draw copies the topic labels into +81 only
   while it draws (0x43B8AA). The click handler (0x4A3E19 and the others) changes the button params but not +81, so the
   frame of the click shows the old label in MM6 too. The street-npc-talk capture was taken on that frame. No change.
2. **Saved state.** The party's bribe count (MM6 party +220), the tavern drink bits (party +640) and the tip news
   (party bytes) live in MM6's party struct and so in its saves. EC6 `PART` v5 now carries them after the reputation:
   bribes as int32, the 21 drink bits as a uint32 mask, the 21 tips as int32 (186-byte payload, v4 and older reject).
   The per-NPC begged, bribed and threatened states are not saved, on purpose. They live in the street NPC table
   (0x6B74F0, stride 60), which MM6 never saves: only 0x41CEA9, 0x43BE3D and 0x4567D0 address it, and the map load at
   0x4567CB regenerates every street NPC through 0x469210, which zeroes the state (+8). OE regenerates them on every
   map load as well (`Engine.cpp`), and the new save test shows the bribed NPC asking for a bribe again after the load.
   Before this change, an in-game load also kept the live session's bribes, drinks and tips, because `stageCoreParty`
   starts from the live party.
3. **Street NPC state after a New Game.** The news topic (+56) of the street NPC table. MM6's generator 0x469210
   writes +0 to +32 and +48 but never +52 (ability used) or +56 (news), and its New Game (0x486323) copies only the
   400-record npcdata table (0x6A9168 -> 0x6AEF28). So a slot's news topic survives regeneration, and the next villager
   in that slot skips the pick at 0x43BC20 and tells the old news, from whatever map it was picked on. Ours did the
   same. Now an MM6 New Game clears `pAdditionalNPC` (`UIPartyCreation.cpp`). The carry-over across map loads within
   one game is vanilla and stays (a Castle Ironfist villager can tell New Sorpigal's news after a trip). GrayFace
   2.5.7 has no hook for it. Owner's call whether to fix that too.
4. **Talker with nobody active.** No change. MM6 never opens a street NPC conversation with nobody active: the talk
   message handler returns at 0x42DADA when CurrentMember (0x4D50E8) is 0. Ours refuses too (`Interaction.cpp`
   `DoInteractionWithTopmostZObject`, "Nobody is in a condition to do anything!". Whether MM6 shows a status there was
   not traced). Inside the conversation MM6 uses CurrentMember as an index into its member table at 0x944C64, whose
   slot 0 holds the first member (0x485FB8), so its diplomacy and bribe price would fall back to the first member, while
   its text builder gets member CurrentMember - 1 = 255 (0x43ADC4) and reads past the party. Our fallback to the first
   member can't be reached, so nothing was changed and no test was written.

## Tests

- `MM6InGameSave.StreetNpcBribesAndTavernTipsSurviveLoad`: party from the creation screen, a drink and a tip at A
  Lonely Knight, a 50 gold bribe to Janine, save from the Escape menu, back to the main menu, a New Game, Load from the
  Escape menu. The barkeep tells the same news without a drink, and Janine (fresh again) asks for 100 gold.
- `MM6NpcDialogue.StreetNpcNewsAfterNewGame`: talk to Cora south of Castle Ironfist (slot 15 on outd3), a New Game,
  talk to Philomena (slot 15 on oute3). Her news is "Abandoned Temple".
- Revert and fail: with both fixes stashed the news test shows "The Seer" (Cora's Castle Ironfist news), and the save
  test shows "Have a Drink first...", no second tip and no gold for it.
- Full `MM6*` game tests: 277 passed, 2 skipped (`MM6ZSaveHeadless`, private research env).
- `OpenEnroth_UnitTest`: 1125 passed, 43 skipped.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840`: 3/3 passed. The only shared file is `UIPartyCreation.cpp`,
  and its new line runs for MM6 only.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/street-npc-followups/before.png` is the frame right after clicking Beg
on Marianne, status bar "Beg". `after.png` is the next frame on the final code, status bar "Skills", the label of the
topic under the cursor. The frame after the click on the final code is byte-identical to `before.png`, since leftover 1
needed no change. The temporary capture test is removed.

## Found, not fixed

- MM6 greets with speech 22 or 23 by the party's reputation (0x43C084..0x43C09D: 23 when it's negative), ours plays
  good day or good evening by the hour (`initializeNPCDialogue`). Not checked further.
- Street NPC news topics carry over across map loads within a game, as in vanilla (leftover 3).
