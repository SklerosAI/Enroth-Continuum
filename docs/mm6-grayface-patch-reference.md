# MM6 GrayFace patch reference

The GrayFace MM6 Patch 2.5.7 as a reference for MM6 work: where the pinned copy lives, what the patch changes, how each
of its engine fixes maps onto our MM6 runtime, the data fixes we could overlay, and how a lane uses it. MM7 is Codex's
title and has its own reconciliation, see [MM7 status](mm7-status.md).

## Pinned copies

Nothing from the patch is redistributed through this repository. The only copies live under the git-ignored
`local-game-data/public-references/`.

- **Patch source.** Tag `MM6Patch-2.5.7`, commit `5d95fa3a74a49894527fa35442d59100327bd586`, 2022-03-04, a shallow
  clone of [GrayFace/Misc](https://github.com/GrayFace/Misc) in `GrayFace-Misc-MM6Patch-2.5.7/`. Upstream `master` has
  no MM6 patch commit after this tag, so 2.5.7 is current. The MM7 lane's `GrayFace-Misc-MM7Patch-2.5.7` checkout is
  the same tree.
- **Hook table.** `MMPatches/MM6Patch/Hooks.pas` lines 2913 to 3169, `MMPatches/MMCommon/MMHooks.pas` `HooksCommon` at
  line 4018 and the `{$IFDEF MM6}` `Hooks` array at line 4180. Each row is an executable address plus a one-line
  comment naming the fix. The comments are the authoritative fix list.
- **Option defaults.** `MMPatches/MMCommon/MMCommon.pas` lines 971 to 1262. `ReadBool('FixX', true, false)` means the
  fix is on by default and hidden from the ini.
- **Shipped readme.** `MMPatches/Install/MM6patch/Files/MM6Patch ReadMe.TXT`, the version history with every `[-]` bug
  fix, `[+]` feature and `[*]` change since 2.0, plus Mok's and Angel's earlier fixes.
- **Shipped data.** `MMPatches/Install/MM6patch/Data/00 patch.games.lod` (26 map entries), `00 patch.icons.lod` (44
  script and table entries), `OptData/00 patch.sprites.lod` (skeleton and enforcer art) and `Files/Data/00
  add.icons.lod` (patch UI icons). GrayFace's custom-archive convention: `Data\*.games.lod` entries override
  `games.lod` entries, and the same for every other archive.
- **Decoded data and diffs.** `GrayFace-MM6Patch-2.5.7-decoded/`. `icons/` and `games/` hold every inflated entry.
  `diffs-vs-gog-original/` holds a unified diff per changed table and a record-level diff per changed script against
  the unpatched GOG files.

The owner's GOG install is **unpatched**: no `MM6patch.dll`, no `Data\00 patch.*.lod`, and `MM6.ini` holds only the
three Mok keys. Everything the engine reads is the official 1.2 data.

## What the patch is

Three layers, and only the first two matter to a reimplementation:

1. **Engine bug fixes**, applied as code hooks. About 90 distinct fixes for MM6. These are the vanilla behaviours our
   runtime either reproduces or has already fixed. The cross-reference tables below are the record.
2. **Data fixes**, shipped as override archives. 26 map geometry or placement files and 34 event scripts, plus three
   text tables. The data inventory below lists them.
3. **Platform and QoL work**: DirectDraw and 32-bit colour, window sizing, Smack and MSS replacements, mouse look, quick
   saves, MP3 music, key remapping, custom LOD loading. The QoL table at the end maps those options onto what the engine
   already has and marks the rest not applicable.

The patch also adds one event opcode, `0x45` (69) `RefundChestArtifacts`, used by the patched `d18`, `outb2`, `hive` and
`pyramid` scripts. Our `EvtOpcode` enum stops at 68, so an overlay of the patched scripts needs that opcode handled
before it can load.

## Engine bug fixes, cross-referenced

The tables below list every GrayFace fix that changes MM6 game logic. "Patch default" is what a GrayFace-patched game
does out of the box. "Ours" is what our MM6 runtime does today, with the file that decides it. State values: **vanilla**
means we reproduce the original bug, **fixed** means we match the patch, **mm7** means we inherit MM7 behaviour which
may or may not be right for MM6, **absent** means the feature isn't implemented so the question doesn't arise yet,
**unclear** means nobody has read the path.

### Combat and spells

MM6 runs behind verification lists: `src/Engine/MM6/MM6SpellEffectGuard.h` for player spells and
`MM6MonsterSpellEffect.h` for monster spells. A spell not on the list throws instead of running inherited MM7 code, so
**absent** below means the path throws today, and the GrayFace decision is still to be made when the spell is admitted.

- **C1**. Every monster spell dealt Fire damage (`FixMonsterSpells=1`, `MMHooks.pas:3435`). State: **fixed, party
  side**.
  `mm6::monsterSpellDamageType` in `MM6MonsterSpellEffect.h` uses the spell's own element, called from `Actor.cpp` for
  hits on the party. A non-Fire monster spell hitting another monster still throws. Unit test
  `MM6MonsterSpells.SpellsUseOriginalMonsterDiceAndOwnElement`, game test `DISABLED_FirstMapMonsterSpellsHitParty`.
- **C2**. Monster Paralyze did nothing (`FixParalyze=1`, `Hooks.pas:2868`, picks a target member and applies condition
  12). State: **absent**.
  Spell 86 is not a verified monster spell, `AI_SpellAttack` throws.
- **C3**. AOE damage skipped paralyzed monsters (`MonsterIsAliveHook`, `MMHooks.pas:3120`). State: **fixed**.
  `Actor::CanBeDamaged` excludes only stoned, shared code, MM7 tests `Issues.Issue415a-d`. No admitted MM6 player attack
  spell deals area damage and `HostilityTable::InitializeMm6` keeps monsters friendly to each other, so a Percival or
  Carnage arrow's explosion is the area damage a player can aim at monsters. Test
  `MM6GrayFaceShared.ExplosionHitsParalyzedMonster`.
- **C4**. Elemental weapon enchants ignored resistances (`FixBonusDamageProc`, `Hooks.pas:1432`, routes the bonus
  through the monster resistance roll). State: **absent**.
  `MM6CombatRules.h` throws on any special enchantment other than Antique, so no enchant reaches the shared
  resistance-aware path in `Actor.cpp`.
- **C5**. Second-hand weapon doubled recovery (`FixDualWeaponsRecovery=1`, works like MM7). State: **fixed by MM7
  rule**.
  `Character::GetAttackRecoveryTime` takes the slower weapon, no MM6 branch.
- **C6**. "Increases rate of Recovery" enchant did nothing, patch makes it +10% (`IncreaseRecoveryRateStrength=10`,
  `Hooks.pas:802`). State: **fixed**.
  Fixed 2026-11-18 behind `gameplay.fix_recovery_enchant`. MM6 now divides every new recovery by 1.1, rounded, in
  `Character::SetRecoveryTime` and in the turn-based queue (`_406457`), the two places GrayFace hooks (0x482C8A,
  0x406889), and `Character::Recover` no longer scales the time passing. Read statically, MM6.exe's own `Recover`
  (0x482BB0) and the per-frame loop at 0x4885F9 do scale elapsed recovery by 1.5 for special enchantment 17, the 50 at
  0x482E60 that GrayFace replaces. Why the readme calls it non-working was not found, so the flag-off path keeps the old
  real-time +50%. Tests `MM6Melee.RecoveryEnchantShortensRecoveryByTenPercent` and
  `MM6TurnBased.RecoveryEnchantShortensWaitForNextTurn`.
- **C7**. Scholar gave no +5% experience (`FixScholarExp`, `Hooks.pas:2262`). State: **fixed**.
  `learningPercent` adds 5 for Scholar, used by `Party::GivePartyExp`. MM7 test `Issues.Issue1068`, MM6 test
  `MM6GrayFaceShared.ScholarAddsFivePercentExperience`.
- **C8**. +2/+3 weapon-skill hirelings (professions 15, 16, 46) didn't reduce weapon delay (`CheckWeaponNPCs`,
  `Hooks.pas:2602`). State: **mm7, wrong shape**.
  Arms Master and Weapons Master add +2/+3 to `SKILL_ARMSMASTER`, a skill MM6 doesn't have, and the recovery formula
  subtracts that level. Net effect is a hidden flat cut, not the MM6 "+N to all weapon skills" rule.
