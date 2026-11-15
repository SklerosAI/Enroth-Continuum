# MM6 event spells, second batch, handoff

- Owner: Claude Code lane `mm6-event-spells`, worktree `enroth-worktrees/mm6-indoor-light`, branch `claude/mm6-event-spells` off `7024a995c`.
- Commits: `3a55cf8e6` (d08 event projectiles), `dc5b091f7` (event Wizard Eye, Bless, Day of the Gods and Mass Curse), and this handoff.
- Private trace: `local-game-data/private-wiki/mm6/EVENT_SPELLS_PE_TRACE_2026-11-12.md` (addresses, tables, script scan).

## Scope found in the scripts

All 67 active map scripts and `global.evt` were walked record by record. `global.evt` casts nothing. Besides the five IDs the status file named, d08 also casts Flame Arrow (2), Static Charge (13), Cold Beam (24) and Rock Blast (41), which the sweep never reached because it stopped at the first refusal. t1 event 26 casts Mass Curse, but no face, decoration, timer or ChangeEvent names that event, so nothing reaches it. None of d01, d04, d08, d13, zddb08, znwc and t1 is on the 26-map main-quest list.

| MM6 ID | Name | Maps and events | Event effect | MM6.exe |
|---|---|---|---|---|
| 2 | Flame Arrow | d08 plates 22, 23, timer 43 | flies, sounds and hurts as Cold Beam | 0x43ecc4 |
| 12 | Wizard Eye | d01 event 64 (dais, clickable) | nothing, no sound | 0x43f9b1 |
| 13 | Static Charge | d08 plates 22, 23, timer 44 | projectile 2010, 2-6 Elec, hits like a bolt | 0x43ecd8, 0x432bc2, 0x45d91b |
| 24 | Cold Beam | d08 timer 45 | projectile 3010, 2d3 Cold | 0x43ecd8, 0x432c32 |
| 35 | Magic Arrow | d08 plates 34, 35, timer 46 | flies, sounds and hurts as Cold Beam | 0x43ecc4 |
| 41 | Rock Blast | d08 plate 36 | projectile 4070, 512 area, level d8 Phys | 0x43ecd8, 0x432d4b, 0x45cc3f |
| 46 | Bless | zddb08 click 17 | Bless on every character, power 0, 240 + 300L s Normal, 240 + 900L s Expert and Master | 0x43f52c |
| 83 | Day of the Gods | d04 click 50, znwc click 73 | all seven stat buffs on every character, power 2L, 3L or 4L by mastery, L hours | 0x43f78b, getters via 0x483800 |
| 91 | Mass Curse | d13 click 22, t1 event 26 (unreachable) | Curse through the bad-thing routine on every character, only within 4096 (0x445420 estimate) of the launch point | 0x43f924, 0x480010 |

Event damage levels are masked to 6 bits (0x432ad0). Bless and Day of the Gods levels are not masked.

## Tests

- New `MM6CombatHeadless` game tests, each run alone in a fresh process: `D08ArrowAndStaticChargePlateHitsParty`, `D08MagicArrowPlateHitsParty` (plate 35, the beam of plate 34 is stopped by three monsters in its path), `D08MagicArrowTimerHitsParty`, `D08RockBlastPlateHitsWholeParty`, `D01DaisWizardEyeDoesNothing`, `D04FaceCastsDayOfTheGods`, `ZnwcDeskCastsDayOfTheGods`, `Zddb08MuralBlessesParty`, `D13MonolithCursesParty`. All 26 `MM6CombatHeadless` tests pass alone.
- Revert and fail: with the source reverted every new test ends with the guard's exception. Reverting only the arrow swap or the 2010 impact fails the d08 plate tests. Giving the event Bless the player's power fails the mural test.
- Unit tests `MM6EventSpell` and `MM6SpellEffectGuard` updated.
- Map sweep on d01, d04, d08, d13, zddb08, znwc and t1: all finish every phase with actors, no event throws. d01 and d04 need `--max-frames 40000` to reach their date timers (15 and 27 game hours).
- Unit tests 1,122 passed, 45 skipped. Full MM6 suite 179 passed, 2 skipped. MM7 controls 147 passed, 2 skipped (the shared `Spells.cpp` call and `SpriteObject.cpp` impact changed, both MM6-gated). `check_style` clean. `check_tidy` not run, no clang-tidy 22 locally.
- Capture `20261112-190930-mm6-event-spells.png` shows the first map. The food and gold numbers next to the apple and the coins are not drawn. That is outside this lane.

## Open

- The MM6 portrait overlays for Bless and Day of the Gods (5010, 10000, 8050) are not drawn, as for the other MM6 buffs.
- The game tests don't pin the Bless and Day of the Gods durations, only that the buffs land with their power.
- The event buffs use the shared Bless and stat buffs, which the HUD and stat getters read. The older MM6 monster code (Dispel Magic, the Shield check) indexes raw MM6 buff slots instead, so a monster Dispel Magic doesn't clear these stat buffs. The two conventions need one owner.
- Spirit Arrow (45) shares the arrow case but no shipped event casts it, so it stays refused.

Privacy: IDs, addresses and map stems only.
