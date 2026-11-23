# Open questions

Open questions and known bugs across MM6 and MM7 that were still open on 2026-11-23, grouped by title and area.

Each item names a source file or test where one exists. Remove an item when its fix or answer lands, and say where.
The open GrayFace MM6 backlog is tracked in [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md) and
summarised in [MM6 campaign status](mm6-campaign-status.md), so it is not repeated here.

## MM6: high priority

- **Spellbook crash (B1).** Reading a spellbook with a character who may learn it ends the game, and quitting then
  aborts in `GUIWindow::~GUIWindow`. Hit by the owner on 2026-11-23 with a Cleric reading First Aid. The crash report,
  the log line and what a fix needs are in [playthrough go/no-go](mm6-playthrough-go-no-go.md#b1-crash-report).
  `Character::useItem` in `src/Engine/Objects/Character.cpp`.
- **Other reachable throws (B2 to B9).** Scrolls, wands, enchanted weapons, the stun channel, Acolyte and Piper
  abilities, two monster spells, the Excalibur event and an item reward with nowhere to go. Each ends the game. The
  table is in [playthrough go/no-go](mm6-playthrough-go-no-go.md#blockers).

## MM6: campaign and gates

- **Owner sessions.** The working gate needs visible owner play beyond New Sorpigal: travel, a later dungeon, chain
  checkpoints and the ending.
- **Class promotions.** No MM6 game test promotes a character. The event setter writes the class ID the script gives.
- **Spellbook state between tests.** `MM6GrayFaceShared.BuffDurationShowsMinutesNextToDays` leaves the global
  `spellbookSelectedSpell` at Protection from Fire. Run with only those two tests,
  `MM6Spellbook.SpellPictureSelectsThenCasts` then fails. The full `MM6*` run passes.
- **King's Library panel.** The library's right panel draws red and grey stripes in the C8 capture. Not yet checked
  against MM6's art.

## MM6: rules and combat

- **Item-target spells.** Enchant Item and Golden Touch are refused, so no MM6 cast opens the item screen.
  `mm6::isItemTargetSpell`.
- **Gate Master hireling.** The Gate Master's ability opens MM7's Town Portal book. Not checked against MM6.exe.
  `NPC.cpp`, `Game.cpp`.
- **Encounter spawner edges.** MM6.exe 0x455910 spawns nothing when the group would bring the map to 500 actors,
  spawns indoors on a spawn point outside every sector, and has no alert-status check. The port fills slots one by one,
  drops such a spawn point as MM7 does, and keeps MM7's alert check. No shipped map was scanned for spawn points
  outside every sector. `SpawnEncounter` in `src/Engine/Objects/Actor.cpp`.
- **Monster AI on hostile start.** MM6 monsters start with their `monsters.txt` hostility, so under the shared AI a
  hostile one engages anywhere in detection range (10240) on map entry, where one that starts friendly turns hostile
  only within 5120. MM6.exe's own AI hostility steps (0x401AC7, 0x402036) weren't compared with the shared AI.
  `Actor::UpdateActorAI` in `src/Engine/Objects/Actor.cpp`.
- **Event W2 follow-up.** With nobody active, the event run picks one member, but other active-member reads in the
  same run (a held item moved when a new one is given) still see nobody. `MM6GiveItemEvent.cpp`, `EvtInterpreter.cpp`.
- **Map timer hour 88.** The Temple of the Sun (t4) dawn timer has an out-of-range hour byte. Its first firing now
  follows GrayFace, later firings step one day where GrayFace steps 88 hours. `GameTests_MM6MapVisit.cpp`.
- **Divine Intervention and Zombie.** Divine Intervention's Zombie exception is MM7-gated until MM6 evidence exists.
  `CastSpellInfo.cpp`.
- **Question inside another window.** A Question reached while another branchless window is open throws, where MM6
  ends the event. No shipped script reaches it.
- **Hireling share rounding.** MM6.exe's gold-find routine 0x41EDE0 takes `(salaries / 100) * gold / 100` from a find,
  at least 1. `Party::foundGold` takes `gold * salaries / 100 / 100`. They differ only when the hirelings' prices don't
  add up to a multiple of 100, which needs one of the cheap professions 52 to 77 hired, and by at most a few gold.
- **Blaster bolt range.** MM6.exe's projectile hit routine (0x431505 to 0x43152B) takes a blaster bolt's range factor
  from the bolt's byte at 0x54, read statically as its distance, and uses 1 only at Expert and Master. At Master it also
  adds the skill to hit. The port's `SPELL_BLASTER_PROJECTILE` case in `Actor.cpp` uses 1 at every mastery, so a Novice
  bolt at long range may hit more often than in MM6. The 0x54 byte isn't confirmed at runtime.
- **Guild learn speech.** MM6.exe's shop click handler 0x4A4CA0 has a learn-skill branch (0x4A4D09, dialogue states
  36 to 66) that plays speech 78 on success, and status 403 with sound 27 for a skill already known. The port's
  `MM6Guild` learn plays nothing and `MM6SkillGuild` plays its house learn sound. Which guilds reach 0x4A4D09 isn't
  traced.
- **Stuck on the edge line.** A party that lands from a fall exactly on oute3's west edge line while holding forward
  stays there, bobbing between z 155 and 158.75 with the airborne flag set, so the border trip is never offered. Seen
  for 300 frames with a healthy party. Not traced or checked against MM6.exe.
- **Any-item roll.** Chests and `GiveItem` kind 0 keep MM6.exe's off-by-one, which favours item 1. The GrayFace patch
  leaves that roll alone. Owner decision 2026-11-22: fix it, with no flag. Not done. It changes chest contents on every
  map, so tests that pin chest rolls will move. `pickChestBaseItemId` in `MM6ChestBaseItemPicker.h`.
- **MM7 monster types on MM6 paths.** `supertypeForMonsterId` makes MM6's Genies and Plate Knights undead and its
  Ghosts, Liches and Skeletons not. It decides Turn Undead, Destroy Undead, Control Undead, Mass Fear, Enslave and bow
  slaying, which MM6 refuses or throws on today. Melee slaying uses `mm6::weaponSlays`. Each needs MM6's own families before it is admitted.
  `CastSpellInfo.cpp`, `SpriteObject.cpp`, `Vis.cpp`, `Character.cpp`.
- **Monster popup.** The right-click popup offsets the monster picture by MM7 type (`monster_popup_y_offsets`) and
  never plays the attack animation for MM7's peasant types, MM6 IDs 115 onwards. MM6.exe's popup was not traced.
  `UIPopup.cpp`.
- **Land monsters in water.** Outdoors the port steers a land monster that stands on water back to land, an MM7 rule.
  The part of MM6.exe's mover read so far (0x4622D8 to 0x46237C) only turns it away at the shore. `UpdateActors_ODM`.
- **Retracting lava bridges.** The Supreme Temple of Baa's four lava bridges are doors 12 to 15. A monster on one when
  it retracts falls into the lava. GrayFace's C24 limit wouldn't stop that either.

## MM6: houses, shops and NPCs

- **Temple blessings.** Above reputation 200 a matching donation casts Guardian Angel through Day of Protection at
  master rank. Owner decision 2026-11-22: they must work, without admitting the spells as player spells. Not done.
  TODO in `GUIWindow_MM6House::donate`.
- **House resident topic of the day and news.** MM6.exe 0x4998A0 lists a resident's profession topic of the day
  (`proftext.txt`) before Join, and its news (`npcnews.txt`, picked by 0x43BC20 as the house lists its people) after
  Join, when its `npcdata.txt` News column is set. The port shows neither, so most residents, teachers and guild
  recruiters included, lack their first topic. `GUIWindow_MM6House::refreshTopics`.
- **Street NPC news carry-over.** A street NPC slot keeps its news topic across map loads within one game, as in
  vanilla. GrayFace has no fix. Owner decision 2026-11-22: refresh it on map load. Not done.
- **Potion mixing.** Item-on-item use of potions stays refused (`mm6::shouldBlockItemOnItemUse` in
  `src/Engine/MM6/MM6ConsumableRules.h`). The mixing outcomes are not traced.

## MM6: media and UI

- **Look up and look down portraits.** MM6.exe's speech table gives speech 63 portrait 31 and speech 64 portrait 32.
  The shared `portraitVariants` table in `src/Engine/mm7_data.cpp`, which MM6 uses, gives 17 and 20.
- **House view background.** Hidden captures of the Oracle's conversation and of the Circus tents on Dragonsand draw
  the left view area plain white. Not checked in visible play, so it may be offscreen-only, since hidden runs play no
  house video, or a missing house picture.
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
- **Full audit.** The archive, map, spell, item and monster audit has not started. Its plan names the week of
  2026-11-23, the delivery order puts it after the MM6 working gate. Owner's call. See
  [MM7 status](mm7-status.md#full-audit-plan).
- **In-game mod panel.** A read-only panel of the Atlas launch options and the Fortune's Turn checkbox is on the
  owner's backlog, not implemented.
- **Scroll targeting exits.** Cancel returns the scroll. Attacking, passing the turn, stealing, a full spell queue and
  turn-based wait still use it up. `src/Engine/Objects/Character.cpp`, `GameTests_MM7Scrolls.cpp`.
- **5:00 AM sunlight.** MM7 has the same `UpdateSunlightVectors` minute check MM6 fixed behind `fix_sunrise_light`.
  MM7 owner's call. `src/Engine/Graphics/Outdoor.cpp`.
- **Wall pressure plates.** Original MM7 fires a pressure plate the party runs into. Upstream's fix for #2244 dropped
  wall plates along with the lift retrigger. MM7 keeps upstream's behaviour. `BLV_ProcessPartyActions` in `Indoor.cpp`.
- **2026-11-07 audit items still open on 2026-11-23.** A wand charge is spent before the outcome. Monster Haste and
  Day of Protection are never read. The Hour of Power hit check reads the Shield buff's power. The `_SelectTarget`
  index never advances. The Assassins' and Barbarians' enchantments add no elemental damage. Artifacts 530, 531 and
  536 lack their Sleep, elf-slaying and drowning effects. Two grandmaster teacher requirements differ from their text,
  and the Archer and Warrior Mage Disarm Trap caps are unresolved against MM7.exe.
- **Upstream issues still present at the audit.** #2867, #2502, #1463, #2770, #2763, #2786, #2071, #920 and #2003,
  #1530, #682, #586, #1917, #1918, #843, #2268, #2232. Mac: #1201 (HiDPI), #2171 (window size after fullscreen) and
  #1591 (signing).
- **All Artifacts and Storage.** The owner's own transaction with the opt-in mod is still to be run.
- **Shoals pixels.** The dense green fog is authored. An exact comparison with the original executable is optional.

## Platform and release

- **Screen Recording permission.** Atlas is ad-hoc signed, so macOS may ask for Screen Recording again after each
  install before rolling video works.
- **Lint.** `check_tidy` needs clang-tidy 22, which is not installed on the owner's Mac.
- **Build page.** `build-and-dev-setup.md` is linked from the index, the README and most process pages, but the page
  was never written.

## Source

- [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp), [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp)
- [MM6ConsumableRules.h](../src/Engine/MM6/MM6ConsumableRules.h)
- [Actor.cpp](../src/Engine/Objects/Actor.cpp), [Character.cpp](../src/Engine/Objects/Character.cpp)
- [GUIWindow.cpp](../src/GUI/GUIWindow.cpp)
- [MM6CampaignChain.cpp](../test/Bin/GameTest/MM6CampaignChain.cpp)
- [GameTests_MM7Scrolls.cpp](../test/Bin/GameTest/GameTests_MM7Scrolls.cpp)
- [run_mm7_gameplay_harness.py](../tools/run_mm7_gameplay_harness.py)

See also: [MM6 campaign status](mm6-campaign-status.md), [MM7 status](mm7-status.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md), [Releases](releases.md)