- **C9**. Finger of Death gave no experience (`FixFingerDeathBug`, `Hooks.pas:1753`). State: **absent**.
  Spell 95 not verified for players, refused. The shared kill path would award XP.
- **C10**. Condition cures lasted 3 hours or 3 days per skill point instead of 1, overflowing at high skill
  (`FixConditionSpells`, `Hooks.pas:1331`, 10800 to 3600 seconds). State: **fixed for 67, flag**.
  `mm6::applyTimedCure` in `MM6TimedCureRules.cpp` holds MM6.exe's windows, 180, 3600 and 86400 seconds per point for 40
  and 49 and 180, 10800 and 259200 for 57, 60, 64 and 67, and takes GrayFace's 3600 and 86400 while
  `gameplay.fix_condition_spells` is on (default). Skill is clamped to 63. Only Cure Weakness (67) is admitted, Cure
  Poison and Cure Disease aren't covered. Unit test `MM6TimedCureRules.SixConditionsAndStrictRankWindows`, game test
  `MM6PlayerSpells.CureWeaknessWindow`.
- **C11**. Charm had the wrong Master duration (`FixCharm`, `Hooks.pas:2901`, doubles the Master value). State:
  **absent**.
  Charm (61) refused for players.
- **C12**. Static Charge Expert cost wrong, costs not read from `spells.txt` (`FixSpellsCost`, `Hooks.pas:2857`, copies
  `spells.txt` costs over the exe table). State: **vanilla, deliberate**.
  `mm6::spellManaRecovery` reads the spell table in MM6.exe on purpose, see [MM6 rules from MM6.exe](mm6-executable-rules.md). The
  exe row for Static Charge is mana 2/2/0 where `spells.txt` says 2/1/0.
- **C13**. Water Walk drained mana every 5 minutes (`FixWaterWalkManaDrain=1`). State: **fixed, config**.
  `Engine.cpp` behind `gameplay.fix_water_walk_mana_drain`, default true. No test pins the interval.
- **C14**. Stronger but shorter buff was refused when a weaker longer one was active (`FixCompareBuff`,
  `MMHooks.pas:2039`, accepts if longer **or** stronger). State: **fixed, config**.
  `SpellBuff::Apply` in `Spells.cpp` takes a strictly stronger buff even when it ends sooner, with the new shorter
  expiry, behind `gameplay.fix_stronger_shorter_buff` (default on, MM6 only). It compares power alone, as the hook does.
  Every MM6 buff goes through it: player casts, potions, event Bless and Day of the Gods, hireling Fly and Water Walk.
  Game test `MM6PlayerSpells.StrongerShorterBlessReplacesLongerOne`.
- **C15**. Light Bolt no double damage to Undead. State: **n/a**.
  MM7 and MM8 hook only, MM6 has no Light Bolt.
- **C16**. Armageddon unresistable. State: **n/a**.
  The MM6 default `ArmageddonElement` equals the vanilla byte, the hook changes nothing in MM6. Ours refuses the player
  cast and no-ops the event cast.
- **C17**. Negative resistance divided by zero (`NegativeResHook`, `MMHooks.pas:3039`). State: **fixed**.
  `mm6::resistedDamage` guards `divider > 0`. Unit test `MM6CombatRules.ResistanceRollRunsWithoutLuckOrResistance`.
- **C18**. Turn-based turn took twice as long (`Hooks.pas:2979`, 426 to 213 ticks). State: **fixed**.
  Fixed 2026-11-18 behind `gameplay.fix_turn_based_turn_time`. The MM6 branch of `NextTurn` advances 213 ticks,
  MM6.exe's 426.67 (float at 0x4B9370) with the flag off. Monster movement per round doesn't change, it runs on the
  64-tick AI timer. Tests `MM6TurnBased.PartyAndMonstersTakeTurns` (flipped) and `RoundTakesFiftyGameSeconds`.
- **C19**. Starburst and Meteor Shower hit beyond arrow range (`FixStarburst=1`, `Hooks.pas:1048`). State: **absent**.
  Player casts of 9 and 22 refused.
- **C20**. Blasters, Acid Burst, Blades, Flying Fist, Toxic Cloud and Shrapmetal couldn't target rats, launch at
  height/2 while aim used height/3 (`FixDragonTargeting`, `Hooks.pas:2562`). State: **fixed by consistency**.
  MM6 attack spells and blasters launch from party height/3 (`CastSpellInfo.cpp`), aim uses /3 (`Actor.cpp`).
  Consistent, nothing checks it against a rat.
