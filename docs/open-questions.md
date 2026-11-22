# Open questions

Open questions and known bugs across MM6 and MM7 that were still open on 2026-11-19, grouped by title and area.

Each item names a source file or test where one exists. Remove an item when its fix or answer lands, and say where.
The open GrayFace MM6 backlog is tracked in [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md) and
summarised in [MM6 campaign status](mm6-campaign-status.md), so it is not repeated here.

## MM6: campaign and gates

- **Owner sessions.** The working gate needs visible owner play beyond New Sorpigal: travel, a later dungeon, chain
  checkpoints and the ending.
- **Research-folder tests.** `mm6::PrivateMapSource`, `saveNewGameMM6ScopedForTest` and the `GameTests_MM6Save` cases
  gated on `ENROTH_MM6_MAP_RESEARCH_ROOT` still read the private research folder and `tools/mm6_map_name_manifest.py`.
- **Test-only rule code.** `MM6FirstAidPreflight`, `MM6FirstAidTransaction` and `MM6PowerCureTransaction` are used
  only by their unit tests.
- **Spellbook state between tests.** `MM6GrayFaceShared.BuffDurationShowsMinutesNextToDays` leaves the global
  `spellbookSelectedSpell` at Protection from Fire. Run with only those two tests,
  `MM6Spellbook.SpellPictureSelectsThenCasts` then fails. The full `MM6*` run passes.
- **King's Library panel.** The library's right panel draws red and grey stripes in the C8 capture. Not yet checked
  against MM6's art.

## MM6: rules and combat

