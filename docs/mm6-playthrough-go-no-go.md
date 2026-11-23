# MM6 playthrough go/no-go

Whether every MM6 game system is ready for an owner's end-to-end playthrough, from New Game to the ending, and what
still blocks it.

## Verdict

**NO-GO.** Assessed on `claude/mm6` at `920c9138a`, 2026-11-23.

A player hits a crash in the first hour. Reading a spellbook ends the game, and so do scrolls, wands and every
enchanted weapon. Each blocker below ends the game through an uncaught `std::runtime_error`, because the main loop
rethrows any exception and the process exits.

The verdict flips to **GO** only when all of these hold, on one commit named here:
1. Every row under [Blockers](#blockers) is closed, each by a game test that does what the player does and checks that
   the game goes on.
2. Every row marked *Required* under [Systems](#systems) is GO.
3. The fresh-process campaign chain C0 to C9 passes on that commit.
4. A Release MM6 helper built from that commit is installed and launches from the Atlas.

A blocker closes either by implementing the behaviour from MM6.exe, or by refusing it the way an unadmitted spellbook
cast does, with a status line and no throw. A refusal keeps the game alive but leaves the system row *Partial*.

## Blockers

Player-reachable paths that end the game. *Seen* means the crash happened in play. *Code* means the path was read in
source and, where it says so, in the shipped MM6 data, but not run.

| # | Player action | Throw site | Evidence |
| --- | --- | --- | --- |
| B1 | Read a spellbook with a character who may learn it, a Cleric reading First Aid, [report](#b1-crash-report) | `Character::useItem`, "spellbook learning rules" | **Seen** 2026-11-23 |
| B2 | Use a scroll on a portrait or the paperdoll | `Character::useItem`, "scroll targeting and effects" | Code |
| B3 | Equip a wand and attack, press Pass, enter turn-based mode or steal | `Character.cpp`, the two "MM6 wand ..." throws | Code. Wands roll from treasure level 2 and equip without a skill. |
| B4 | Hit a monster with a weapon, or a bow's arrow, that has any special enchantment but Antique | `mm6::requireVerifiedPartyAttack` | Code |
| B5 | Melee at Master Staff or Master Mace, or while wearing an "of Force" item | `mm6::requireVerifiedMonsterDamage`, Earth | Code |
| B6 | Use a hired Acolyte's or Piper's ability from their portrait | `mm6::requireVerifiedPlayerCastSkill` | Code. Dal the Acolyte is one of New Sorpigal's street NPCs. |
| B7 | Fight a Medusa Enchantress (Paralyze) or a Corrosive Ooze (Toxic Cloud) | `mm6::requireVerifiedSpellEffect` | Code and `monsters.txt` |
| B8 | Pull Excalibur from the stone, `oute1.evt` event 226 | `MM6ItemEventWrite.cpp`, "generated-artifact state" | Code and the shipped script |
| B9 | Get an event item reward while holding an item, with no room in any backpack | `MM6ItemEventWrite.cpp`, "ground-drop fallback" | Code |

The campaign chain doesn't reach these because its fixtures give items and spells in code and its party carries no
enchanted weapons.

### B1 crash report

High priority. The owner's first crash of the 2026-11-23 playtest.

- **Build.** Installed Atlas, Release arm64 MM6 helper from `626f5f431`, reporting `ECv0.1.1-255-g626f5f431`.
- **Steps.** A Cleric reads the First Aid spellbook from the character screen. Body Magic is a Cleric school and the
  Cleric has the skill, so the prerequisite check passes.
- **What happens.** `Character::useItem` throws "MM6 spellbook learning rules are not implemented". The profile log
  `logs/openenroth_2026_11_23_13_50_34.log` ends with `[critical] Terminated with exception: MM6 spellbook learning
  rules are not implemented`, written by `GameStarter::run`, which rethrows.
- **Second failure.** `main` returns with the character screen still open in `pGUIWindow_CurrentMenu`. During static
  destruction `GUIWindow::~GUIWindow` lets an exception escape, so `std::terminate` aborts with SIGABRT and macOS
  shows a crash report instead of a clean exit. Which call inside the destructor throws is not traced. The candidates
  are `MM_TRACE`, which then goes to the fallback logger at trace level, and `lWindowList`, which is destroyed before
  the window that removes itself from it. The destructor carries an upstream TODO about `cxa_finalize`.
- **What a fix needs.** MM6.exe's spellbook learn rule (school, mastery and class gates, the already-known check,
  speech and sound), a game test that reads a guild-bought book by input and checks the spellbook page, and a clean
  exit when the game ends with a window open.

## Systems

| System | Required | State | Evidence or gap |
| --- | --- | --- | --- |
| Launch, New Game, party creation | Yes | GO | `MM6PartyCreationHeadless.*`, `MM6NewGameHeadless.*`, installed app 2026-11-23 |
| Save, load, autosave, quick save | Yes | GO | `MM6InGameSave.*`, `MM6SaveAnywhereHeadless.*`, first-map gate, chain saves |
| HUD, books, character screen | Yes | GO | `MM6Hud.*`, `MM6BooksHeadless.*`, `MM6CharacterScreenHeadless.*` |
| Walking, map edges, dungeons, stables and boats | Yes | GO | `MM6MapTravel.*`, `MM6Transport.*`, `MM6Dungeons.*`, the map sweep loads all 67 maps |
| Houses, shops, guilds, temples, training, banks, taverns | Yes | GO | `MM6Houses.*`. Gaps that don't block: temple blessings, residents' topic of the day and news. |
| Street NPCs and hirelings | Yes | NO-GO | B6 |
| Melee and bows | Yes | NO-GO | B4, B5 |
| Monsters and encounters | Yes | NO-GO | B7. The rest is covered by `MM6CombatHeadless.*`, `MM6Encounters.*`, `MM6MonsterFamilies.*`. |
| Learning spells | Yes | NO-GO | B1 |
| Casting spells | Yes | Partial | Spells in `mm6::isVerifiedPlayerSpell` cast. Others are refused with a status line, Town Portal and Lloyd's Beacon among them. |
| Scrolls and wands | Yes | NO-GO | B2, B3 |
| Potions | No | Partial | Drinking works, `MM6Potion.*`. Mixing is refused without a throw. |
| Event item rewards | Yes | NO-GO | B8, B9 |
| Main quest | Yes | GO headless | Chain C0 to C9 in fresh processes, with the strength, positioning and no-damage fixtures |
| Class promotions | Yes | Unverified | No MM6 game test promotes a character |
| Side quests | No | Unverified | Only those on the chain's route are covered |
| Party death and respawn | Yes | GO | `MM6PartyDeath.*` |
| Finale and win screen | Yes | GO headless | `MM6Finale.*`, chain C9 |
| Fortune's Turn, chest reroll, All Artifacts | No | GO headless | `MM6FortunesTurn.*`, `MM6ChestReroll.*`, `MM6AllArtifacts.*` |
| Installed Atlas MM6 helper | Yes | GO | `626f5f431` installed and launched 2026-11-23 |

Owner decision still open: whether refusing the unadmitted spells is enough for the playthrough, or whether
Town Portal, Lloyd's Beacon and the rest of the guild stock must cast first.

## Source

- [Character.cpp](../src/Engine/Objects/Character.cpp), `Character::useItem` and the wand paths
- [MM6CombatRules.h](../src/Engine/MM6/MM6CombatRules.h), [MM6MonsterDamage.h](../src/Engine/MM6/MM6MonsterDamage.h)
- [MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h)
- [MM6ItemEventWrite.cpp](../src/Engine/Evt/MM6ItemEventWrite.cpp)
- [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp), [GUIWindow.cpp](../src/GUI/GUIWindow.cpp)

See also: [MM6 campaign status](mm6-campaign-status.md), [Open questions](open-questions.md), [MM6 characters, items and spells](mm6-characters-items-spells.md), [Testing](testing.md)
