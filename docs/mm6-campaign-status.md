# MM6 campaign status

Where Might and Magic VI stands on `claude/mm6` (2026-11-20): what a player can do, how it is verified,
the campaign chain, GrayFace fix progress and the latest lane results.

## Summary

MM6 is a playtest candidate, not a released or campaign-complete title. The `ECv0.1.0` release ships an MM6 helper
behind the Atlas **Play MM6** button for owner playtests only. The planned MM6 release is `ECv0.2.0`, date undecided.
Since 2026-11-18 finished MM6 lanes merge into the long-lived `claude/mm6` branch, not `master`, until that tag.

The owner's delivery order puts MM6 working second, after MM7 working (closed 2026-11-11). "Working" means a separate
MM6 runtime and profile support New Game or Load, visible play, travel and main campaign progression through an ending
with durable saves, while MM7 keeps working. Headless tests alone never close that gate.

## What a player can do today

Every item below has input-driven game tests against the owner's real MM6 data. Suites named `...Headless` and
`DISABLED_` cases run in fresh processes through the gate runners.

- **New Game.** Party creation by input (portraits, voices, names, classes, skills, attributes) and production New
  Game into New Sorpigal. `MM6PartyCreation.*`, `MM6Creation.*`, `MM6NewGame.*`.
- **HUD and books.** MM6 HUD, character screen and paperdoll, spellbook, quest, autonotes, map and calendar books,
  awards. `MM6Hud.*`, `MM6BooksHeadless.*`, `MM6CharacterScreenHeadless.*`.
- **Houses.** Every New Sorpigal house kind opens with its MM6 layout. House exits respect their quest bits.
  `MM6Houses.*`, `MM6HouseExit.*`.
- **Shops.** Buy, sell, identify, repair and special stock. A character who can't act is refused, and so is a
  zero-value item. `MM6Houses.WeaponShop*`, `MM6Houses.GeneralStore*`.
- **Services.** Temple heal and donate, training, spell and skill guilds with membership, expert and master teachers,
  bank deposit and withdraw, tavern rent, fill packs, drink and tip, the town hall Bounty Hunt. One `MM6Houses` test
  per service.
- **NPCs.** Street NPC talk with weekday, news and Join topics, Beg, Threat and Bribe, hiring two followers, house
  NPC topics. `MM6NpcDialogue.*`, `MM6StreetNpcs.*`, `MM6Hirelings.*`.
- **Combat.** Melee, bows (Percival arrows explode), admitted player spells, monster missiles, spells and special
  attacks, trap and event damage, turn-based mode, potions, rest and wait. `MM6Melee.*`, `MM6Bow.*`,
  `MM6PlayerSpells.*`, `MM6TurnBased.*`, `MM6Potion.*`, `MM6Rest.*`, `MM6CombatHeadless.*`.
- **Death.** A party that dies in a dungeon or outdoors wakes in New Sorpigal by MM6 rules. `MM6PartyDeath.*`.
- **Dungeons.** Goblinwatch (d01), the Abandoned Temple (d02) and Gharik's Forge (d18): doors, switches, chests, traps
  and wall plates. `MM6Dungeons.*`, `MM6WallPlate.*`, `MM6Chest*`.
- **Travel.** Walking off a map edge, dungeon doors and back, stable and boat routes including the price-fixing quest,
  map state kept across a return, map timers that count while the party is away. `MM6MapTravel.*`, `MM6Transport.*`,
  `MM6MapVisit.*`.
- **Save and load.** Save on any map in the EC6 container, autosave on map change, quick save, in-game Load and load
  in a fresh process, with artifacts, relics and specially enchanted items carried or worn. `MM6InGameSave.*`,
  `MM6InGameLoad.*`, `MM6SaveAnywhereHeadless.*`, `MM6AutosaveHeadless.*`, `MM6ItemSaveHeadless.*`.
- **Finale.** The hive is won with the ritual and lost on the scripted failure paths. `MM6Finale.*`.

Spells MM6 has not admitted refuse with a status line instead of running inherited MM7 code. The admission list is
[MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h). Town Portal, Lloyd's Beacon, Stun, Shield, scrolls
and wands are not admitted. Rules come from the original `MM6.exe` in the data folder through `mm6::Executable`, see
[MM6 executable rules](mm6-executable-rules.md).

## How it is verified