- **C21**. Party attacked monsters in their dying animation (`CanAttackMon`, `MMHooks.pas:2941`). State: **fixed**.
  Melee falls back unless `CanBeDamaged`, `FindClosestActor` filters Dying, `DamageMonsterFromParty` returns on
  `IsNotAlive`. Test `MM6GrayFaceShared.BlowSkipsDyingMonster`.
- **C22**. Shield and armor recovery penalties (Mok's patch). State: **mm7, open**.
  MM7 multiplier table in `Character.cpp:1864`. MM6 `skilldes.txt` says armor Expert reduces and Master eliminates the
  penalty, our table keeps half for Plate at Master. Mok's actual change not traced.
- **C23**. Blaster recovery floor (`BlasterRecovery=4`, game default 0). State: **fixed, QoL**.
  `gameplay.minimum_recovery_blasters` default 5.
- **C24**. Monsters jumped off bridges into lava (`MonsterJumpDownLimit=500`, `Hooks.pas:2522`). State: **vanilla**.
  No drop-height limit in indoor or outdoor actor movement.
- **C25**. Random item generation favoured the first candidate and starved the last (`FixItemGeneration`,
  `MMHooks.pas:3705`, one-off in the cumulative compare). State: **fixed**.
  The hook sits after MM6.exe's `rand() % total` at 0x448A33 and adds 1 (`inc edx`), on the requested-kind path that
  shops and `GiveItem` use. A total of 0 jumps past it and still takes the first candidate. MM6.exe's any-item roll at
  0x448AFA has the same off-by-one, a roll of 0 takes item 1, and the patch leaves it, so chests stay vanilla. In the
  MM6 data the last kind 22 candidate at treasure levels 4 to 6 has weight 1, the Toxic Cloud, Day of Protection and
  Armageddon books, so vanilla magic shops 36 to 41 and the `global.evt` level 4 and 6 kind 22 rewards never give
  them, and a roll of 0 gives a Helm, which has weight 0 there. Fixed 2026-11-19 behind `fix_item_generation`. Tests
  `MM6ItemGenerator.KindRollGivesEveryCandidateItsWeight` and `MM6Houses.WeaponShopStockGivesEveryCandidateItsWeight`.
- **C26**. Clubs had no sound and no delay. State: **fixed, vanilla**.
  MM7 and MM8 hooks only. Our club delay is MM6's native 100 ticks. MM6.exe's swing sound comes from a jump table
  (0x42A3AE, table 0x42A408) indexed by the item skill byte, where club and an empty or broken hand are both 0 and play
  sound 83, the mace swing. So the silent club was a port bug, not vanilla. Fixed 2026-11-18 without a flag, together
  with the empty hand, which played the staff's 81. Test `MM6Melee.ClubAndEmptyHandSwingLikeAMace`.
- **C27**. A hit that Endurance reduced to zero recovery still switched the active character (`FixZeroRecovery`,
  `MMHooks.pas:3001`). State: **fixed**.
  `SetRecoveryTime` returns early at zero, and `Party::switchToNextActiveCharacter` keeps an active member who can act.
  Either guard alone holds the first hit, the second hit needs both. Test
  `MM6GrayFaceShared.HitWithoutRecoveryKeepsActiveCharacter`.
- **C28**. Quick Spell mana check used the wrong school (`FixQSpellPointsCheck`, `MMHooks.pas:3024`). State: **fixed**.
  `KeyboardInputHandler.cpp` gates on the spell's own school and the MM6 table.
- **C29**. Progressive dagger triple damage, 1% per skill point (`ProgressiveDaggerTrippleDamage=1`). State: **fixed**.
  `Character.cpp:1098` triples at `random(100) < level`. MM7 test `Issues.Issue1721`.

### World, events, timers, travel, saves and UI

- **W1**. Event timer countdown reset on every visit, and a late revisit fired a periodic timer many times, so wells
  refilled dozens of uses (`FixTimers=1`, `Hooks.pas:2336-2445`, stores the last firing per period kind and triggers
  once per period). State: **fixed**.
  `Processor.cpp` records each `OnLongTimer` firing per map and interval in `mm6::mapTimerFirings()`, saved in the
  optional EC6 `TIMR` section, and a map load sets the alarm one interval after that firing
  (`mapTimerFirstAlarmAfterLastFiring` in `MM6MapVisit.cpp`), falling back to the last visit for a save without it, as
  `FixTimerValidate` does. A missed alarm fires once and the next comes one interval later. A daily `OnTimer` goes off
  at its time of day today if that's still ahead. Flag `gameplay.fix_map_timers`, off gives the vanilla reset. Tests
  `MM6MapVisit.FountainRefillCountsDownWhileAway` (with a save and a load while away),
  `FountainRefillsADayAfterTheLastVisit`, `TimerWithoutIntervalWaitsForItsTimeOfDay`.
- **W2**. With nobody active, a fountain could be drunk more than once (`InactiveMemberEventsHook`, `Hooks.pas:464`,
  hooked at the event processor entry 0x43C7C0, sets CurrentMember to a random member who can act, `_Character_IsAlive`
  = CanAct 0x4876E0, or member 1 when nobody can, for one run of the processor and clears it afterwards. Not switchable
  in GrayFace). State: **fixed, held-item recipient open**.
  Vanilla MM6.exe runs the event with nobody active. Its click handler 0x4206D0, which the click and the Space messages
  both call at 0x42D4E6 and 0x42D501, never reads CurrentMember (0x4D50E8) before its event calls 0x4209C1 to 0x420ADC.
  The processor at 0x43C87C then targets 4 (active) with a CurrentMember and 6 (random) without, and the random target
  re-rolls `rand() & 3` on every command. Ours: `Interaction.cpp:81` `canRunInteractionEvent()` runs the event for MM6
  with nobody active, `EvtInterpreter::iterateCharacters` (`EvtInterpreter.cpp:2152`) picks one member who can act
  through `mm6InactivePartyEventTarget` the first time a command of the run targets the active character, and keeps it
  for every later command, `ForPartyMember` and the explicit active targets included. An event that never targets a
  member draws no random number. Flag `gameplay.fix_inactive_party_events`, off gives the vanilla per-command re-roll.
  Test `MM6EventMember.WellWithNobodyActive`. MM7 still refuses the click with nobody active. Not matched yet:
  GrayFace's CurrentMember also steers the other active-member reads of the run, like where an event moves a held item
  when it gives a new one (`MM6GiveItemEvent.cpp:34`, `EvtInterpreter.cpp` `VAR_PlayerItemInHands`), and ours still sees
  nobody active there.
- **W3**. Inactive members couldn't open chests (vanilla), could attack and could use scrolls
  (`FixInactivePlayersActing=1`, `FixInfiniteScrolls=1`). State: **fixed, scrolls absent**.
  Chest screen lets any member be selected and `Chest.cpp` needs an active member to open, attacking is refused for
  incapacitated members (`Engine.cpp:1526`, `UIGame.cpp:582`, `Character.cpp:4588`). MM6 scrolls throw.
- **W4**. New Day effects skipped at month start after "Wait until Dawn" then Esc (`MMHooks.pas:4092`). State: **fixed,
  config**.
  Esc calls `Rest(remaining)`. MM6 doesn't use the shared 3 AM check, `mm6::isNewDay` (0x4881C4) kept the vanilla
  `newDay > oldDay` compare, so the month-start jump was still skipped. Now behind `gameplay.fix_new_day_at_month_start`
  (default on, MM6 only), GrayFace's `newDay != oldDay`. Test
  `MM6GrayFaceShared.NewDayAtMonthStartAfterWaitUntilDawnAndEsc`.
- **W5**. Full outdoor brightness for a minute at 5:00 AM (`FixBright5AM`, `MMHooks.pas:3632`, feeds the current minute
  into the lighting update). State: **fixed, config**.
  `OutdoorLocation::UpdateSunlightVectors` is MM6.exe's 0x47BFE0 and stored the minute only between 5:00 and 21:00, so
  when 5:00 AM matched the stored minute the redraw check skipped the update and the sun and sprite dimming of the last
  daytime update stayed for a minute. Now behind `gameplay.fix_sunrise_light` (default on, MM6 only), which stores the
  minute on entry as GrayFace does. MM7 keeps the vanilla code. Test `MM6GrayFaceShared.SunRisesAtFiveAm`.
- **W6**. Waiting in the Rest dialog didn't tick character recovery (`FixWaitHook`, `Hooks.pas:1592`). State: **fixed**.
  `Engine.cpp:1441` runs `Character::Recover` inside `Rest`. Test `MM6FirstCombat.WaitAnHourNeitherEatsNorHeals` pins HP
  and food, `MM6GrayFaceShared.WaitTicksRecovery` pins recovery.
- **W7**. Coach and boat fare was withdrawn before the autosave, so the arrival autosave already lacked the gold
  (`TravelGoldFixHook`, `Hooks.pas:1623`, subtracts after `DoSaveGame`). State: **fixed, better than GrayFace**.
  `MM6Transport.cpp` takes the fare and passes the trip's days in `depart()` after the map change's autosave, so the
  autosave has the party at the house with its gold and date. Flag `gameplay.fix_travel_autosave`. Test
  `MM6Transport.CoachAutosaveKeepsTheFare` loads the autosave.
- **W8**. Horseman and boatman speech lengths detected automatically. State: **fixed, unobservable until travel voices
  load**.
  `mm6::transportDepartureDelayMs` waits for the speech `Character::playReaction` played, capped by the vanilla 1500 and
  2500 ms, flag `gameplay.fix_travel_speech_delay`. The shared speech table has no voice for travel speeches 71 and 72,
  so no sample plays and the cap applies, test `MM6Transport.CoachLeavesAfterTheOriginalDelayWithoutSpeech`. GrayFace
  waits nothing without a sample, we keep the vanilla delay.
- **W9**. Map-crossing dialog offered while flying low, accepting did nothing (`TravelWalkTriggerFixHook`,
  `Hooks.pas:1639`). State: **fixed**.
  `Outdoor.cpp` offers through `borderOfferPartyStatePermits`, the flags plus the confirmation's flying check, flag
  `gameplay.fix_flying_border_offer`. Unit test `OfferRefusesFlyingLikeTheConfirmation`, game test
  `MM6MapTravel.FlyingLowOverTheEdgeOffersNoTrip`.
- **W10**. Lloyd's Beacon autosaved after taking SP and recovery, Town Portal autosaved after recovery and even within
  the same map, Town Portal wasted the turn on cancel, Town Portal and Enchant Item didn't pause (`Hooks.pas:1705-1750`,
  `2461-2508`, `TPFixHook`). State: **absent, decide at admission**.
  Spells 29, 31 and 33 are not admitted, refused in `MM6SpellEffectGuard.h` and `Game.cpp:140`. The shared books keep
  the vanilla order and MM7 town data.
