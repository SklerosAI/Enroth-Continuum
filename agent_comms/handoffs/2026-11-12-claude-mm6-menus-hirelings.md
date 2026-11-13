# MM6 menus and hirelings handoff

- Owner: Claude Code lane MENUS-HIRELINGS, reviewed by the Claude lead.
- Branch: `claude/mm6-menus-hirelings` in the `mm6-indoor-light` worktree, from `master` `6361f74a5`. Not pushed.
- Private trace with addresses: `local-game-data/private-wiki/mm6/MENUS_PE_TRACE_2026-11-12.md` (not in Git).

## Commits

1. `fix(mm6): draw the title screen buttons and play its music as MM6 does`. `MainMenuResources.h`, `UIMainMenu.*`,
   `MainMenuState.cpp`, a `CALL_PLAY_MUSIC` engine call with `tapes.music()`. Test `MM6MainMenu.ButtonArtAndMusic`.
2. `fix(mm6): lay out the escape menu and its Controls screen as MM6 does`. `UIGame.*`, `GameMenu.cpp`, new
   `MM6GameMenuLayout.h`. Tests `MM6GameMenu.ButtonsMatchArt`, `MM6GameMenu.Controls`. Capture test
   `MM6OffscreenFrame.DISABLED_MenusCapture`, whitelisted in `GameTestMain.cpp`.
3. `fix(mm6): lay out the Save and Load screens as MM6 does`. `UISaveLoad.*`, `GameMenu.cpp`. Test
   `MM6GameMenu.SaveAndLoadScreens`.
4. `fix(mm6): give generated street NPCs MM6's portraits`. `NPCTable.cpp`, new `MM6StreetNpcPortraits.h`. Tests
   `MM6Hirelings.StreetNpcPortraitExists`, `MM6Hirelings.HireTwoAndRefuseAThird`. `MM6NpcDialogue.StreetNpcPanel`
   now expects Marianne's MM6 portrait npc260 instead of npc382.
5. This handoff, the portrait header wording and the trace log.

Shared-core files changed: `EngineCallObserver.h`, `AudioPlayer.cpp`, `UIMainMenu.*`, `MainMenuState.cpp`,
`UIGame.*`, `GameMenu.cpp`, `UISaveLoad.*`, `NPCTable.cpp`. Every behaviour change is behind `GameTitle::MM6` or a
resources table that keeps MM7's values. The only change MM7 sees is that the music request is reported to the call
observer before the same-track early return.

## What MM6.exe does

- Title screen 0x4506E0: New, Load, Credits, Exit are 132x44 at x 482, y 9 / 71 / 133 / 195, hotkeys N, L, C and none
  on Exit, pressed art MMNEW1 / MMLOA1 / MMCRE1 / MMESC1. The hovered button animates start00x..start06x and back,
  first step 500 ms after the menu opens, then at most one step per 50 ms. Music is CD track 13 from 18.5 s
  (AIL_redbook_play at 0x4A6C3D and 0x45378B). OpenEnroth now plays track 13 from its start.
- Escape menu 0x42BF29: Resume (18,160), New (18,213), Save (18,266) 219x39, Controls (242,160), Load (242,213),
  Quit (242,266) 215x39. OpenEnroth had MM7's grid, so Resume Game started a new game and Load Game quit.
- Controls 0x42D743, drawn by 0x40F550: ControlBG with turn rate, Walksound, Show Damage, three volume sliders with
  16 px notches from x 278 and Resume Game at (261,292). MM6 has no key binding and no video screen.
- Escape or Resume from the escape menu, Controls, Save and Load go straight back to the game (0x42B6BB, 0x42BDFD).
- Save 0x44EB94 and Load 0x44E2D0 share one layout: check mark (276,306) and cross (376,306) 61x28 with check_p and
  x_d, arrows (216,194) and (216,323) 16x16 with sarr1_pr and sarr2_pr, no scroll bar, header at (18,141).
  OpenEnroth drew MM7's LS_saveU, LS_loadU and x_u, which MM6 lacks, as two missing-image placeholders.