| Method | What it proves | Where |
| --- | --- | --- |
| Game tests | Input-driven scenarios against the real data, observed through tapes. Each fix is reverted once to see its test fail. | `GameTests_MM6*.cpp`, `--gtest_filter='MM6*'` |
| Unit tests | Parsers, rule tables, save codecs. Data-backed cases read the install named by `OPENENROTH_MM6_PATH` and skip without it. | `MM6*_ut.cpp`, `test/Testing/Unit/MM6TestData.*` |
| MM7 controls | A shared-code change keeps MM7 working | `Prs.Pr2354`, `Prs.Pr2956`, `Issues.Issue840` plus the MM7 tests on the same path |
| First-map gate | New Game, save, fresh-process load, autosave, quick save and buffs across processes, each child in its own temporary profile | `tools/run_mm6_firstmap_gate.py` |
| Sweeps | Every MM6 map loads and runs its on-load events with actors. Monster combat on main-quest maps. | `tools/run_mm6_map_sweep.py`, `tools/run_mm6_combat_sweep.py` |
| Campaign chain | Each chapter loads the previous chapter's save in a fresh process | `tools/run_mm7_campaign_chain.py --title mm6` |
| Hidden captures | Before and after frames of each gameplay lane, rendered offscreen and inspected whole | temporary `MM6OffscreenFrame.DISABLED_*` tests, PNGs in the ignored `local-game-data/mm6/` |
| Owner playtests | Visible play of the installed helper | 2026-11-15 short playtest, 2026-11-17 playtest and tour |

The last map sweep on record loaded all 67 MM6 maps and ran every phase on all 26 main-quest maps without a crash.
`check_tidy` needs clang-tidy 22, which the owner's Mac lacks, so MM6 lanes run `check_style` only.

## Working-gate rows

| Row | What closes it | State |
| --- | --- | --- |
| M1 | Production New Game, Save, fresh-process Load | Done (first-map gate) |
| M2 | Live travel, Save on any map, fresh-process Load restoring chests, dead monsters and doors on active and parked maps | Done headless |
| M3 | Party death and respawn, in-game Load, autosave and quick save | Done headless |
| M4 | Main-quest chain from New Game to the finale with fresh-process checkpoints | C0 to C8 merged, C9 passes on the real C8 save |
| M5 | Owner sessions: the first hours, travel and a dungeon, chain checkpoints and the ending | Partial, New Sorpigal playtests only |
| M6 | The Atlas installs a sibling MM6 helper with its own profile, MM7 untouched | Helper ships in `ECv0.1.0` on `Profiles/MM6-Atlas` |
| M7 | MM7 stays working after every shared-core merge | Continuous |
| M8 | Player-blocking defects from M1 to M7 fixed | Ongoing |

## Campaign chain

The main quest spans 26 maps plus `global.evt`. The chain splits it into chapters C0 to C9. Each chapter is a
`MM6CampaignChain.DISABLED_ChapterN` game test that loads the previous chapter's `saveNNN.ec6` in a fresh process,
plays its part by input and writes the next save. Fixtures are named per chapter: C1's party strength and teleports
in front of doors, crystals and exits.

| Chapter | Content | State |
| --- | --- | --- |
| C0 | New Game and the opening | Merged |
| C1 | Councils A and C, stable pricing, a chest pickup | Merged |
| C2 | Council B | Merged |
| C3 | Councils D and E | Merged |
| C4 | Council F and the traitor | Merged |
| C5 | Oracle admission and the t6 crystal | Merged |
| C6 | The three castle crystals (Alamos, Darkmoor, Kriegspire) and the Oracle hand-in | Merged |
| C7 | The Tomb of VARN and the Control Cube | Merged-ready, passes on the real C6 save, see below |
| C8 | The Control Center's blasters and the NPC 12 release, through the room chain between houses 168, 553 and 554 | Passes on the real C7 save, see below |
| C9 | The hive: the sanctum key, the Demon Queen, the reactor, the exit obelisk and the winning ending | Passes on the real C8 save, see below |

The chapter helpers live in [MM6CampaignChain.h](../test/Bin/GameTest/MM6CampaignChain.h). Private event traces for
each chapter stay in the ignored `local-game-data/private-wiki/mm6/`.

### C7 route

C7 passes on the real C6 save, run through C0 to C6 in fresh processes, with no fixture beyond C1's party strength.

- Starts on `outc2`. The Oracle's screen (`oracle.blv` door 2) is already raised from C5, and her floor plate
  (event 2) toggles it, so the party walks up without the plate. Face event 3, NPC 8, topic 76: award 33, quest bit
  166, bits 162 to 165 cleared. Out through event 50.
- `outb3` event 90 leads into `pyramid.blv`. The keyed doors, chests and pools check the active character's items,
  and the first character carries every key and scroll, so the chapter selects it with the 1 key before each check
  and tries a check again when a monster's hit moved the active character off it.