- **W11**. Ground items from one map appeared on another after walking across a border (`FixItemDuplicates=1`). State:
  **fixed by design**.
  `Outdoor.cpp:545` and `MM6IndoorSnapshot.cpp:490` rebuild sprite objects from the map's own data,
  `MM6MapTransition.cpp:139` restores only that map's items.
- **W12**. On respawn, artifacts generated in never-opened chests must be cleared from the found list, and the
  exclusive-artifact generator was broken and didn't mark artifacts found (`FixUnmarkedArtifacts=1`,
  `MMHooks.pas:1370-1560`, plus the `RefundChestArtifacts` opcode in scripts). State: **half: clearing is vanilla,
  picker is fixed**.
  Respawn at `Outdoor.cpp:511` never touches `generatedArtifactFlags`, nothing clears them. The live direct-artifact
  picker is our own safe policy (`MM6ChestDirectArtifactPicker.h`), marks generated and caps at 13, with the broken
  original kept detached for tests.
- **W13**. Items that didn't fit a chest reappear when space is freed (`FixChests=0`, off), most valuable items placed
  first (`FixChestsByReorder=1`), compact stacking (`FixChestsByCompacting=1`), vertical stacking
  (`PlaceChestItemsVertically=1`). State: **vanilla, put-in absent**.
  `MM6ChestPlacement.h` places once in random order, items that don't fit keep a slot with no cell and are never shown,
  no retry, reorder or compaction. Putting an item into an MM6 chest throws. Unit test
  `PlacementKeepsNoFitItemWithoutExtraWandDraw`.
- **W14**. Event commands couldn't operate on some skills, Thievery included (four range bytes in `Hooks.pas`). State:
  **fixed range**.
  `MM6SkillEvent.cpp` accepts all 31 slots 56..86 for Compare and Set, Add and Subtract throw. Tests
  `MM6SkillEvent_ut.cpp`, `MM6SkillEvent` game suite.
