# MM6 GrayFace patch reference

Status: **pinned source and shipped data decoded, engine cross-reference recorded, 2026-11-15**. This packet makes the GrayFace MM6 Patch a first-class reference for MM6 work. It records where the pinned copy lives, what the patch changes, which of those changes are vanilla engine bugs our MM6 runtime must decide on, which are data fixes we could overlay, and what each lane should do with it. MM7 is Codex's title and is out of scope here.

## Pinned copies

| What | Where | Notes |
| --- | --- | --- |
| Patch source, tag `MM6Patch-2.5.7`, commit `5d95fa3a74a49894527fa35442d59100327bd586`, 2022-03-04 | `local-game-data/public-references/GrayFace-Misc-MM6Patch-2.5.7/` (ignored) | Shallow clone of [GrayFace/Misc](https://github.com/GrayFace/Misc). Upstream `master` has no MM6 patch commit after this tag, so 2.5.7 is current. The MM7 lane's `GrayFace-Misc-MM7Patch-2.5.7` checkout is the same tree. |
| Hook table | `MMPatches/MM6Patch/Hooks.pas` lines 2913 to 3169, `MMPatches/MMCommon/MMHooks.pas` `HooksCommon` at line 4018 and the `{$IFDEF MM6}` `Hooks` array at line 4180 | Each row is an executable address plus a one-line comment naming the fix. The comments are the authoritative fix list. |
| Option defaults | `MMPatches/MMCommon/MMCommon.pas` lines 971 to 1262 | `ReadBool('FixX', true, false)` means the fix is on by default and hidden from the ini. |
| Shipped readme | `MMPatches/Install/MM6patch/Files/MM6Patch ReadMe.TXT` | Version history with every `[-]` bug fix, `[+]` feature and `[*]` change since 2.0, plus Mok's and Angel's earlier fixes. |
| Shipped data | `MMPatches/Install/MM6patch/Data/00 patch.games.lod` (26 map entries) and `00 patch.icons.lod` (44 script and table entries), `OptData/00 patch.sprites.lod` (skeleton and enforcer art), `Files/Data/00 add.icons.lod` (patch UI icons) | GrayFace custom-archive convention: `Data\*.games.lod` entries override `games.lod` entries, same for every other archive. |
| Decoded data and diffs | `local-game-data/public-references/GrayFace-MM6Patch-2.5.7-decoded/` (ignored) | `icons/` and `games/` hold every inflated entry. `diffs-vs-gog-original/` holds a unified diff per changed table and a record-level diff per changed script against the owner's unpatched GOG files. `lodls.py`, `lodinflate.py` and `evtdump.py` regenerate them. |

The owner's GOG install at the MM6 app data root is **unpatched**: no `MM6patch.dll`, no `Data\00 patch.*.lod`, and `MM6.ini` holds only the three Mok keys. Everything the engine reads today is the official 1.2 data. Nothing from the patch is redistributed through this repository, the ignored folders are the only copies.

## What the patch is

Three layers, and only the first two matter to a reimplementation:

1. **Engine bug fixes**, applied as code hooks. About 90 distinct fixes for MM6. These are the vanilla behaviours our runtime either reproduces or has already fixed. The cross-reference tables below are the record.
2. **Data fixes**, shipped as override archives. 26 map geometry or placement files and 34 event scripts, plus three text tables. The data inventory below lists them.
3. **Platform and QoL work**: DirectDraw and 32-bit colour, window sizing, Smack and MSS replacements, mouse look, quick saves, MP3 music, key remapping, custom LOD loading. The QoL table at the end maps those options onto what the engine already has and marks the rest not applicable.

The patch also adds one event opcode, `0x45` (69) `RefundChestArtifacts`, used by the patched `d18`, `outb2`, `hive` and `pyramid` scripts. Our `EvtOpcode` enum stops at 68, so an overlay of the patched scripts needs that opcode handled before it can load.

## Engine bug fixes, cross-referenced

The tables below list every GrayFace fix that changes MM6 game logic. "Patch default" is what a GrayFace-patched game does out of the box. "Ours" is what our MM6 runtime does today, with the file that decides it. State values: **vanilla** means we reproduce the original bug, **fixed** means we match the patch, **mm7** means we inherit MM7 behaviour which may or may not be right for MM6, **absent** means the feature isn't implemented so the question doesn't arise yet, **unclear** means nobody has read the path.

### Combat and spells

MM6 runs behind verification lists: `src/Engine/MM6/MM6SpellEffectGuard.h` for player spells and `MM6MonsterSpellEffect.h` for monster spells. A spell not on the list throws instead of running inherited MM7 code, so **absent** below means the path throws today, and the GrayFace decision is still to be made when the spell is admitted.

| # | GrayFace fix (patch default) | Ours | State |
| --- | --- | --- | --- |
| C1 | Every monster spell dealt Fire damage (`FixMonsterSpells=1`, `MMHooks.pas:3435`) | `mm6::monsterSpellDamageType` in `MM6MonsterSpellEffect.h` uses the spell's own element, called from `Actor.cpp` for hits on the party. A non-Fire monster spell hitting another monster still throws. Unit test `MM6MonsterSpells.SpellsUseOriginalMonsterDiceAndOwnElement`, game test `DISABLED_FirstMapMonsterSpellsHitParty`. | fixed, party side |
| C2 | Monster Paralyze did nothing (`FixParalyze=1`, `Hooks.pas:2868`, picks a target member and applies condition 12) | Spell 86 is not a verified monster spell, `AI_SpellAttack` throws. | absent |
| C3 | AOE damage skipped paralyzed monsters (`MonsterIsAliveHook`, `MMHooks.pas:3120`) | `Actor::CanBeDamaged` excludes only stoned, shared code, MM7 tests `Issues.Issue415a-d`. | fixed, no MM6 test |
| C4 | Elemental weapon enchants ignored resistances (`FixBonusDamageProc`, `Hooks.pas:1432`, routes the bonus through the monster resistance roll) | `MM6CombatRules.h` throws on any special enchantment other than Antique, so no enchant reaches the shared resistance-aware path in `Actor.cpp`. | absent |
| C5 | Second-hand weapon doubled recovery (`FixDualWeaponsRecovery=1`, works like MM7) | `Character::GetAttackRecoveryTime` takes the slower weapon, no MM6 branch. | fixed by MM7 rule |
| C6 | "Increases rate of Recovery" enchant did nothing, patch makes it +10% (`IncreaseRecoveryRateStrength=10`, `Hooks.pas:802`) | `Character::GetSpecialItemBonus` returns 50, the MM7 value, applied in `Character::Recover`. | mm7, +50% not +10% |
| C7 | Scholar gave no +5% experience (`FixScholarExp`, `Hooks.pas:2262`) | `learningPercent` adds 5 for Scholar, used by `Party::GivePartyExp`. MM7 test `Issues.Issue1068`. | fixed, no MM6 test |
| C8 | +2/+3 weapon-skill hirelings (professions 15, 16, 46) didn't reduce weapon delay (`CheckWeaponNPCs`, `Hooks.pas:2602`) | Arms Master and Weapons Master add +2/+3 to `SKILL_ARMSMASTER`, a skill MM6 doesn't have, and the recovery formula subtracts that level. Net effect is a hidden flat cut, not the MM6 "+N to all weapon skills" rule. | mm7, wrong shape |
| C9 | Finger of Death gave no experience (`FixFingerDeathBug`, `Hooks.pas:1753`) | Spell 95 not verified for players, refused. The shared kill path would award XP. | absent |
| C10 | Condition cures lasted 3 hours or 3 days per skill point instead of 1, overflowing at high skill (`FixConditionSpells`, `Hooks.pas:1331`, 10800 to 3600 seconds) | `mm6::applyTimedCure` in `MM6TimedCureRules.cpp` with 180, 3600, 86400 seconds per point and skill clamped to 63, so GrayFace's windows. Only Cure Weakness (67) is admitted, Cure Poison and Cure Disease aren't covered. Unit test `MM6TimedCureRules_ut.cpp:35`. | fixed for 67 |
| C11 | Charm had the wrong Master duration (`FixCharm`, `Hooks.pas:2901`, doubles the Master value) | Charm (61) refused for players. | absent |
| C12 | Static Charge Expert cost wrong, costs not read from `spells.txt` (`FixSpellsCost`, `Hooks.pas:2857`, copies `spells.txt` costs over the exe table) | `mm6::spellManaRecovery` reads `rules/spells.json`, exported from the exe table on purpose (`project/MM6_F4_SPELL_MANA_RECOVERY.md`). The exe row for Static Charge is mana 2/2/0 where `spells.txt` says 2/1/0. | vanilla, deliberate |
| C13 | Water Walk drained mana every 5 minutes (`FixWaterWalkManaDrain=1`) | `Engine.cpp` behind `gameplay.fix_water_walk_mana_drain`, default true. No test pins the interval. | fixed, config |
| C14 | Stronger but shorter buff was refused when a weaker longer one was active (`FixCompareBuff`, `MMHooks.pas:2039`, accepts if longer **or** stronger) | `SpellBuff::Apply` in `Spells.cpp:471` refuses whenever the new expiry is earlier, whatever the power. MM6 Bless, Heroism, Protection from Magic, Fly, Feather Fall and the combat buffs all go through it. | vanilla |
| C15 | Light Bolt no double damage to Undead | MM7 and MM8 hook only, MM6 has no Light Bolt. | n/a |
| C16 | Armageddon unresistable | The MM6 default `ArmageddonElement` equals the vanilla byte, the hook changes nothing in MM6. Ours refuses the player cast and no-ops the event cast. | n/a |
| C17 | Negative resistance divided by zero (`NegativeResHook`, `MMHooks.pas:3039`) | `mm6::resistedDamage` guards `divider > 0`. Unit test `MM6CombatRules.ResistanceRollRunsWithoutLuckOrResistance`. | fixed |
| C18 | Turn-based turn took twice as long (`Hooks.pas:2979`, 426 to 213 ticks) | Turn engine keeps 426, the vanilla value, by decision (commit `3871e33e1`), pinned by `MM6TurnBased.PartyAndMonstersTakeTurns`. | vanilla, deliberate |
| C19 | Starburst and Meteor Shower hit beyond arrow range (`FixStarburst=1`, `Hooks.pas:1048`) | Player casts of 9 and 22 refused. | absent |
| C20 | Blasters, Acid Burst, Blades, Flying Fist, Toxic Cloud and Shrapmetal couldn't target rats, launch at height/2 while aim used height/3 (`FixDragonTargeting`, `Hooks.pas:2562`) | MM6 attack spells and blasters launch from party height/3 (`CastSpellInfo.cpp`), aim uses /3 (`Actor.cpp`). Consistent, nothing checks it against a rat. | fixed by consistency |
| C21 | Party attacked monsters in their dying animation (`CanAttackMon`, `MMHooks.pas:2941`) | Melee falls back unless `CanBeDamaged`, `FindClosestActor` filters Dying, `DamageMonsterFromParty` returns on `IsNotAlive`. | fixed, no MM6 test |
| C22 | Shield and armor recovery penalties (Mok's patch) | MM7 multiplier table in `Character.cpp:1864`. MM6 `skilldes.txt` says armor Expert reduces and Master eliminates the penalty, our table keeps half for Plate at Master. Mok's actual change not traced. | mm7, open |
| C23 | Blaster recovery floor (`BlasterRecovery=4`, game default 0) | `gameplay.minimum_recovery_blasters` default 5. | fixed, QoL |
| C24 | Monsters jumped off bridges into lava (`MonsterJumpDownLimit=500`, `Hooks.pas:2522`) | No drop-height limit in indoor or outdoor actor movement. | vanilla |
| C25 | Random item generation favoured the first candidate and starved the last (`FixItemGeneration`, `MMHooks.pas:3705`, one-off in the cumulative compare) | `MM6ItemGenerator.h:96` keeps `cumulative >= roll`, pinned by `MM6ItemGenerator.FilteredKindDrawsOverTypeCandidatesOnly`. | vanilla, deliberate |
| C26 | Clubs had no sound and no delay | MM7 and MM8 hooks only. Our club delay is MM6's native 100 ticks. The attack-sound switch has no club case, so a club swings silently unless `treat_club_as_mace`. | n/a, silent swing open |
| C27 | A hit that Endurance reduced to zero recovery still switched the active character (`FixZeroRecovery`, `MMHooks.pas:3001`) | `SetRecoveryTime` returns early at zero. | fixed |
| C28 | Quick Spell mana check used the wrong school (`FixQSpellPointsCheck`, `MMHooks.pas:3024`) | `KeyboardInputHandler.cpp` gates on the spell's own school and the MM6 table. | fixed |
| C29 | Progressive dagger triple damage, 1% per skill point (`ProgressiveDaggerTrippleDamage=1`) | `Character.cpp:1098` triples at `random(100) < level`. MM7 test `Issues.Issue1721`. | fixed |

### World, events, timers, travel, saves and UI

| # | GrayFace fix (patch default) | Ours | State |
| --- | --- | --- | --- |
| W1 | Event timer countdown reset on every visit, and a late revisit fired a periodic timer many times, so wells refilled dozens of uses (`FixTimers=1`, `Hooks.pas:2336-2445`, stores the last firing per period kind and triggers once per period) | `MM6MapVisit.cpp` sets each `OnTimer` alarm to now plus interval on every map load and never stores the last firing, so the countdown resets like vanilla. `Processor.cpp:205-220` fires once and moves the alarm past now, so there is no catch-up burst. Daily timers wait for the next midnight. Tests `MM6MapVisit.FountainRefillsADayAfterTheLastVisit` and `TimerWithoutIntervalWaitsFromTheNextMidnight`. | half: reset is vanilla, burst is fixed |
| W2 | With nobody active, a fountain event ran once per member and could be drunk four times (`InactiveMemberEventsHook`, `Hooks.pas:464`, picks one living member for the whole event) | `EvtInterpreter.cpp:249` sets `CHOOSE_RANDOM`, and `iterateCharacters` re-rolls `rng->random(4)` for **every command**, so the compare and the set can hit different members. | vanilla, and a different shape of it |
| W3 | Inactive members couldn't open chests (vanilla), could attack and could use scrolls (`FixInactivePlayersActing=1`, `FixInfiniteScrolls=1`) | Chest screen lets any member be selected and `Chest.cpp` needs an active member to open, attacking is refused for incapacitated members (`Engine.cpp:1526`, `UIGame.cpp:582`, `Character.cpp:4588`). MM6 scrolls throw. | fixed, scrolls absent |
| W4 | New Day effects skipped at month start after "Wait until Dawn" then Esc (`MMHooks.pas:4092`) | Esc calls `Rest(remaining)` and the shared 3 AM new-day check in `Engine.cpp:1356` has no month special case. | fixed |
| W5 | Full outdoor brightness for a minute at 5:00 AM (`FixBright5AM`, `MMHooks.pas:3632`, feeds the current minute into the lighting update) | `Outdoor.cpp` fog and sun dimming are continuous in the hour, no MM6 path. Not watched on screen. | fixed on paper |
| W6 | Waiting in the Rest dialog didn't tick character recovery (`FixWaitHook`, `Hooks.pas:1592`) | `Engine.cpp:1441` runs `Character::Recover` inside `Rest`. Test `MM6FirstCombat.WaitAnHourNeitherEatsNorHeals` pins HP and food, not recovery. | fixed |
| W7 | Coach and boat fare was withdrawn before the autosave, so the arrival autosave already lacked the gold (`TravelGoldFixHook`, `Hooks.pas:1623`, subtracts after `DoSaveGame`) | `MM6Transport.cpp:79-81` takes gold and rests, `depart()` autosaves afterwards through `Engine::autoSave`. Test `MM6Transport` checks the fare, not the order. | vanilla |
| W8 | Horseman and boatman speech lengths detected automatically | Fixed 1500 ms coach and 2500 ms boat delays, the vanilla constants. | vanilla constants |
| W9 | Map-crossing dialog offered while flying low, accepting did nothing (`TravelWalkTriggerFixHook`, `Hooks.pas:1639`) | `MM6OutdoorBorderEligibility.h` offers on flags only, `MM6OutdoorBorderConfirmation.h:82` refuses while flying, `Game.cpp:783` clamps. Unit test `ConfirmationChecksFlyingSeparately` pins this split. | vanilla, deliberate |
| W10 | Lloyd's Beacon autosaved after taking SP and recovery, Town Portal autosaved after recovery and even within the same map, Town Portal wasted the turn on cancel, Town Portal and Enchant Item didn't pause (`Hooks.pas:1705-1750`, `2461-2508`, `TPFixHook`) | Spells 29, 31 and 33 are not admitted, refused in `MM6SpellEffectGuard.h` and `Game.cpp:140`. The shared books keep the vanilla order and MM7 town data. | absent, decide at admission |
| W11 | Ground items from one map appeared on another after walking across a border (`FixItemDuplicates=1`) | `Outdoor.cpp:545` and `MM6IndoorSnapshot.cpp:490` rebuild sprite objects from the map's own data, `MM6MapTransition.cpp:139` restores only that map's items. | fixed by design |
| W12 | On respawn, artifacts generated in never-opened chests must be cleared from the found list, and the exclusive-artifact generator was broken and didn't mark artifacts found (`FixUnmarkedArtifacts=1`, `MMHooks.pas:1370-1560`, plus the `RefundChestArtifacts` opcode in scripts) | Respawn at `Outdoor.cpp:511` never touches `generatedArtifactFlags`, nothing clears them. The live direct-artifact picker is our own safe policy (`MM6ChestDirectArtifactPicker.h`), marks generated and caps at 13, with the broken original kept detached for tests. | half: clearing is vanilla, picker is fixed |
| W13 | Items that didn't fit a chest reappear when space is freed (`FixChests=0`, off), most valuable items placed first (`FixChestsByReorder=1`), compact stacking (`FixChestsByCompacting=1`), vertical stacking (`PlaceChestItemsVertically=1`) | `MM6ChestPlacement.h` places once in random order, items that don't fit keep a slot with no cell and are never shown, no retry, reorder or compaction. Putting an item into an MM6 chest throws. Unit test `PlacementKeepsNoFitItemWithoutExtraWandDraw`. | vanilla, put-in absent |
| W14 | Event commands couldn't operate on some skills, Thievery included (four range bytes in `Hooks.pas`) | `MM6SkillEvent.cpp` accepts all 31 slots 56..86 for Compare and Set, Add and Subtract throw. Tests `MM6SkillEvent_ut.cpp`, `MM6SkillEvent` game suite. | fixed range |
| W15 | Door state command accepts 3 to switch a moving door (`DoorStateSwitchHook`, `Hooks.pas:1213`) | `Indoor.cpp:786-834` ignores action 2 while moving, action 3 matches nothing. Only the patched `cd2.evt` could need it. | vanilla, modder feature |
| W16 | Save slots couldn't have gaps and save and load targeted different slots, Quick Save broke in the Hive after the Reactor | New saves take the first free `saveNNN.ec6`, the menu is a file list, Quick Save has no Hive check, only autosave is skipped during the Hive finale. | fixed by the list design |
| W17 | Awards page not refreshed on Tab (MM6 and MM7 hook) | `UICharacter.cpp:971` refills awards when the active member changes. Arrow keys unbound. | fixed |
| W18 | Buff duration shown wrong for "1 day 5 minutes" (`DoFixBuffTime`, `MMHooks.pas:3726`, shows the two largest units) | `GUIWindow.cpp:272-307` drops minutes when days are present. Matches the patch's two-unit rule for that case, the exact vanilla mistake isn't recorded. | fixed on paper |
| W19 | Condition priority order for display and stats (`FixConditionPriorities=1`, `MMHooks.pas:2080`), stats screen shows the condition that sets the stats | Display order is the GrayFace order behind `gameplay.alternative_condition_priorities` (default true). Stats use the original MM6 order in `MM6ActualStatCondition.h`. The MM6 stats screen shows the display condition, not the stat-setting one. | display fixed, stats screen differs |
| W20 | Depleted wands break instead of vanishing (`KeepEmptyWands=1`) | MM6 wand fire throws. `gameplay.destroy_discharged_wands=false` keeps an empty wand. | absent, config ready |
| W21 | Some scrolls were endless on the paper doll in turn-based mode, right-click scroll use auto-picked the target (`Hooks.pas:675-740`) | MM6 scrolls throw. | absent |
| W22 | Black potion with no effect was not consumed (`DontWasteBlackPotion`, `Hooks.pas:2288`) | `MM6Potion.h:38` ignores a repeated Essence but reports success, `Character.cpp:3397` removes it. | vanilla |
| W23 | Circus removed all trophies, Cloak of Baa given on every repeat talk | Both are script bugs in `global.evt`, fixed by the patched script (data inventory). Our engine runs the shipped script faithfully. `MM6CampaignChainC4` grants the cloak once and never repeats the talk. | vanilla by data |
| W24 | SFT and TFT frames: first frame too long, last too short (`FixSFT=1`) | `Sprites.cpp:175` and `TextureFrameTable.cpp:34` use `t >= frameLength`, correct boundaries, no MM6 path. | fixed |
| W25 | Movement rounding, jump height, strafe precision (`FixMovement=1`) | Float physics shared with MM7, nothing to round. | n/a |
| W26 | Shops bought blasters (`CanSellItemHook`, `Hooks.pas:2632`) | `MM6ShopPrices.cpp:117` lets weapon shops take any single-handed through bow type, blasters 64 and 65 are single-handed and not excluded. | vanilla |
| W27 | Buy dialog read out of bounds with no active member | `MM6Shop.cpp:141` returns without an active member. | fixed |
| W28 | Unconscious members could identify and repair in shops (`FixDeadPlayerIdentifyItem=1`, `MMHooks.pas:3062`, hooked at MM6 0x41C58E) | The patch hook sits in the item info box (0x41C440), where the selected member's Identify Item and Repair skills act on a right-clicked item. It treats a member who can't act (0x4876E0) as no member. Paid shop identify and repair were never broken in MM6.exe: the shop click handler 0x4A4CA0 refuses a member who can't act with "%s is in no condition to %s" (427, 562) and sound 27, and every shop screen draws that line in the right panel. Our port had dropped that check, so `MM6Shop.cpp` now refuses the same way, unconditionally since it is vanilla. Tests `MM6Houses.WeaponShopIdentifyUnconscious` and `WeaponShopRepairUnconscious`. Our MM6 shops show no item info box on right-click, and the map's inventory and chest info boxes already refuse a member who can't act (`UIPopup.cpp`), so the skill path has no hole yet. Porting the MM6 shop item info box must bring the patch's check with it. | fixed, paid path vanilla |
| W29 | Free Haven Smuggler's Guild added to the map | House 151 exists in `2devents.txt` and `MM6HouseServiceData.cpp:203` gives it skills, but the GOG `Outc2.odm` has no door to it. The patched `Outc2.odm` and `outc2.evt` add the entrance. | vanilla by data |

### Owner decision, 2026-11-15: fix them all

The owner decided that MM6 fixes every bug in the tables above, the way GrayFace did or a better way where one exists. This replaces the earlier "decide per item" stance. It also reverses the four deliberate-vanilla choices (C12, C18, C25, W9), so their pinning tests flip with the fix. No code has changed yet. The MM6 playtest board keeps its priority over this backlog, and each fix lands as its own reviewed packet with a game test that reproduces the player-visible symptom.

Rules for every fix in this backlog:
- A fix that changes game logic sits behind a `gameplay.fix_*` config flag that defaults on, like `fix_water_walk_mana_drain`. GrayFace makes most fixes switchable through `mm6.ini`, and the flag keeps a vanilla run one switch away.
- A fix that needs new saved state stores it in our own save fields, never in a layout the original MM6 can't read back. GrayFace keeps its timer state in a patch-only map extra for the same reason.
- An admission-time item is fixed in the packet that admits the spell or feature. The spell isn't admitted with the vanilla bug and fixed later.

| # | Fix | Approach | Why this way |
| --- | --- | --- | --- |
| C14 | Stronger but shorter buff | Accept the new buff when it is longer **or** stronger, as GrayFace does. | Same as GrayFace. |
| C24 | Monsters jump into lava | Refuse a walking step down more than 500 units for a non-flying monster, the GrayFace limit. | Same as GrayFace. Our actor movement has no drop check at all, so this is new code in both indoor and outdoor movement. |
| W1 | Timer countdown resets on every visit | Store each map's last periodic firing per period kind and schedule from it, with GrayFace's daily-at-fixed-time rule. | Same as GrayFace, stored in our save fields. Keeps the catch-up guard we already have. |
| W2 | Random member per command when nobody is active | Pick one conscious member once per event run and keep it for every command, falling back to member 1. | Same as GrayFace. Our current per-command re-roll is worse than vanilla, the compare and the set can hit different members. |
| W7 | Fare taken before the travel autosave | Autosave before the fare and the travel days are applied. | Better than GrayFace, which only moves the gold. Ours also applies the travel days before the save, so the save should come first. |
| W8 | Fixed coach and boat speech delay | Wait for the actual speech sample to finish, capped by the vanilla 1500 and 2500 ms. | Same as GrayFace. |
| W12 | Found-artifact flags never cleared on respawn | On respawn, clear the found flag for every artifact generated in a chest that was never opened, and implement opcode 69 `RefundChestArtifacts`. | Same as GrayFace. Opcode 69 also unblocks the patched scripts. |
| W13 | Chest overflow items hidden forever | At generation, place items most valuable first and compact them, then place leftovers when the chest is reopened with space. | GrayFace ships reorder and compacting on and leftover placement off. We turn all three on, because a hidden item is lost loot for the player. |
| W15 | Door action 3 ignored | Accept action 3 as "switch even while moving". | Same as GrayFace, cheap and needed by anything that runs patched scripts. |
| W22 | Black potion wasted | Leave the potion in the inventory when it has no effect. | Same as GrayFace. |
| W26 | Shops buy blasters | Exclude blasters from every shop's trade list. | Same as GrayFace. |
| W28 | Unconscious members identify and repair | Require the selected member to be able to act before identify or repair in a shop, and on the map. | Same as GrayFace. Done 2026-11-18 for the paid shop path, which MM6.exe already refused, so it has no config flag. The skill path is unreachable today, see the W28 row above. |
| C12 | Static Charge cost from the exe table | Read Normal, Expert and Master costs from `spells.txt`, as GrayFace does. | Same as GrayFace. Flips the exe-table decision in `MM6_F4_SPELL_MANA_RECOVERY.md`. |
| C18 | Turn-based turn twice as long | Advance 213 ticks per turn instead of 426. | Same as GrayFace. Flips `MM6TurnBased.PartyAndMonstersTakeTurns`. |
| C25 | Item roll favours the first candidate | Fix the off-by-one in the cumulative compare so every candidate gets its weight. | Same as GrayFace. Flips the two `MM6ItemGenerator` pins. |
| W9 | Border dialog offered while flying low | Don't offer the map-crossing dialog while flying. | Same as GrayFace. Flips `ConfirmationChecksFlyingSeparately`. |
| C6 | Recovery enchant at the MM7 +50% | +10% recovery rate for MM6, the GrayFace default. | Same as GrayFace. MM6 vanilla did nothing, so 50% would be a new rule rather than a fix. |
| C8 | Weapon-skill hirelings add a hidden skill | Apply the +2 and +3 to the weapon skills, as `npcprof.txt` describes, for delay and for whatever vanilla already applied it to. Remove the MM7 Armsmaster route for MM6. | Better than GrayFace, which only adds the bonus to delay. Trace what vanilla applies the bonus to before writing it. |
| C22 | Armor recovery uses the MM7 table | Trace Mok's original change and MM6's skill text, then make Master armor eliminate the penalty if both agree. | Mok's fix isn't in the GrayFace source, so the target needs tracing first. |
| C26 | Clubs swing silently | Play the mace swing sound for clubs. | Not an MM6 GrayFace item, but the same bug GrayFace fixed for MM7 and MM8. |
| W19 | Stats screen shows the display condition | Show the condition that determines the stats on the stats screen, the most severe one everywhere else. | Same as GrayFace 2.5.2. |
| W5, W18 | 5:00 AM brightness, buff duration text | Pin with a game test, fix only if the test shows the vanilla bug. | Both look fixed in code but nobody has watched them. |
| C3, C7, C21, C27, W4, W6, W17, W24 | Fixed in shared code, no MM6 test | Add an MM6 game test for each. | A shared-code regression would otherwise pass unnoticed for MM6. |

**Admission-time fixes.** C2 monster Paralyze applies the Paralyzed condition to one chosen member. C4 elemental weapon enchants go through the monster resistance roll. C9 Finger of Death awards experience. C11 Charm uses the corrected Master duration. C19 Starburst and Meteor Shower refuse targets beyond arrow range. W10 Lloyd's Beacon and Town Portal autosave before taking spell points and recovery, Town Portal doesn't autosave within the same map and doesn't waste the turn on cancel, and Town Portal and Enchant Item pause the game. W20 an empty wand stays in the inventory. W21 scrolls don't repeat on the paper doll in turn-based mode, and a right-click scroll asks for its target.

**Data fixes.** Better than GrayFace's override archives, in two parts:
- The 34 script fixes become an engine-side patch table. Each entry names the original file's hash and the record-level edit from the diffs in the decoded folder, so the engine applies it only to the exact GOG 1.2 file and nothing derived from the game's data is shipped. Opcode 69 from W12 is a prerequisite. The three text tables are typo fixes and go in the same table.
- The 21 map geometry fixes can't be expressed as small edits. The engine loads `data/*.games.lod` override archives from the owner's data folder, the GrayFace custom-archive convention, so the owner can drop in the archive from the official GrayFace download. W29, the Smuggler's Guild door, arrives this way.


## Data fixes, inventory

Record-level diffs against the owner's GOG 1.2 files. Opcode numbers are MM6 event opcodes, `op 4` is `MouseOver` (the hint shown for a map entrance), `op 6` is `MoveToMap`, `op 7` is `OpenChest`, `op 14` is `Compare`, `op 15` is `ChangeDoorState`, `op 69` is the patch's `RefundChestArtifacts`. Names follow `src/Engine/Evt/EvtEnums.h`.

### Scripts (`00 patch.icons.lod`)

| File | Map | Change | Readme entry |
| --- | --- | --- | --- |
| `oute3.evt` | New Sorpigal | `MouseOver 0` added for events 101 to 103 | Missing dungeon/house hints in all outdoor locations |
| `outd3.evt` | Castle Ironfist | `MouseOver 0` added for events 90 to 94 | same |
| `oute1.evt` Eel Infested Waters, `oute2.evt` Misty Islands, `outd1.evt` Silver Cove, `outd2.evt` Bootleg Bay, `outc1.evt` Frozen Highlands, `outb3.evt` Dragonsand | | one `MouseOver 0` added | same |
| `outa1.evt` | Sweet Water | `MouseOver 0` added for events 90 and 205 | same |
| `outa3.evt` | Hermit's Isle | `MouseOver 0` added for event 90 | same |
| `outc2.evt` | Free Haven | `MouseOver 0` added for events 150 to 153 | same, and the four new entrances belong to the added Smuggler's Guild in `Outc2.odm` |
| `outc3.evt` | Mire of the Damned | hint and `SpeakInHouse` moved from event 68 to 69, two hints added | Duplicated house, hints |
| `outa2.evt` | Paradise Valley | house event moved from 65 to 66, fountain `Cmp` branch target 6 to 7 | Fountain showed "+100 Power temporary" instead of "Refreshing!" when depleted, duplicated house |
| `outb1.evt` | Kriegspire | Superior Temple of Baa `MoveToMap` direction, four hints added, fountain flag variable `0x6d5` to `0x8d5` | Superior Temple entered in the wrong direction, Blackshire and Kriegspire fountains shared one "used" flag |
| `outb2.evt` | Blackshire | event 73 (statuettes quest) rewritten with `RefundChestArtifacts`, event 68 extended with the refund, Lair of the Wolf `MoveToMap` direction, two hints | Statuettes reward repeatable after refill, artifacts in inaccessible chests, Lair of the Wolf direction |
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

Changed geometry or placement, no record-level diff: `CD2.blv`, `CD3.blv`, `D04.blv`, `d08.blv`, `D09.blv`, `D10.blv`, `d16.blv`, `oracle.blv`, `sci-fi.blv`, `sewer.blv`, `t1.blv`, `T7.blv`, `znwc.blv`, `Outb1.odm`, `Outb3.odm`, `Outc2.odm`, `Outc3.odm`, `Outd2.odm`, `outa2.odm`, `outd1.odm`, `outd3.odm`. Byte-identical to GOG: `D04.dlv`, `d14.blv`, `d14.dlv`, `Outb2.odm`, `Outc1.odm`. The readme explains each: duplicated chests and houses, Dragonsand's two Tomb of VARN doors sharing a texture, Snergle's cabinet front not clickable, sewer rats spawning inside walls, the Free Haven Smuggler's Guild building, the Bootleg Bay circus entrance texture, Temple of Baa floor alignment, the New World Computing door moving a wall.

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
| `SupportTrueColor`, `BorderlessFullscreen`, `WindowWidth/Height`, `Stretch*`, `CompatibleMovieRender`, `SmoothMovieScaling`, `RenderMax*`, `Scaling*`, `MouseLookCursorHD` | engine renderer | not applicable |
| `DataFiles`, custom `Data\*.lod` archives, `ExeMods`, MMExtension loading | none | the custom-archive convention is the one piece worth copying, see the data-fixes decision |
| `NoCD`, MSS32 and Smack replacements, Delphi memory manager, registry key buffer, XP compatibility, exit crash, window activation sound stop | engine | not applicable |
| `FixSFT=1`, `TFT` timing, `SpriteAngleCompensation=1`, `ClickThroughEffects=1`, infinite dungeon view distance, minimap zoom memory, `MonSpritesSizeMul`, `WinScreenDelay=500` | renderer and UI | W24 covers SFT and TFT timing, the rest is presentation |

## How a lane uses this

- **Before claiming a vanilla MM6 behaviour**, grep the hook comments. If GrayFace has a hook for it, the vanilla game is buggy there and the decision is ours: reproduce, fix, or fix behind a config flag like `fix_water_walk_mana_drain`. Record the decision in the cross-reference table.
- **Before writing a chest, fountain, altar or quest-reward test for a map in the data inventory**, read that map's diff. The original script may be the bug, and a test that pins the original script's behaviour pins a bug.
- **When the owner reports a map problem** from the readme's "Fixes in maps" lists, the fix is data, not engine. Options are a local override archive on the owner's profile or an engine-side script patch, see the data-fixes decision above.

See also: [MM6 Implementation Checklist](MM6_IMPLEMENTATION_CHECKLIST.md), [MM6 Spell Effect Dispatch](MM6_F4_SPELL_EFFECT_DISPATCH.md), [MM6 Event Campaign Spec](MM6_EVENT_CAMPAIGN_SPEC.md), [Source and Privacy Boundary](../.llm-wiki/Source-and-Privacy-Boundary.md).