- Hiring (0x4A3B69), the `%17` gold share (price / 100, at least 1, 0x48A13A) and the share of found gold (0x41EF1E)
  match OpenEnroth, no change made.
- Street NPC generator 0x469210 picks portraits from a 120-id male table and a 66-id female table with one random
  call. OpenEnroth used MM7's elf ranges, which MM6's icons.lod doesn't have.

## Tests

- Unit tests: 1123 passed.
- Full MM6 suite: 218 ran, 216 passed, 2 skipped (`MM6ZSaveHeadless.LoadedMapSourceMatchesPrivateResearch` and `ScopedRealFirstMapSaveAndRollback`, they need the private F3 map source), on the final tree.
- MM7 controls (`MM7*:VendorRefreshIntegration.*:Prs.Pr2354:Prs.Pr2956:Issues.Issue840`): 149 ran, 147 passed, 2 skipped (`VendorRefreshIntegration.*RefreshesOnEntryButNotBuyTab`), on the final tree.
- `check_style` clean. `check_tidy` not run, there is no clang-tidy 22 on this machine.
- Each fix was reverted with its test kept, rebuilt, and its tests failed: `MM6MainMenu.ButtonArtAndMusic`,
  `MM6GameMenu.ButtonsMatchArt`, `MM6GameMenu.Controls`, `MM6GameMenu.SaveAndLoadScreens`,
  `MM6Hirelings.StreetNpcPortraitExists` (drew npc487, which icons.lod lacks).
- The campaign chain was not run.

## Captures

In `local-game-data/progress-captures/`, `*-mm6-menus-hirelings-*`, each inspected whole:

- `title`: title.pcx with the four buttons, New Game hovered. No cursor in the offscreen frame.
- `escape`: the options panel over the 3D view with MM6's six buttons, the HUD, food 7 and gold 200, portraits and
  bars as usual.
- `controls`: ControlBG under the options panel, High and Smooth marked, Walksound and Show Damage crossed, the three
  volume marks on the second notch.
- `save`, `load`: loadsave with the Save Game or Load Game header, the check mark and cross from MM6's art, no
  placeholders, `[New Save]` in the first row of the save list, an empty load list in the test profile.
- `npc-talk`: Marianne the Teacher with portrait npc260 on evpan019, More Information and Hire, her join text.
- `npc-hired`: Marianne in the first hireling frame at (491,149), gold 9700.

## Open

- House residents can't be hired. Naomi Highland (healer, house 475) and Orville Corvair (scholar, house 476) in New
  Sorpigal are joinable in npcdata.txt, but the MM6 house window shows only their name, `mm6SelectHouseTopic` in
  `src/GUI/UI/Houses/MM6House.cpp` handles scripted lines only. That file belongs to the houses lane. Door faces:
  event 70 at (12648,18464) facing west, event 71 at (10840,18776) facing east.
- Street NPC first screen: MM6 offers the weekday topic (proftext.txt), Join and the news topic (npcnews.txt) before
  More Information and Hire (`NPC_DIALOGUE_PE_TRACE_2026-11-12.md`). OpenEnroth jumps to More Information and Hire, so
  there is no rumour or news topic on the street. Not started. Item 4 of this lane (tavern rumour, street news) is
  open.
- Hireling abilities (healer, cartographer) were not exercised, MM6 house hiring is the way to a healer in town.
- The title music starts at the top of track 13, MM6 starts 18.5 s in. The audio tracks have no seek.
- The escape menu draws no pressed art in either title, MM6 draws resume1 etc. at the button.
- Graphics Detail on Controls has no OpenEnroth setting, High stays marked and the three buttons are not created.
- The Save and Load screens' slot text, thumbnail (283,160) and date positions were not traced, OpenEnroth's stay.
  MM6 saves in OpenEnroth have no thumbnail.
- The voice volume feedback sound is OpenEnroth's 5788, MM6 plays 5044.
- The sex table MM6 uses for generated NPCs (0x4BF8B4) was not compared with `sexForMonsterType`.
- Credits screen and its music were not traced.