- **W15**. Door state command accepts 3 to switch a moving door (`DoorStateSwitchHook`, `Hooks.pas:1213`). State:
  **vanilla, modder feature**.
  `Indoor.cpp:786-834` ignores action 2 while moving, action 3 matches nothing. Only the patched `cd2.evt` could need
  it.
- **W16**. Save slots couldn't have gaps and save and load targeted different slots, Quick Save broke in the Hive after
  the Reactor. State: **fixed by the list design**.
  New saves take the first free `saveNNN.ec6`, the menu is a file list, Quick Save has no Hive check, only autosave is
  skipped during the Hive finale.
- **W17**. Awards page not refreshed on Tab (MM6 and MM7 hook). State: **fixed**.
  `UICharacter.cpp:971` refills awards when the active member changes. Arrow keys unbound, and Tab doesn't switch
  members on the character screen, the digit keys do. Test `MM6GrayFaceShared.AwardsFollowActiveCharacter`.
- **W18**. Buff duration shown wrong for "1 day 5 minutes" (`DoFixBuffTime`, `MMHooks.pas:3726`, shows the two largest
  units). State: **fixed, config**.
  MM6.exe 0x41A2F0 skips minutes when there are days (the `jne` at 0x41A472 that GrayFace removes), and the shared
  `MakeDateTimeString` did the same, so a day and 5 minutes showed as "1 Day". Now behind
  `gameplay.fix_buff_duration_text` (default on, MM6 only), buff times show the two largest nonzero units. Test
  `MM6GrayFaceShared.BuffDurationShowsMinutesNextToDays`.
- **W19**. Condition priority order for display and stats (`FixConditionPriorities=1`, `MMHooks.pas:2080`), stats screen
  shows the condition that sets the stats. State: **display fixed, stats screen differs**.
  Display order is the GrayFace order behind `gameplay.alternative_condition_priorities` (default true). Stats use the
  original MM6 order in `MM6ActualStatCondition.h`. The MM6 stats screen shows the display condition, not the
  stat-setting one.
- **W20**. Depleted wands break instead of vanishing (`KeepEmptyWands=1`). State: **absent, config ready**.
  MM6 wand fire throws. `gameplay.destroy_discharged_wands=false` keeps an empty wand.
- **W21**. Some scrolls were endless on the paper doll in turn-based mode, right-click scroll use auto-picked the target
  (`Hooks.pas:675-740`). State: **absent**.
  MM6 scrolls throw.
