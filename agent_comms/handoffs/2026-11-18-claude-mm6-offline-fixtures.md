# Claude MM6 offline-fixtures lane, 2026-11-18

Branch `claude/mm6-offline-fixtures`, off `claude/mm6` at `6ef7b362c`. Owner request: port the little Python the MM6
work still needs into C++ in `src` and `test`, and clear out the Python research scripts. No gameplay change, so no
captures.

## Commits

- `dc1c4556f` Read the MM6 install directly in the data-backed unit tests. New `MM6TestData` helper in
  `test/Testing/Unit` (part of `testing_unit`). `MM6TestData::open()` opens the folder named by `OPENENROTH_MM6_PATH`
  through the lowercase file system and exposes `games.lod`, `icons.lod`, decompressed `icons.lod` entries, the sorted
  map names and the 68 active event scripts (one per map plus `global.evt`). `readMM6TestRules()` reads a profile
  rules file from `OPENENROTH_MM6_RULES_PATH`. `ItemTable::loadMm6Tables()` is split out of `InitializeMm6` so a unit
  test can run the production MM6 item parser. Two research-only tests dropped.
- `3a3ed99db` Remove the 78 `scripts/mm6_*.py` and `scripts/test_mm6_*.py` files. One line in
  `.llm-wiki/MM6-Source-and-Probe-Index.md` records the removal and the two environment variables.

## The 45 former skips

Before: 1170 unit tests, 1125 passed, 45 skipped. With the legacy research fixtures supplied by hand, all 45 passed.

Ported, now reading the install or the profile (43):

- Games.lod maps, decoded with `decodeMM6IndoorBaseArchiveEntry` and `decodeMM6ArchiveEntry`:
  `MM6IndoorSnapshot.ShippedFiftyTwoPairs`, `MM6OutdoorDeltaSnapshot.ShippedFifteenDeltas`,
  `MM6ChestLiveOwnerAdapter.ShippedMapsMatchCurrentRuntimeChestShape`,
  `MM6ChestMapLoadHandoff.ShippedBaselinesStageWithSyntheticTable`, `MM6MapSaveFields.ShippedSixtySevenDeltas`,
  `MM6MapSaveValidation.ShippedSixtySevenBaseDeltaPairs`, `MM6Archive.ShippedDeltasRoundTrip`.
- icons.lod tables: `MM6AwardTable.ShippedHundredRowsLoadWithTextAndNoPriority`,
  `MM6HouseType.ShippedTableHasNoUnknownLabels`, `MM6HouseService.ShippedTableFamilyCounts`,
  `MM6SpellDamageTag.RetainsAllShippedSpellSourceTags`, `MM6MonsterDamage.ReadsSixShippedMonsterResistanceColumns`,
  `MM6MonsterSpecialAttack.CoversAllShippedActiveMonsterRows`, `MM6MonsterSpells.ReadsAllShippedActiveMonsterCasts`,
  `MM6MonsterProjectile.ReadsAllShippedRangedAttackCells`,
  `MM6MonsterProjectile.NineLabelsResolveToShippedObjectAndSpriteRows`, `MM6PotionCombinations.ShippedMatrixAggregate`,
  `MM6ChestCallerAdapter.StagesShippedItemTables` (was a numeric profile from a private script, now the production
  parser), `MM6EventFraming.AllActiveShippedScripts` (68 scripts through `EvtProgram`).
- Anims VIDs: `MM6MovieLookup.ResolvesEveryShippedMovieByItsExtensionlessName` (now the real 40 + 87 entries rather
  than synthetic VIDs built from a name list).
- Profile rules: `MM6SpellRules.ProfileManifest`, `MM6FirstAidRules.ProfileManifest`,
  `MM6FirstAidPreflight.ProfileRuleComposition`, `MM6FirstAidTransaction.ProfileRulesPlanAndStageActualCharacterState`,
  `MM6PowerCureRules.ProfileManifest`, `MM6PowerCureTransaction.ProfileRulesStageActualCharacterState`,
  `MM6TimedCureRules.ProfileManifest`, `MM6CombatBuffRules.ProfileManifest`, `MM6TransportRules.ProfileManifestShape`,
  `MM6TransportRules.RejectsAlteredProfileManifest`, `MM6RoomDescriptors.RejectsMalformedManifestBeforeUse`.
- Install and rules together, all twelve `MM6HouseServiceData` data tests. `house-route-probe.json` gave way to a scan
  of the `SpeakInHouse` instructions in the active scripts (671 table-backed, 1 blank, 2 terminal, same counts as the
  probe). `F3_MEDIA_RECONCILIATION` gave way to the rooms the house table references and the `PlaySound`
  instructions of the active scripts. `DecodesOneActive...` became `DecodesEveryActiveMM6EventSoundWithoutPlayback`
  and decodes every sound the 8 instructions play. The room-frame test no longer writes a private TSV into the research
  folder, it reports failures through gtest.

Dropped (2):

- `MM6RoomDescriptors.RetainedRowsAndActiveHouseEntries`. Its unique part compared the profile room manifest with the
  F3 PE report, which tests the research export, not code. Its house and room checks are made by
  `MM6HouseServiceData.PreflightsEveryActiveHouseInstruction` and `PreservesShippedRowsWithoutMM7ServiceRules`.
- `MM6PrivateMapSource.F3IdentityContainmentAndStaging`. It guards `mm6::PrivateMapSource`, the research-directory map
  reader, which production never uses (production uses `LoadedMapSource`).

The runner's pre-test Python checks (`mm6_f4_check_*`, `tools/mm6_map_name_manifest.py`) re-checked research claims
against the F3 reports and had no C++ behind them. They are not ported.

## Test results

- `OpenEnroth_UnitTest` with `OPENENROTH_MM6_PATH` and `OPENENROTH_MM6_RULES_PATH`: 1168 tests, 1168 passed, 0 skipped.
- Without them: 1168 tests, 1125 passed, 43 skipped cleanly (22 need the install, 11 the rules, 10 both).
- MM6 game tests `MM6Chest*.*:MM6CreationItemSeed.*:MM6NewGame.*:MM6Houses.*`: 39/39 passed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840`: 3/3 passed (`ItemTable.h` is shared, MM7 code paths unchanged).
- `check_style` clean. `check_tidy` not run, clang-tidy 22 is not installed locally.

## Open points

- The profile `rules/*.json` files can no longer be regenerated from the repo. They came from the deleted exporters
  applied to a private, hand-corrected F3 PE report that no tracked script produced, so they were not reproducible
  before either. If a fresh machine needs them, the next step is a C++ extractor that reads the tables from `MM6.exe`.
- `mm6::PrivateMapSource`, `saveNewGameMM6ScopedForTest` and the `GameTests_MM6Save` cases gated on
  `ENROTH_MM6_MAP_RESEARCH_ROOT` still depend on the research folder and `tools/mm6_map_name_manifest.py`. They were
  out of scope here.
- `scripts/__pycache__` exists only in the main checkout (git-ignored). It was left alone and can be deleted after the
  merge.