- Switch event 29 is a mouse click, Space from in front of it finds nothing. It opens doors 37 and 38 over chest 44
  (keys 574, 537, bit 231). Flame Door event 31 takes 574. Chest 42 holds 573 and 538 (bit 229), the Water Temple door
  (event 11) takes 573. Chest 45 holds 576 and 539 (bit 232). Chest 43 takes 576 and holds 577 and 542 (bit 230).
  Bookshelves 26 and 27 give 540 (bit 234) and 541 (bit 233), each scroll comes in hand.
- Each code is spoken holding its scroll: pools 21 to 25 take `kcops`, `ulus`, `aruhu`, `yttocs` and `yoccm`, and
  clear bits 229, 232, 234, 233 and 230. The Well of VARN (event 1) takes `krik`, clears 231 and shows "The waters
  part.". The well is two pressure plates side by side, faces 226 and 3293, and
  `walkOntoMM6CampaignChainIndoorEventPlate` stops on either.
- Control Room Entry event 35, then chest 41 takes 577 and gives the Control Cube 456 (bits 42 and 195). Out through
  event 17. The Oracle's topic 76 takes the cube for award 34 and 500,000 experience each, topic 77 sets bits 169 and
  235 and moves Nicolai (NPC 13) from topic 25 to 26.

The guardians at switch 29, a Djinn and two Genies, are not fought. The party works the switch with them next to it,
takes the chest and moves on, and the chapter's experience is exactly the Oracle's 2,000,000. Fighting isn't a way
past them for C1's party, whose weapons are at skill 1. Its hits on VARN's guardians land for 2 to 18 after the
physical resistance roll, and each character attacks about once every 180 recovery ticks. Their hits stop a route that
doesn't allow for them, in two ways seen in runs on the real C6 save:

- A hit leaves the character a short recovery, and a hit on the active character moves the active character to the
  next one. A check made right then reads the wrong backpack. In one run every keyed check passed up to pool 25, where
  a hit during the walk onto the plate left the pool silent. A player steps onto the plate again with the carrier
  selected, and so does the chapter.
- The Djinn's special attack, a 44% roll per hit at its level 44 saved against Endurance as
  [MM6 combat and monsters](mm6-combat-and-monsters.md) describes, put the first character to sleep within 30 frames
  of the switch in a run that stood there longer. A sleeping character can't be selected, so the Flame Door answered
  "The door is locked". The chapter leaves the switch as soon as the cover is open and the carrier isn't slept on the
  real C6 save. If a later change to the RNG stream lets the sleep land, C4's fixture, no damage inside the dungeon,
  also blocks special attacks on all three monster hit paths.

The guardians' actor indices differ between runs that reach the tomb by different routes, so C7 doesn't name them.

### C8 route

C8 passes on the real C7 save, run through C0 to C7 in fresh processes, with no fixture beyond C1's party strength. It
also takes the Control Center's blasters, which no earlier chapter took and the hive's reactor needs, as it takes no
physical damage.

