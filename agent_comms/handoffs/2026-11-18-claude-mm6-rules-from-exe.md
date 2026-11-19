# Claude MM6 rules-from-exe lane, 2026-11-18

Branch `claude/mm6-rules-from-exe`, off `claude/mm6` at `99eb2e283`. The MM6 engine no longer reads `rules/*.json` from
the profile. It reads its rule tables from the GOG 1.2 `MM6.exe` in the MM6 data folder, and no `--rules-path`,
`OPENENROTH_MM6_RULES_PATH` or `MM6_RULES_PATH` exists any more.

## Commits

- `22a2caf76` Read the MM6 rule tables from MM6.exe. `mm6::Executable` (`src/Engine/MM6/MM6Executable.h`) checks the
  SHA-256 (`b804fda0...e3e6`, a clear error names the file and both hashes otherwise), maps virtual addresses through
  the PE section table and reads bytes, words and C strings. `mm6::executable()` reads `mm6.exe` from the data
  filesystem once. The five table loaders take it, the four code-constant tables became source constants, and
  `gameplay.fix_condition_spells` was added (see below). Holds the acceptance test `MM6ExecutableRules.MatchesProfileJson`.
  SHA-256 moved from `MM6PrivateMapSource.cpp` to `src/Utility/Sha256.h`.
- `ae199ef79` Style fix of `MM6TimedCureRules_ut.cpp`. By mistake it also carries the deletion of
  `src/GUI/UI/Tests/MM6ExecutableRules_ut.cpp`, which was already staged, while its CMake entry goes in the next commit.
  So this one commit alone doesn't configure. The tree is right from `6aec4c6fb` on. Squashing is the integrator's call.
- `6aec4c6fb` Remove the MM6 rules-path plumbing: GameTest `--rules-path`, `MM6HeadlessRules`, the `GameStarter`
  `prepareUserFs` callback, `readMM6TestRules` and `OPENENROTH_MM6_RULES_PATH`, the acceptance test, `MM6_RULES_PATH`
  and `--rules-path` in `tools/run_mm6_firstmap_gate.py`, `run_mm6_map_sweep.py`, `run_mm6_combat_sweep.py`,
  `run_mm7_campaign_chain.py`, and the Atlas rules seeding in `launcher/AtlasApp.swift`. Two `.llm-wiki` pages.

## Where each table comes from now

| File | Now | Source of the old json |
| --- | --- | --- |
| `creation.json` | MM6.exe bytes: HP base 0x4C2630 (6), SP base 0x4C2638 (6), HP factor 0x4C2640 (18), SP factor 0x4C2654 (18), stats 0x4C2668 (6x7), skills 0x4C2694 (6x31) | same addresses, `mm6_f3_pe_tables.py` |
| `spells.json` | MM6.exe 0x4BDD70, 100 rows of 14 bytes: cost x3, recovery x3, flags | same |
| `house-rooms.json` | MM6.exe 0x4BE888, 119 rows of 16 bytes: name pointer, background +4, NPC picture +8, house type +12, sound group +13 | same |
| `transport.json` | MM6.exe routes 0x4C3F20, 36 rows of 32 bytes (map, 7 days, duration +8, x/y/z +12, yaw +24, quest bit +28), slots 0x4C43A0, 21x3 bytes for houses 48..68 | same, plus a slot kind from the 2DEvents.txt cell text |
| `shop-stock.json` | MM6.exe rows 0x4C433C, magic levels 0x4C4546, general stores 0x4C459C, visible slots 0x4C45F1, second bank +540, guild books 0x4C48B0 | untracked `local-game-data/mm6/research-2026-11-07-shops/export_shop_stock_private.py`, same addresses |
| `first-aid.json` | constant 5/7/10, MM6.exe 0x427E25 | `spells.txt` text and the guide |
| `power-cure.json` | constant 10 + 2 x skill, MM6.exe 0x428591 | `spells.txt` text and the guide |
| `timed-cures.json` | constants, MM6.exe 0x426355, 0x426B01, 0x427534, 0x427741, 0x4279FC, 0x427D8C | `spells.txt` text and the guide |
| `combat-buffs.json` | `blessAndHeroismDuration` (3840 + 300/300/900 s per point), Heroism power 5 + skill, MM6.exe 0x42472A and 0x426C21 | `spells.txt` text and the guide |