- **W22**. Black potion with no effect was not consumed (`DontWasteBlackPotion`, `Hooks.pas:2288`, hooked at 0x459103,
  for items 181 to 187 whose flag at character +0x128 is set it jumps to 0x459EE5, sound 27 and return, past the "can
  not be used that way" line). State: **fixed, config**.
  Vanilla MM6.exe checks the flag inside the Essence effect, so a repeat still ran the common tail 0x459977 and the
  mouse item was cleared. `Character::useItem` now checks `mm6::isRepeatedEssence` (`MM6Potion.h`) first and keeps the
  potion in the hand with the error sound, no status text, speech or recovery. Only the seven Essences, as in the patch.
  Flag `gameplay.fix_wasted_black_potion`. Test `MM6Potion.RepeatedEssenceIsNotWasted`.
- **W23**. Circus removed all trophies, Cloak of Baa given on every repeat talk. State: **vanilla by data**.
  Both are script bugs in `global.evt`, fixed by the patched script (data inventory). Our engine runs the shipped script
  faithfully. `MM6CampaignChainC4` grants the cloak once and never repeats the talk.
- **W24**. SFT and TFT frames: first frame too long, last too short (`FixSFT=1`). State: **fixed**.
  `Sprites.cpp:175` and `TextureFrameTable.cpp:34` use `t >= frameLength`, correct boundaries, no MM6 path. Test
  `MM6GrayFaceShared.AnimationFramesTurnWhenTheirTimeIsUp` covers 993 MM6 sprite animations and the two TFT animations.
- **W25**. Movement rounding, jump height, strafe precision (`FixMovement=1`). State: **n/a**.
  Float physics shared with MM7, nothing to round.
- **W26**. Shops bought blasters (`CanSellItemHook`, `Hooks.pas:2632`, in the shop hint routine 0x485120 for the sell
  action, and `CanSellItemHook2`, `Hooks.pas:2645`, at the sell click 0x4A53C9, both refuse an item whose items.txt
  Value is 0). State: **fixed, config**.
  Vanilla MM6.exe's trade check 0x4A4C30 lets weapon shops take any single-handed through bow type and the general store
  anything up to 445, so a blaster sold for 1 gold. In MM6 items.txt the Value 0 items a shop would otherwise take are
  blasters 64 and 65 and gold 197 to 199. `GUIWindow_MM6Shop::sell` now refuses a Value 0 item. Identify and repair
  still take a blaster, as with the patch. Flag `gameplay.fix_shops_buying_blasters`. Test
  `MM6Houses.WeaponShopRefusesBlaster`. Our MM6 shops show no sell hint, so the 0x485120 half has nothing to hook yet.
- **W27**. Buy dialog read out of bounds with no active member. State: **fixed**.
  `MM6Shop.cpp:141` returns without an active member.
- **W28**. Unconscious members could identify and repair in shops (`FixDeadPlayerIdentifyItem=1`, `MMHooks.pas:3062`,
  hooked at MM6 0x41C58E). State: **fixed, paid path vanilla**.
  The patch hook sits in the item info box (0x41C440), where the selected member's Identify Item and Repair skills act
  on a right-clicked item. It treats a member who can't act (0x4876E0) as no member. Paid shop identify and repair were
  never broken in MM6.exe: the shop click handler 0x4A4CA0 refuses a member who can't act with "%s is in no condition to
  %s" (427, 562) and sound 27, and every shop screen draws that line in the right panel. Our port had dropped that
  check, so `MM6Shop.cpp` now refuses the same way, unconditionally since it is vanilla. Tests
  `MM6Houses.WeaponShopIdentifyUnconscious` and `WeaponShopRepairUnconscious`. The proprietor's menu is gated the same
  way in MM6.exe (state 1 of the weapon, armor, magic and general store, guild, mercenary and thieves' guild, stable and
  boat, tavern and training draws), and ours now hides it behind that line too, tests
  `MM6Houses.WeaponShopMenuUnconscious` and `TrainingUnconscious`. Our MM6 shops show no item info box on right-click,
  and the map's inventory and chest info boxes already refuse a member who can't act (`UIPopup.cpp`), so the skill path
  has no hole yet. Porting the MM6 shop item info box must bring the patch's check with it.
- **W29**. Free Haven Smuggler's Guild added to the map. State: **vanilla by data**.
  House 151 exists in `2devents.txt` and `MM6HouseServiceData.cpp:203` gives it skills, but the GOG `Outc2.odm` has no
  door to it. The patched `Outc2.odm` and `outc2.evt` add the entrance.

### Owner decision, 2026-11-15: fix them all

The owner decided that MM6 fixes every bug in the tables above, the way GrayFace did or a better way where one exists.
This replaces the earlier "decide per item" stance. It also reverses the four deliberate-vanilla choices (C12, C18, C25,
W9), so their pinning tests flip with the fix. The MM6 playtest board keeps its priority over this backlog, and each
fix lands as its own reviewed change with a game test that reproduces the player-visible symptom. The entries below
say which ones are done.

Rules for every fix in this backlog:
- A fix that changes game logic sits behind a `gameplay.fix_*` config flag that defaults on, like
  `fix_water_walk_mana_drain`. GrayFace makes most fixes switchable through `mm6.ini`, and the flag keeps a vanilla run
  one switch away.
- A fix that needs new saved state stores it in our own save fields, never in a layout the original MM6 can't read back.
  GrayFace keeps its timer state in a patch-only map extra for the same reason.
- An admission-time item is fixed in the packet that admits the spell or feature. The spell isn't admitted with the
  vanilla bug and fixed later.

- **C14**. Stronger but shorter buff. Accept the new buff when it is longer **or** stronger, as GrayFace does.
  Same as GrayFace.
- **C24**. Monsters jump into lava. Refuse a walking step down more than 500 units for a non-flying monster, the
  GrayFace limit.
  Same as GrayFace. Our actor movement has no drop check at all, so this is new code in both indoor and outdoor
  movement.
- **W1**. Timer countdown resets on every visit. Store each map's last periodic firing per period kind and schedule from
  it, with GrayFace's daily-at-fixed-time rule.
  Same as GrayFace, stored in our save fields. Keeps the catch-up guard we already have. Done 2026-11-18 behind
  `fix_map_timers`.
- **W2**. Random member per command when nobody is active. Run the event as vanilla does, and pick one member who can
  act once per event run, falling back to member 1, kept for every command. An event that resumes after a question or a
  dialogue is a new run and picks again.
  Same as GrayFace, owner's choice 2026-11-18. Done 2026-11-18 behind `fix_inactive_party_events`. Our engine used to
  refuse the click with nobody active, which is neither vanilla nor GrayFace.
- **W7**. Fare taken before the travel autosave. Autosave before the fare and the travel days are applied.
  Better than GrayFace, which only moves the gold. Ours also applies the travel days before the save, so the save should
  come first. Done 2026-11-18 behind `fix_travel_autosave`.
- **W8**. Fixed coach and boat speech delay. Wait for the actual speech sample to finish, capped by the vanilla 1500 and
  2500 ms.
  Same as GrayFace. Done 2026-11-18 behind `fix_travel_speech_delay`. No travel speech plays yet, so the cap is what
  applies.
- **W12**. Found-artifact flags never cleared on respawn. On respawn, clear the found flag for every artifact generated
  in a chest that was never opened, and implement opcode 69 `RefundChestArtifacts`.
  Same as GrayFace. Opcode 69 also unblocks the patched scripts.
- **W13**. Chest overflow items hidden forever. At generation, place items most valuable first and compact them, then
  place leftovers when the chest is reopened with space.
  GrayFace ships reorder and compacting on and leftover placement off. We turn all three on, because a hidden item is
  lost loot for the player.
- **W15**. Door action 3 ignored. Accept action 3 as "switch even while moving".
  Same as GrayFace, cheap and needed by anything that runs patched scripts.
- **W22**. Black potion wasted. Leave the potion in the inventory when it has no effect.
  Same as GrayFace. Done 2026-11-18 behind `fix_wasted_black_potion`. The potion stays in the hand, as in the patch.
- **W26**. Shops buy blasters. Exclude blasters from every shop's trade list.
  Same as GrayFace. Done 2026-11-18 behind `fix_shops_buying_blasters`. Like the patch it refuses only the sale, of any
  item with no value.
- **W28**. Unconscious members identify and repair. Require the selected member to be able to act before identify or
  repair in a shop, and on the map.
  Same as GrayFace. Done 2026-11-18 for the paid shop path, which MM6.exe already refused, so it has no config flag. The
  skill path is unreachable today, see the W28 row above.
- **C12**. Static Charge cost from the exe table. Read Normal, Expert and Master costs from `spells.txt`, as GrayFace
  does.
  Same as GrayFace. Flips the exe-table decision described in [MM6 rules from MM6.exe](mm6-executable-rules.md).
- **C18**. Turn-based turn twice as long. Advance 213 ticks per turn instead of 426.
  Same as GrayFace. Flips `MM6TurnBased.PartyAndMonstersTakeTurns`. Done 2026-11-18 behind `fix_turn_based_turn_time`.
- **C25**. Item roll favours the first candidate. Fix the off-by-one in the cumulative compare so every candidate gets
  its weight.
  Same as GrayFace. Done 2026-11-19 behind `fix_item_generation`. The any-item roll has the same off-by-one and stays,
  as in the patch.
- **W9**. Border dialog offered while flying low. Don't offer the map-crossing dialog while flying.
  Same as GrayFace. Flips `ConfirmationChecksFlyingSeparately`. Done 2026-11-18 behind `fix_flying_border_offer`.
- **C6**. Recovery enchant at the MM7 +50%. +10% recovery rate for MM6, the GrayFace default.
  Same as GrayFace. MM6 vanilla did nothing, so 50% would be a new rule rather than a fix. Done 2026-11-18 behind
  `fix_recovery_enchant`, see the C6 row above about MM6.exe's code.
- **C8**. Weapon-skill hirelings add a hidden skill. Apply the +2 and +3 to the weapon skills, as `npcprof.txt`
  describes, for delay and for whatever vanilla already applied it to. Remove the MM7 Armsmaster route for MM6.
  Better than GrayFace, which only adds the bonus to delay. Trace what vanilla applies the bonus to before writing it.
- **C22**. Armor recovery uses the MM7 table. Trace Mok's original change and MM6's skill text, then make Master armor
  eliminate the penalty if both agree.
  Mok's fix isn't in the GrayFace source, so the target needs tracing first.
- **C26**. Clubs swing silently. Play the mace swing sound for clubs.
  Not an MM6 GrayFace item, but the same bug GrayFace fixed for MM7 and MM8. Done 2026-11-18. MM6.exe plays the mace
  swing for clubs, so it has no config flag.
- **W19**. Stats screen shows the display condition. Show the condition that determines the stats on the stats screen,
  the most severe one everywhere else.
  Same as GrayFace 2.5.2.
- **W5, W18**. 5:00 AM brightness, buff duration text. Pin with a game test, fix only if the test shows the vanilla bug.
  Done 2026-11-18. Both showed the vanilla bug and are fixed behind `fix_sunrise_light` and `fix_buff_duration_text`.
- **C3, C7, C21, C27, W4, W6, W17, W24**. Fixed in shared code, no MM6 test. Add an MM6 game test for each.
  Done 2026-11-18 in `GameTests_MM6GrayFaceShared.cpp`. W4 turned out vanilla in MM6's own day check and is fixed behind
  `fix_new_day_at_month_start`.

**Admission-time fixes.** C2 monster Paralyze applies the Paralyzed condition to one chosen member. C4 elemental weapon
enchants go through the monster resistance roll. C9 Finger of Death awards experience. C11 Charm uses the corrected
Master duration. C19 Starburst and Meteor Shower refuse targets beyond arrow range. W10 Lloyd's Beacon and Town Portal
autosave before taking spell points and recovery, Town Portal doesn't autosave within the same map and doesn't waste the
turn on cancel, and Town Portal and Enchant Item pause the game. W20 an empty wand stays in the inventory. W21 scrolls
don't repeat on the paper doll in turn-based mode, and a right-click scroll asks for its target.

**Data fixes.** Better than GrayFace's override archives, in two parts:
- The 34 script fixes become an engine-side patch table. Each entry names the original file's hash and the record-level
  edit from the diffs in the decoded folder, so the engine applies it only to the exact GOG 1.2 file and nothing derived
  from the game's data is shipped. Opcode 69 from W12 is a prerequisite. The three text tables are typo fixes and go in
  the same table.
- The 21 map geometry fixes can't be expressed as small edits. The engine loads `data/*.games.lod` override archives
  from the owner's data folder, the GrayFace custom-archive convention, so the owner can drop in the archive from the
  official GrayFace download. W29, the Smuggler's Guild door, arrives this way.


## Data fixes, inventory

Record-level diffs against the owner's GOG 1.2 files. Opcode numbers are MM6 event opcodes, `op 4` is `MouseOver` (the
hint shown for a map entrance), `op 6` is `MoveToMap`, `op 7` is `OpenChest`, `op 14` is `Compare`, `op 15` is
`ChangeDoorState`, `op 69` is the patch's `RefundChestArtifacts`. Names follow `src/Engine/Evt/EvtEnums.h`.