- **MM6 monsters taken for MM7 peasants.** `monsterTypeForMonsterId` maps MM6 monster ids through MM7's monster types,
  so MM6 ids 115 to 173, oozes, rats, sea serpents, wolfmen and the hive bosses among them, land in MM7's peasant
  types 39 to 58 (`monsters.txt` of the owner's MM6 data). Killing one runs `Actor::ApplyFineForKillingPeasant`, which
  adds a fine and sets MM7's `AWARD_FINE`, award 1, which MM6 calls "Returned the Prince". In chain C9 the Demon Queen
  gives every character that award and the party a fine of 10,000. `AggroSurroundingPeasants` and the other
  `isPeasant` callers read the same types. What MM6.exe does on a peasant kill is not checked yet.
- **Encounter spawner.** MM6.exe rings extra monsters 128 (outdoor) or 64 (indoor) units around the spawn point,
  caps the level odds at difficulty 4 and keeps table hostility. The port stacks them, caps at 5 and spawns friendly
  aggressors. Not changed because it reaches every MM6 map. `SpawnEncounter` in `src/Engine/Objects/Actor.cpp`.
- **Travel clears recovery.** Map travel calls the shared `restAndHeal`, which clears recovery. MM6's travel keeps it.
  `src/Application/Game.cpp`, the travel branch near `restAndHeal`.
- **Map variable overflow.** `MM6MapByteState` throws where MM6 wraps on subtract below zero or saturates on add above
  255. No shipped script was checked for either case. `src/Engine/Evt/MM6MapByteState.cpp`.
- **Event W2 follow-up.** With nobody active, the event run picks one member, but other active-member reads in the
  same run (a held item moved when a new one is given) still see nobody. `MM6GiveItemEvent.cpp`, `EvtInterpreter.cpp`.
- **Map timer hour 88.** The Temple of the Sun (t4) dawn timer has an out-of-range hour byte. Its first firing now
  follows GrayFace, later firings step one day where GrayFace steps 88 hours. `GameTests_MM6MapVisit.cpp`.
- **Break Weapon and Zombie.** If MM6 Break Weapon is admitted for an unarmed target, the shared empty-list path can
  assert. Divine Intervention's Zombie exception is MM7-gated until MM6 evidence exists. `Character.cpp`.
- **Question inside another window.** A Question reached while another branchless window is open throws, where MM6
  ends the event. No shipped script reaches it.
- **Skill bonus formulas.** Several MM6 skill bonuses in `Character::GetSkillBonus` use MM7's formulas where MM6.exe's
  skill bonus routine (0x483930) differs. Spear damage starts at Expert where MM6 needs Master, Expert spear gives no
  armor class, shields scale by (1, 1, 2) where MM6 has (1, 2, 3), leather by (1, 1, 2) where MM6 has 1, and the blaster
  multipliers differ. Weapon-skill hireling points ride on the same terms.
- **Artifact effects beyond attributes.** MM6.exe's artifacts and relics have effects outside its item bonus routine
  that the port doesn't have yet: Mordred's healing hit, Conan's slaying, Pellinore's regeneration and Hades's drain,
  the shielding of Valeria and Aegis, the immunities of Pendragon and Aegis, the spell skill boost of Guinevere,
  Igraine and Morgan, and the extra hit point damage of Hades, Ares and Artemis. Addresses are in the artifact table on
  [Characters, items and spells](mm6-characters-items-spells.md#artifacts-and-relics). Poseidon's armor class -10 has
  no game test, a new party has no armor class for it to lower.
- **Hireling share rounding.** MM6.exe's gold-find routine 0x41EDE0 takes `(salaries / 100) * gold / 100` from a find,
  at least 1. `Party::foundGold` takes `gold * salaries / 100 / 100`. They differ only when the hirelings' prices don't
  add up to a multiple of 100, which needs one of the cheap professions 52 to 77 hired, and by at most a few gold.
- **Any-item roll.** Chests and `GiveItem` kind 0 keep MM6.exe's off-by-one, which favours item 1, because the GrayFace
  patch leaves that roll alone. Fixing it changes chest contents on every map. Owner's call. `mm6::generateAnyItem`.
- **Retracting lava bridges.** The Supreme Temple of Baa's four lava bridges are doors 12 to 15. A monster on one when
  it retracts falls into the lava. GrayFace's C24 limit wouldn't stop that either.

## MM6: houses, shops and NPCs

- **Temple blessings.** Above reputation 200 a matching donation casts Guardian Angel through Day of Protection at
  master rank. Not done, the spells aren't verified MM6 player spells. TODO in `GUIWindow_MM6House::donate`.
- **House residents can't be hired.** Joinable residents such as the New Sorpigal healer and scholar show only their
  scripted topics. `mm6SelectHouseTopic` in `src/GUI/UI/Houses/MM6House.cpp`.
- **Shop sounds and hints.** The MM6 sell screen is silent. MM6.exe plays sound 479 and speech 77 on a sale and speech
  79 with sound 27 on a refusal. There is no sell hint, so GrayFace's hint half of W26 has nothing to attach to.
- **Shop item info box.** MM6 shops have no right-click item info box. When it is ported, GrayFace's W28 check (no
  skill identify or repair by a member who can't act) comes with it. `src/GUI/UI/Houses/MM6Shop.cpp`.
- **Shop footer.** The footer draws over the status bar, so "Done!" and "Good as New!" never show. Not checked against
  MM6.exe.
- **Shop item hover.** Hovering an item on a shop shelf shows no name in the status bar. Not checked against MM6.exe.
- **Greeting speech.** MM6 greets with speech 22 or 23 by the party's reputation. Ours picks good day or good evening
  by the hour. `initializeNPCDialogue` in `UIDialogue.cpp`.
- **Street NPC news carry-over.** A street NPC slot keeps its news topic across map loads within one game, as in
  vanilla. GrayFace has no fix. Owner's call whether to clear it.
- **Potion mixing.** Item-on-item use of potions stays refused (`mm6::shouldBlockItemOnItemUse` in
  `src/Engine/MM6/MM6ConsumableRules.h`). The mixing outcomes are not traced.

## MM6: saves

- **A party carrying random ground treasure or a random `GiveItem` gift can't save.** `mm6::spawnRandomTreasure` and
  `mm6::giveEventItem` build their items through `generatedRuntimeItem`, which gives them no native record. Once one is
  carried the inventory capture throws "MM6 raw inventory edits are not reversible" and the save is refused. Seen with
  throwaway game tests that picked up the Oracle's third treasure marker (a Leather Belt of Speed) and that put a level
  2 `GiveItem` roll in the backpack on `oute3`. Chest loot is not affected, the chest projection reads each record
  from the chest.
- **A party carrying an item it found lying on a map can't save.** Items a map's own data puts on the ground, the
  herbs around New Sorpigal or the scroll in the hive, load through the shared `Item_MM7` reconstruct, which drops the
  native record. Once the party picks one up the inventory capture finds no record and every save says "This game
  can't be saved here yet". Seen with a throwaway game test that picked up an `oute3` herb and saved.
- **Poison Resistance on ground items.** Ground item records load through the shared `SpriteObject_MM7` reconstruct,
  which reads standard bonus 14, MM6's Poison Resistance, as Earth. Chests convert it (`mm6::reconstructChest`), ground
  items don't. Whether a shipped map places such an item was not checked.
- **Border autosave reopens the travel dialog.** The autosave on a walk across an outdoor border holds the party at
  the edge, so loading it opens the travel dialog again at once. Not checked against MM6.exe.
  `MM6InGameSave.BountyKillSurvivesAutosave` closes it with No.
- **First-map save after a spawned actor.** A v5 first-map save refuses with "ARTR count differs from ACTR baseline"
  once an actor was added to `oute3` after the anchor, seen only with a test's `spawnMonster`. Whether play can add
  one there (a summon, a scripted spawn) was not checked. `MM6V5ActorRuntimeFields.cpp`.

## MM6: media and UI

- **Voices.** MM6 voice samples exist, but the shared `speechVariants` table has no variant for the travel speeches 71
  and 72 or for Beg, Threat and Bribe (53 to 58). Those play as portrait reactions only, and the W8 cap always applies.
- **Dungeon door sounds.** When MM6.exe opens the dungeon entry and exit dialog (0x43A070) it calls 0x496520 with the
  house if the room's sound byte is set, and 0x488CA0 with 47 on a character under a level and alert check. Neither
  call is traced. The MM6 path plays nothing there. The d01 and d02 rooms have no sound byte.
- **Spell cast on an item.** `GUIWindow_Inventory_CastSpell` in `src/GUI/UI/UIInventory.cpp` draws `x_x_u`, absent
  from MM6 `icons.lod`. It opens for item-targeted spells listed by MM7 ids, and MM7's Fire Aura (4) shares its number
  with MM6's Fire Bolt. Whether an MM6 cast reaches it is not verified.
- **Spell popup mastery lines.** The spellbook's right-click popup shows a fourth line, "Get : Casting cost 0 spell
  points", the Master text under a Grandmaster label MM6 doesn't have.
- **Oracle background.** A hidden capture of the Oracle's conversation draws the left view area plain white. Not checked
  in visible play, so it may be offscreen-only or a missing house picture.
- **Missing chest button art.** Opening a chest logs a missing `ib-bcu-a`, the MM7 cancel button background loaded in
  `src/GUI/GUIWindow.cpp`. Harmless.
- **Character screen.** Pressing I draws the stats tab for one frame before the inventory. Tab doesn't switch members
  on the MM6 character screen, the digit keys do. Neither is checked against MM6.exe.
- **Space at the New Sorpigal well.** Space picks the well's wall face, so a drink needs a click. Not checked against
  MM6.exe.
- **Menus.** The title music starts at the top of its track where MM6 starts 18.5 s in. The Escape menu has no pressed
  art. MM6 saves have no thumbnail. The voice volume feedback plays OpenEnroth's sound. Credits are not traced.
- **Fog and overlays.** MM6 fog is not implemented, the map-visit block only carries its fields. The Torch Light radius
  and the Fly and Water Walk overlays are not ported. Buff portrait sparkles are not drawn.

## MM7

- **Strange Temple exit.** Event 501 carries an out-of-bounds Harmondale X, so OpenEnroth loads the map but skips the
  placement. Static evidence says the original executable copies the value and lets its first outdoor update handle
  it. A private native trace of the settled position decides between clamping, travel handling or no change.
  `MM7StrangeTemple` pins both the out-of-bounds source coordinate and today's in-bounds landing.
- **Ordinary movement and combat.** Chains still use approach teleports. The audit wants representative regions walked
  and fought through input. Movement before the Colony Zod entrance still needs a stable route.
  `tools/run_mm7_gameplay_harness.py`.
- **Graphical Mac play.** The audit's interface, media, mods and long-session rows need visible play on the Mac. See
  [MM7 status](mm7-status.md) for the rows. The chest reroll button and the Fortune's Turn popup, checkbox, bounty, ore
  smith and enchant flows were checked headlessly and in hidden captures, not by clicking in the installed app.
- **Full audit.** The archive, map, spell, item and monster audit is planned for the week of 2026-11-23 and has not
  started. See [MM7 status](mm7-status.md#full-audit-plan).
- **In-game mod panel.** A read-only panel of the Atlas launch options and the Fortune's Turn checkbox is on the
  owner's backlog, not implemented.
- **Scroll targeting exits.** Cancel returns the scroll. Attacking, passing the turn, stealing, a full spell queue and
  turn-based wait still use it up. `src/Engine/Objects/Character.cpp`, `GameTests_MM7Scrolls.cpp`.
- **5:00 AM sunlight.** MM7 has the same `UpdateSunlightVectors` minute check MM6 fixed behind `fix_sunrise_light`.
  MM7 owner's call. `src/Engine/Graphics/Outdoor.cpp`.
- **Wall pressure plates.** Original MM7 fires a pressure plate the party runs into. Upstream's fix for #2244 dropped
  wall plates along with the lift retrigger. MM7 keeps upstream's behaviour. `BLV_ProcessPartyActions` in `Indoor.cpp`.
- **2026-11-07 audit items with no fix recorded since.** A wand charge is spent before the outcome. Monster Haste and
  Day of Protection are never read. The Hour of Power hit check reads the Shield buff's power. The `_SelectTarget`
  index never advances. Two enchantment families add no elemental damage. Artifacts 530, 531 and 536 lack their
  described effects. Two grandmaster teacher requirements and two Disarm Trap caps differ from their text.
- **Upstream issues still present at the audit.** #2867, #2502, #1463, #2770, #2763, #2786, #2071, #920 and #2003,
  #1530, #682, #586, #1917, #1918, #843, #2268, #2232. Mac: #1201 (HiDPI), #2171 (window size after fullscreen) and
  #1591 (signing).
- **All Artifacts and Storage.** The owner's own transaction with the opt-in mod is still to be run.
- **Shoals pixels.** The dense green fog is authored. An exact comparison with the original executable is optional.

## Platform and release

- **Screen Recording permission.** Atlas is ad-hoc signed, so macOS may ask for Screen Recording again after each
  install before rolling video works.
- **Lint.** `check_tidy` needs clang-tidy 22, which is not installed on the owner's Mac.

## Source

- [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp), [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp)
- [MM6ConsumableRules.h](../src/Engine/MM6/MM6ConsumableRules.h)
- [MM6MapByteState.cpp](../src/Engine/Evt/MM6MapByteState.cpp)
- [Actor.cpp](../src/Engine/Objects/Actor.cpp), [Character.cpp](../src/Engine/Objects/Character.cpp)
- [GUIWindow.cpp](../src/GUI/GUIWindow.cpp)
- [MM6CampaignChain.cpp](../test/Bin/GameTest/MM6CampaignChain.cpp)
- [GameTests_MM7Scrolls.cpp](../test/Bin/GameTest/GameTests_MM7Scrolls.cpp)
- [run_mm7_gameplay_harness.py](../tools/run_mm7_gameplay_harness.py)

See also: [MM6 campaign status](mm6-campaign-status.md), [MM7 status](mm7-status.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md), [Releases](releases.md)