The "hand-corrected" F3 PE report (`F3_PE_RECONCILIATION_2026-11-05_v2.json`) only corrected how 2DEvents.txt columns
were joined to the transport slots. It never reread or changed an exe value.

## Differences against the owner's json, all checked by the acceptance test

- Creation, spells, house rooms, transport routes and slot routes, shop stock: identical, every value.
- First Aid and Power Cure: identical.
- `combat-buffs.json` `base_seconds` 3600 is a json bug, copied from the printed "1 hour". MM6.exe adds 3840 for
  Shield (0x42472A), Heroism (0x426C21) and Bless. `blessAndHeroismDuration` already used 3840 for the live Bless and
  Heroism casts, and only Shield, which `isVerifiedPlayerSpell` refuses, read the json. No reachable gameplay changed.
- `timed-cures.json` gave all six spells 180/3600/86400 s per point. MM6.exe gives Stone to Flesh (40) and Remove Curse
  (49) those, but Remove Fear (57), Cure Paralysis (60), Cure Insanity (64) and Cure Weakness (67) 180/10800/259200.
  That is GrayFace C10, `FixConditionSpells` in `Hooks.pas:1331`. It is now `gameplay.fix_condition_spells`, on by
  default, through `mm6::TimedCureWindow`. The default game plays exactly as with the json. Only 67 is admitted today.
- Transport slot kinds (blank, scheduled, weekday_uncertain, special_unresolved) described the 2DEvents.txt cell text.
  Nothing read them, `TransportSlotKind` is gone.

## Fixtures and tests

`MM6PlayerSpells.CureWeaknessWindow`: an Expert skill 1 Cure Weakness on a character weak for 2 hours, both flag
settings. The weakness onset is set 2 hours in the past instead of ticking 2400 frames. Reverting the fix (ignoring the
flag in `secondsPerSkill`) fails it on `fix=true` (cured where it shouldn't be), restored after.
New unit tests: `Sha256.KnownDigests`, `MM6Executable.RejectsOtherFiles`, `.ReadsByVirtualAddress`, `.CreationArrays`,
`MM6SpellRules.ExecutableTable`, `MM6ShopStock.ExecutableTables`, `MM6TransportRules.ExecutableTables`,
`MM6RoomDescriptors.ExecutableTableAndRoomSelection`. Synthetic json manifests in the spell unit tests became
synthetic `mm6::SpellRules` tables.

The acceptance test was also checked against a tampered copy of the json (one guild, one route yaw), it failed on both.

## Test results

- Acceptance `MM6ExecutableRules.MatchesProfileJson` at `22a2caf76`, with both env vars: passed. All `MM6*` unit tests
  then: 717/717.
- Unit tests at `6aec4c6fb`, only `OPENENROTH_MM6_PATH` set: 1162/1162 passed, 0 skipped.
- MM6 game tests, `--gtest_filter='MM6*'`, headless, no `--rules-path`: 274 run, 272 passed, 2 skipped
  (`MM6ZSaveHeadless` private research root, as before), 126 disabled.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840`: 3/3.
- `check_style`: clean. `check_tidy` not run, clang-tidy 22 isn't installed.
- `tools/run_mm6_firstmap_gate.py --skip-build`, with only `MM6_DATA_PATH` and `MM7_DATA_PATH`: passed, 38/38 children
  (35 MM6 children over the nine profiles and the 3 MM7 controls).
- Atlas: `swiftc -typecheck` of the three launcher sources passes. Atlas itself was not rebuilt or launched.

## Outside the repo

- `~/enroth-tools/bin/enroth-capture`: passes `--rules-path` only when the worktree's GameTest still lists it in
  `--help`, so lanes built before this branch keep working and builds after it don't pass the flag. Drop the check once
  every worktree has this branch.
- `~/enroth-tools/bin/play-mm6` never passed `--rules-path`. It runs the app on the owner profile, which held `rules/`.
  No edit needed.
- The owner's `Profiles/MM6/rules/` folder is untouched. Nothing reads it after this branch, it can go once every lane
  and the main checkout are past it.

## Open

- GrayFace C12 (Static Charge Expert cost from `spells.txt` instead of the exe table, 2/2/0 against 2/1/0) is still open,
  `mm6::spellManaRecovery` keeps the exe value.
- `MM6FirstAidPreflight`, `MM6FirstAidTransaction` and `MM6PowerCureTransaction` are only used by their unit tests.
- The `project/MM6_*` packets that describe the private rules json as the runtime source were left as history.