### Scripts (`00 patch.icons.lod`)

| File | Map | Change | Readme entry |
| --- | --- | --- | --- |
| `oute3.evt` | New Sorpigal | `MouseOver 0` added for events 101 to 103 | Missing dungeon/house hints in all outdoor locations |
| `outd3.evt` | Castle Ironfist | `MouseOver 0` added for events 90 to 94 | same |
| `oute1`, `oute2`, `outd1`, `outd2`, `outc1`, `outb3` `.evt` | Eel Infested Waters, Misty Islands, Silver Cove, Bootleg Bay, Frozen Highlands, Dragonsand | one `MouseOver 0` added | same |
| `outa1.evt` | Sweet Water | `MouseOver 0` added for events 90 and 205 | same |
| `outa3.evt` | Hermit's Isle | `MouseOver 0` added for event 90 | same |
| `outc2.evt` | Free Haven | `MouseOver 0` added for events 150 to 153 | same, and the four new entrances belong to the added Smuggler's Guild in `Outc2.odm` |
| `outc3.evt` | Mire of the Damned | hint and `SpeakInHouse` moved from event 68 to 69, two hints added | Duplicated house, hints |
| `outa2.evt` | Paradise Valley | house event 65 moved to 66, fountain `Cmp` branch target 6 to 7 | Depleted fountain showed the wrong text, duplicated house |
| `outb1.evt` | Kriegspire | Temple of Baa `MoveToMap` direction, four hints, fountain flag `0x6d5` to `0x8d5` | Wrong entry direction, Blackshire and Kriegspire fountains shared a flag |
| `outb2.evt` | Blackshire | events 73 and 68 gain `RefundChestArtifacts`, Lair of the Wolf direction, two hints | Repeatable statuettes reward, lost artifacts, wrong direction |
| `d04.evt` | Hall of the Fire Lord | chest events 42 and 61 restructured | Locked chest not openable after the trap detonated |
| `d06.evt` | Dragoons' Caverns | chest events 38 and 43 restructured | Quest item lost if the chest was opened by explosion, chests duplicated after the quest |
| `d10.evt` | Dragoons' Keep | chest events 29 and 42 restructured | Duplicated chest, Mordred dagger chest handling |
| `d11.evt` | Corlagon's Estate | event 50 step 7 `OpenChest` 8 to 11 | Quest item chest handling |
| `d13.evt` | The Monolith | event 24 per-class branches collapsed to one check | +5 Personality only for unpromoted Cleric and Druid |
| `d16.evt` | Warlord's Fortress | event 36 extended with a second quest-item check | Quest item chest handling |
| `d18.evt` | Gharik's Forge | event 48 extended with `RefundChestArtifacts` | Artifacts generated in inaccessible chests |
| `d19.evt` | Agar's Laboratory | event 20 `OpenChest` 0 to 2, event 35 reordered with a `Compare` guard | +50 Intellect any number of times, duplicated chests |
| `hive.evt` | The Hive | event 100 gains `RefundChestArtifacts` | Artifacts in inaccessible chests |
| `pyramid.evt` | Tomb of VARN | event 41 gains refund steps | same |
| `sci-fi.evt` | Control Center | event 72 gains a hint and `OpenChest` 14 | Duplicated chest |
| `t3.evt` | Temple of Tsantsa | event 55 variable `0x169` to `0x16a` | Spear under the pillow unobtainable, variable conflict |
| `t5.evt` | Temple of the Moon | altar events 16 and 21 | Might altar only below 50, Luck +2 instead of +5 |
| `t7.evt` | Superior Temple of Baa | event 29 `OpenChest` 6 to 0 | Duplicated chests |
| `cd2.evt` | Castle Darkmoor | event 29 `MoveToMap` flag, event 58 variable | Cube side not teleporting, barrier opening on reload |
| `oracle.evt` | Oracle of Enroth | event 2 split, new event 18 | Oracle gave wrong hints, star screen closing instead of opening |
| `sewer.evt` | Free Haven Sewer | event 25 `MoveToMap` destination | Richard Harp's exit led to a different house |
| `zarena.evt` | The Arena | exit `MoveToMap` confirm flag | Exit doors didn't ask for confirmation |
| `global.evt` | all maps | event 46 (circus prizes) rewritten, event 105 (Slicker Silvertongue) removed | Circus removed all trophies, Cloak of Baa given repeatedly |
| `d01.evt`, `t1.evt`, `dobjlist.bin`, `items.txt`, `npctext.txt`, `quests.txt`, `skilldes.txt` | | byte-identical to GOG 1.2 | shipped for localisation builds, no change for us |