- Starts on `outc2`. Down to the Oracle through the High Council (event 49), her screen still raised from C5, and
  through her way down to the Control Center (house 170, open since C7's Kreegan topic set bit 169).
- The terminal (event 61) teaches every character Blaster at 1 and sets bit 276. Storage containers 2 (event 31) and 1
  (event 30) hold two blasters and a blaster rifle, and one blaster. Back up through event 70 and out of the Oracle
  through event 50.
- A party that had been to the Control Center couldn't save. The PRKM and TIMR save sections refused `sci-fi.blv`, the
  one MM6 map with a hyphen. Fixed, with `MM6ControlCenter.SavesInTheControlCenter`.
- On `outd3`, Castle Ironfist's door (event 43) leads to the throne room, house 154. Nicolai (NPC 13), topic 26 The
  Third Eye: bit 96, his topic moves to 27 and the Oracle's second topic clears.
- The well by the castle (event 231) is a water face inside a rim. Space from beside the rim runs it, a mouse click
  needs the party within 200 units. With bit 96 it gives the Third Eye 446 and sets bits 305 and 196.
- Nicolai's topic 27 takes the Eye, gives the active character Tanir's Bell 461 and sets bit 197. The King's Library
  door (event 42) opens house 168, which with the bell carried plays `archie` once and becomes 553. Archibald (NPC 12),
  topic 30: the Ritual of the Void 544, bits 177 and 198 set, 197 and 235 cleared, 50,000 experience each, and he
  leaves, which empties the room to 554. The library door leads to 554 from then on.
- Items a script adds come in hand, and the chapter puts each into the first character's backpack.
- Reputation falls from 1600 to 672. The release's Subtract 1000 keeps only its low byte, 232, as MM6.exe does, and runs
  once for each character.
- Nicolai's circus quest (bits 90 and 95) is not on the way. His Third Eye topic sits in his second slot whatever his
  first holds.

C8 ends on `outd3` with the Ritual, the bell, three blasters, a blaster rifle and Blaster 1 on every character, the
state the hive's reactor and its exit obelisk need in C9. The blasters are in the first character's backpack, not
equipped.

With nine saves the chain's save helper scrolled the Save menu one slot short of the top and saved over
`save000.ec6`. It now clicks up until the new slot is selected, and checks that before it saves.

### C9 route

C9 passes on the real C8 save, with C1's party strength and no damage inside the hive, the C4 fixture. It ends the
chain with the game won and the party in New Sorpigal.

- Starts on `outd3`. The first character hands a blaster to each of the others through their portraits, each wears
  it from the backpack, and the first character wears the blaster rifle. Nobody wore anything before.
- `outa1` event 90 leads into `hive.blv`, unless quest bit 237 says the party already escaped.
- Chest 2 (event 42) holds the Hive Sanctum Key 570. The sanctum door (event 6, door 6) reads the active character's
  items, takes the key and opens, and says "The door is locked" otherwise.
- The Demon Queen (actor 1, monster 172) and the reactor (actor 0, monster 173) each have 1,300 hit points. Four
  blasters at Blaster 1 kill both. The queen sets bit 202. The reactor, with the Ritual carried, sets bit 180, summons
  16 hive guards and puts the party at the hive exit.
- The exit obelisk (event 60) needs bit 202. With the Ritual it sets bit 237, clears bit 198, takes the Ritual, gives
  award 36 "Destroyed the Hive and Saved Enroth" and opens the win screen. Without it the game is lost. The win screen
  writes `MM6_Win.Pcx`, the farewell box follows, and the party goes on at New Sorpigal's start.

Killing the queen also sets award 1, which MM6 calls "Returned the Prince", and a fine of 10,000, see
[open questions](open-questions.md). C9 doesn't tape award 1 or the fine.

## GrayFace fix progress

On 2026-11-15 the owner decided MM6 fixes every bug the GrayFace MM6 Patch 2.5.7 fixes, GrayFace's way or a better
one. A fix that changes game logic sits behind a `gameplay.fix_*` flag that defaults on. Item numbers below are the
ones in [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md).

Landed:

| Items | How |
| --- | --- |
| C3, C5, C7, C17, C20, C21, C27, C28, C29, W3, W6, W11, W14, W16, W17, W24, W27 | Already right in code. Those without an MM6 test got one in `GameTests_MM6GrayFaceShared.cpp`. |
| C10, C13, C14 | `fix_condition_spells`, `fix_water_walk_mana_drain`, `fix_stronger_shorter_buff` |
| C6, C18 | `fix_recovery_enchant`, `fix_turn_based_turn_time` |
| C8 | `fix_weapon_skill_hirelings` for the Expert recovery cut. The attack, damage and armor points and the dropped Armsmaster route follow MM6.exe, no flag. |
| C26 | No flag. MM6.exe plays the mace swing for clubs, the port was wrong. |
| C22 | No flag. MM6.exe, which already carries Mok's patch, halves armor and shield recovery penalties at Expert and drops them at Master, as `skilldes.txt` says. The port used MM7's table. |
| W1, W7, W8, W9 | `fix_map_timers`, `fix_travel_autosave`, `fix_travel_speech_delay`, `fix_flying_border_offer` |
| W2 | `fix_inactive_party_events` |
| W4, W5, W18 | `fix_new_day_at_month_start`, `fix_sunrise_light`, `fix_buff_duration_text` |
| W19 | `fix_stats_screen_condition`. The display order was already GrayFace's through `alternative_condition_priorities`. |
| W22, W26 | `fix_wasted_black_potion`, `fix_shops_buying_blasters` |
| W15 | `fix_door_state_switch`. No shipped script uses door action 3, so it is unit tested only. |
| C12 | `fix_spells_cost` |
| C4 | `fix_bonus_damage_resistance` for the relics Hades, Ares and Artemis. The elemental enchantments wait for their admission. |
| C25 | `fix_item_generation`. Shop stock and `GiveItem` kinds only, the any-item roll stays vanilla as in the patch. |
| W28 | `fix_incapacitated_skill_identify` for the skill half, in the item info box. The paid shop half has no flag, MM6.exe already refused it. |
| W12 | `fix_unopened_chest_artifacts` for the respawn refund. Event opcode 69 `RefundChestArtifacts` runs without a flag, no shipped script uses it. |
| W13 | `fix_chest_overflow`. Reorder by value and compaction, only for a chest that overflows, and what still has no room on a second page. Beyond GrayFace, whose reopen step pages replace. |
| None | `fix_valeria_shield`. Beyond GrayFace, the patch misses it: Valeria halves arrows from the hand, where MM6.exe looked in the armor slot. |
| None | `fix_expert_blaster`. Beyond GrayFace, the patch misses it: Expert blaster adds its skill to attack twice, as `skilldes.txt` says, where MM6.exe added it once. |

Open:

| Items | What is left |
| --- | --- |
| C24 | Doesn't reproduce. MM6.exe's own walking edge check, which we port, keeps chasing monsters on their ledges. Needs a vanilla case that falls. |
| C2, C4, C9, C11, C19, W10, W20, W21 | Fixed when the spell or feature is admitted, not before |
| Data fixes | Script fixes as an engine-side patch table keyed by file hash. Map fixes through `*.games.lod` override archives. |

## Latest lane results

Ten lanes merged into `claude/mm6` on 2026-11-20:

- **chain-c8.** C8 passes from the C7 save: Nicolai, the outd3 well's Third Eye, Tanir's Bell, the King's Library and
  Archibald's release, plus the Control Center's blasters the hive reactor needs. Saving after the Control Center was
  refused because `sci-fi.blv` has a hyphen, now fixed. The chain from C0 through C8 passed in fresh processes.
- **gf-c22.** Armor and shield recovery penalties follow MM6.exe, which already carries Mok's fix: half at Expert, none
  at Master. No flag.
- **gf-w12, gf-w13, gf-w15.** Artifacts of unopened chests come back after a respawn, event opcode 69 runs, door action
  3 switches a moving door, and an overflowing chest shows its most valuable items first.
- **chest-pages.** The owner chose pages over GrayFace's reopen step. An overflowing chest gets later pages behind a
  page button, kept in the record's spare grid cells.
- **all-artifacts.** The All Artifacts mod reaches MM6: no cap of 13, uniqueness by collected, an Atlas toggle and an
  `ARTC` save section. MM6 has no storage half.
- **artifact-saves.** A party carrying an artifact, a relic or a special bonus item can save. Commit `82abbc12a` carries
  the loot lane's message by mistake, its merge commit gives the right one.
- **mm6-loot.** Monster loot and random ground treasure roll through MM6's generator, so dragons drop artifacts, not
  message scrolls. Ground treasure now draws before chests, as in MM6.exe, which moved the chest rolls tests set.
- **artifact-bonuses.** Artifacts and relics give their attribute bonuses, and Merlin and Percival shorten recovery.
  The lane was stopped before its report when the owner ran short of budget, so only the integrator's final check
  covers it.
- **chest-fixes, not merged.** Chest gold piles, the chest right-click offset and Poison Resistance as Body. Its merge
  was reverted after `MM6ChestScreen.RightClickShowsTheItemUnderTheCursor` failed and the suite aborted on an
  inventory assert on top of chest pages. Its four commits are in `local-game-data/mm6/wip-patches-2026-11-20/`.

## Source

- [MM6Executable.h](../src/Engine/MM6/MM6Executable.h)
- [MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h)
- [GameConfig.h](../src/Application/GameConfig.h), the `gameplay.fix_*` flags
- [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp), [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp)
- [MM6CampaignChain.cpp](../test/Bin/GameTest/MM6CampaignChain.cpp),
  [GameTests_MM6CampaignChainC6.cpp](../test/Bin/GameTest/GameTests_MM6CampaignChainC6.cpp)
- [GameTests_MM6GrayFaceShared.cpp](../test/Bin/GameTest/GameTests_MM6GrayFaceShared.cpp),
  [GameTests_MM6Finale.cpp](../test/Bin/GameTest/GameTests_MM6Finale.cpp)
- [run_mm6_firstmap_gate.py](../tools/run_mm6_firstmap_gate.py), [run_mm6_map_sweep.py](../tools/run_mm6_map_sweep.py),
  [run_mm6_combat_sweep.py](../tools/run_mm6_combat_sweep.py), [run_mm7_campaign_chain.py](../tools/run_mm7_campaign_chain.py)

See also: [Open questions](open-questions.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md), [Testing](testing.md), [Releases](releases.md)