### Tables (`00 patch.icons.lod`)

| File | Change |
| --- | --- |
| `global.txt` | four typo fixes: string 412 "Create Party", 489 "total gold", 574 "describes", 577 "wimps" |
| `npcnames.txt` | six name spellings |
| `proftext.txt` | one whitespace fix in row 54 |

### Maps (`00 patch.games.lod`)

Changed geometry or placement, no record-level diff: `CD2.blv`, `CD3.blv`, `D04.blv`, `d08.blv`, `D09.blv`, `D10.blv`,
`d16.blv`, `oracle.blv`, `sci-fi.blv`, `sewer.blv`, `t1.blv`, `T7.blv`, `znwc.blv`, `Outb1.odm`, `Outb3.odm`,
`Outc2.odm`, `Outc3.odm`, `Outd2.odm`, `outa2.odm`, `outd1.odm`, `outd3.odm`. Byte-identical to GOG: `D04.dlv`,
`d14.blv`, `d14.dlv`, `Outb2.odm`, `Outc1.odm`. The readme explains each: duplicated chests and houses, Dragonsand's two
Tomb of VARN doors sharing a texture, Snergle's cabinet front not clickable, sewer rats spawning inside walls, the Free
Haven Smuggler's Guild building, the Bootleg Bay circus entrance texture, Temple of Baa floor alignment, the New World
Computing door moving a wall.

## QoL and platform options

| GrayFace option (default) | Ours | Note |
| --- | --- | --- |
| `AlwaysRun=1`, `CapsLockToggleRun=0` | `settings.always_run` plus the toggle key | present |
| Quick saves, `QuickSavesCount=3`, F11 | `gameplay.quick_saves_count` and the quick save key | present |
| `NoIntro`, `PostponeIntro` | `debug.no_intro` | present |
| `NoDeathMovie=0` | none | not needed, MM6 death movie playback is its own open item |
| `TurboSpeed`-like `DoubleSpeedKey` F2 | `debug.turbo_speed` | present as a debug flag |
| `ReputationNumber=1` | none | QoL, shows the numeric reputation next to the word, not vanilla |
| Recovery time in attack and shoot hints, train-to level on experience hover | none | QoL, MM7 and MM8 show the level, MM6 vanilla doesn't |
| `BlasterRecovery=4` | none | GrayFace sets a floor on blaster recovery, game default 0 |
| `KeepEmptyWands=1` | `gameplay.destroy_discharged_wands=false` | present, same default as the patch |
| `FixChests*`, `PlaceItemsVertically=1` | own MM6 chest placement | see W13 |
| `MouseLook`, `MouseWheelFly`, `AlwaysStrafe`, `AutorunKey`, `InventoryKey`, `ToggleCharacterScreenKey` | engine key bindings cover some, mouse look not present | QoL, not fidelity |
| `PlayMP3`, `MusicLoopsCount`, `LoudMusic` | `AudioPlayer` plays the GOG `sounds/N.mp3` tracks for MM6 | not applicable |
| `SupportTrueColor`, `BorderlessFullscreen`, `WindowWidth/Height`, `Stretch*`, `CompatibleMovieRender`, `RenderMax*`, `Scaling*` and similar | engine renderer | not applicable |
| `DataFiles`, custom `Data\*.lod` archives, `ExeMods`, MMExtension loading | none | the custom-archive convention is the one piece worth copying, see the data-fixes decision |
| `NoCD`, MSS32 and Smack replacements, Delphi memory manager, registry key buffer, XP compatibility, exit crash, window activation sound stop | engine | not applicable |
| `FixSFT=1`, `TFT` timing, `SpriteAngleCompensation=1`, `ClickThroughEffects=1`, view distance, `WinScreenDelay` | renderer and UI | W24 covers SFT and TFT, the rest is presentation |

## How a lane uses this

- **Before claiming a vanilla MM6 behaviour**, grep the hook comments. If GrayFace has a hook for it, the vanilla game
  is buggy there and the decision is ours: reproduce, fix, or fix behind a config flag like `fix_water_walk_mana_drain`.
  Record the decision in the cross-reference table.
- **Before writing a chest, fountain, altar or quest-reward test for a map in the data inventory**, read that map's
  diff. The original script may be the bug, and a test that pins the original script's behaviour pins a bug.
- **When the owner reports a map problem** from the readme's "Fixes in maps" lists, the fix is data, not engine. Options
  are a local override archive on the owner's profile or an engine-side script patch, see the data-fixes decision above.

## Source

- [GameConfig.h](../src/Application/GameConfig.h), the `gameplay.fix_*` flags
- [GameTests_MM6GrayFaceShared.cpp](../test/Bin/GameTest/GameTests_MM6GrayFaceShared.cpp), MM6 tests for fixes that live
  in shared code
- [MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h) and
  [MM6MonsterSpellEffect.h](../src/Engine/MM6/MM6MonsterSpellEffect.h), the spell admission lists

See also: [MM6 port architecture](mm6-port-architecture.md), [MM6 combat and monsters](mm6-combat-and-monsters.md),
[MM6 travel and maps](mm6-travel-and-maps.md), [MM6 events](mm6-events.md), [MM6 campaign status](mm6-campaign-status.md),
[Source and privacy](source-and-privacy.md)
