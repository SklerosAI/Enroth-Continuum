# MM6 houses and services

How the port routes MM6 houses, what each service does, and how hirelings, street NPCs and resting work, with the
MM6.exe facts behind the rules.

## Overview

MM6 houses run in their own window classes under `src/GUI/UI/Houses/MM6*.cpp`, not the MM7 house screens. The base
class `GUIWindow_MM6House` (`MM6House.h`) lists the people in the house and its exit to another map. Selecting the
proprietor shows the service menu, selecting an NPC shows its topics, and selecting the exit asks whether to take it.
Subclasses handle shops and spell guilds (`MM6Shop`), mercenary and thieves' guilds (`MM6SkillGuild`) and stables and
boats (`MM6Transport`). Temples, taverns, banks and training halls are handled in `MM6House.cpp` with rules in
`MM6Temple`, `MM6Tavern` and `MM6Training`.

`mm6EnterHouse` is the MM6 `enterHouse`. It refuses with a status line, before touching party or UI, when the house
has no room, its room media can't load, or it would show nothing at all.

Many MM6.exe addresses for these routines are in the `@offset` tags of the functions.

## House data

`2devents.txt` has 557 houses. `initializeHouses` (`src/Engine/Tables/HouseTable.cpp`) reads the shared columns, and
`mm6::decodeHouseServiceData` (`src/GUI/UI/MM6HouseServiceData.h`) keeps MM6's own meaning of the rest:

| Column | MM6 meaning |
|---|---|
| Type | House type label, see below |
| Val | Price factor for every service of the house |
| A | Stock or skill text. In MM7 this is a skill price multiplier. |
| C | Stock refresh interval in days for shops. Some rows hold a route or arena note. |
| Open, Closed | Open from the start of the open hour to the end of the hour before closing, across midnight when needed (`isHouseOpen`) |
| Pic, Map, Restrictions, Text | The house's exit to another map, see below. MM7 doesn't use these. |

`parseHouseType(label, GameTitle::MM6)` maps MM6's labels. MM6 adds a general store, an Element guild, a Self guild,
mercenary and thieves' guilds, throne rooms, the city council (a separate town hall type), circus, jail, seer, and
plain-house labels for tents, wagons, dungeon and castle entrances and similar. An unknown MM6 label is invalid, where
MM7 falls back to the mercenary guild. Unit test `MM6HouseType_ut.cpp` pins the routing.

MM6 temples and training halls don't read the restriction and max-level text in `2devents.txt`. Their limits are
hard-coded by house id in MM6.exe (`templeOffersHeal`, `trainingMaxLevel`).

### House exits

Columns 20 to 22 give a house an exit to another map (`MM6HouseExit.h`). The original reads each cell with `atoi`,
so text reads as the number it starts with. The exit is listed after the people and appears only once its quest bit
is set. A restriction of `-n` picks the n-th arrival point of a table MM6 keeps for the Free Haven sewer. Any other
restriction arrives at the map's Party Start. Taking an exit autosaves, leaves the house and starts the transition.

## Service menu rules

- The menu is hidden from an active character who can't act at weapon, armour and magic shops, the general store,
  guilds, mercenary and thieves' guilds, stables, boats, taverns and training. The panel shows the "in no condition"
  line instead, and the menu returns on the first frame with a character who can act. Temples and banks don't check.
- Every shop screen also refuses a click from a character who can't act, with the error sound. This is vanilla
  MM6.exe, so it has no config flag. GrayFace's W28 fix is for the item info box identify and repair, which MM6 shops
  don't have.
- The focus colour starts on the first option and follows the mouse every frame. Arrow keys don't move it in a house.
- A looping room movie can be told to finish its pass, and some rooms move on to another house once the movie ends
  (`roomAfterMovie`). A `MoveNPC` event can stop the loop.

## Shops

### Prices

`MM6ShopPrices.h` holds MM6's formulas. Unlike MM7, reputation plays no part.

- Merchant percentage: `(3, 4 or 5 by mastery) * (Merchant level + hireling points) + 7`, and 0 when the points are
  0. Hireling points: Trader 4, Merchant 6, Gypsy 3, Negotiator 4, Duper 8, each profession counted once.
- Item value: base value plus 100 per point of a standard bonus. A special bonus adds its value when above 10 and
  multiplies the base otherwise.
- Buy: `value * Val` less the merchant percentage, never below the item value.
- Sell: `merchant% * value / 100 + value / (Val + 2)`, between 1 and the value. A broken item fetches 1 gold. A general
  store pays half.
- Identify: `Val * 50`. Repair: `value / (6 - Val)`. Both take the merchant discount but not below a third.

### What shops buy

MM6 decides by house id, not by type (`shopTradesItem`). Weapon shops take single-handed weapons through bows, armour
shops armour through boots, magic shops items with no weapon or armour skill, and general stores anything. Items 430 to
435 and above 445 are taken by no shop.

Vanilla MM6.exe let a weapon shop or the general store buy a blaster for 1 gold. With
`gameplay.fix_shops_buying_blasters` (default on) a shop refuses to buy an item with base value 0, as the GrayFace
patch does.

### Stock and refresh

- Stock lives in MM6's own layout: 120 physical rows of twelve 28-byte item records (`MM6ShopStockFields.h`). Shop rows
  1 to 47 and spell guild rows 119 to 140 each have a 64-bit refresh deadline (`MM6ShopGuildClockFields.h`). Shop 47
  and guild 119 address the same physical slot.
- Stock generation rules are read from MM6.exe (`decodeShopStockRules`). Weapon, magic and general shops have one rule
  row, armour shops a second one for slots 4 and up.
- Stock refreshes when a buy screen opens after the deadline. The next deadline is now plus the house's day interval.
  A shop refresh regenerates both banks, a guild refresh only its spellbooks.
- With the opt-in `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY=1` (set by Atlas, see [Mods and Atlas](mods-and-atlas.md)) the
  first buy screen of each visit refreshes even before the deadline, through the same generator, and writes the
  ordinary next deadline. Later buy screens of that visit follow the deadline again. Without it the schedule above is
  the vanilla one.
- Generation draws from the MM6 gameplay LCG through the MM6 item generator, and every generated slot is identified.
  Slots past the visible count keep their bytes.
- A spell guild of one school offers its first books. A guild of several schools picks a school per slot first.
- New Game starts with an empty stock image and zero deadlines, so every shop fills on its first visit.
- Stock and clocks are saved in EC6 (`SHOP` and `SCLK`). See [MM6 save format](mm6-save-format.md).

## Guilds

- Spell guilds (`MM6Guild`) serve only members, checked by the guild's membership award. Members buy spellbooks and
  learn the guild's skills that their class can learn.
- Mercenary and thieves' guilds (`MM6SkillGuild`) teach five skills each, chosen by house id. The price is 250 times
  Val at a mercenary guild and 100 times Val at a thieves' guild, with the merchant discount floored at a third.
  Learning sets the skill to 1. A short purse plays one house sound, a skill the class can't learn plays the error
  sound.
- Membership comes from NPC topics 381 to 397, which MM6 answers in the executable, not in `global.evt`. Joining takes
  the price from MM6's table, gives every character the membership award through the award variable write (chime and
  reaction for each), clears the recruiter's third topic and plays the joined speech on the active character.
- Expert teachers are the even topics 200 to 258, master teachers the odd topics 201 to 259. Masters check a per-skill
  set of rank, stat, class or honorary award, reputation, fame or item requirements. The Light and Dark masters need
  effective reputation of at least 1000 or at most -1000.

## Temples

- Heal clears every condition and restores HP and SP, then puts back conditions this temple can't cure. Temple Baa
  can't cure dead, stone or eradicated, Temple Stone cures everything, every other temple can't cure eradicated.
- Donate takes Val and raises reputation by 200, to at most 200. Temple Baa takes it back.
- With effective reputation above 200, MM6 also casts a blessing on certain donations. This is not done, the spells
  aren't admitted for MM6.

## Training

Halls cap levels by house id. The price never drops below a third of the full price. The first level bought in a visit
moves time to 9 am a week after the next dawn and rests the party.

## Taverns

- Rent Room takes the price, leaves the house and sleeps until 6 am without eating food (`tavernRoomRestMinutes`).
- Fill Packs fills food up to the tavern's Val, or refuses when the packs already hold that much.
- Have a Drink costs 1 gold. Half the drinks give the got-drunk reaction, a third of those leave the character drunk,
  and a quarter of the others add 5 to 10 to a random stat bonus.
- Tip the barkeep costs 1 gold after a drink at that tavern. The barkeep tells one piece of the region's news, picked
  once per tavern and kept. A region is a map's 1-based position among the base maps of the Games LOD directory, and a
  region without news tells region 1's.
- The per-tavern drink bits and picked tips are in EC6 `PART` v5.

## Banks and town halls

- Deposit and Withdraw take a typed amount and move all of it when the amount is more than the party has or holds.
- Town hall bounties (topic 399) roll a monster per hall per month. Killing it pays 100 gold per monster level with the
  hireling share, gives every character the bounty award, and adds the level to reputation. The three MM6 halls use the
  first three town hall slots of the shared bounty arrays.

## Stables and boats

`MM6Transport` shows a stable or boat as a menu. The route tables come from MM6.exe (`decodeTransportRules`): 36 routes
and three slots for each of the 21 stables and boats. Stables are houses below 57, boats the rest.

- A slot shows its route on the route's weekdays (day of month modulo 7), when the route has no quest bit or the party
  has it, and not twice in a row.
- The price never drops below a third of the full price. House 59 is free.
- Trip days drop for a hired sailor, navigator, pirate, horseman or explorer, to at least 1.
- Paying passes the trip's days with rest and heal, then the party arrives at the route's point.
- `gameplay.fix_travel_autosave` (default on) writes the autosave before the fare and the days. Vanilla wrote it after,
  so loading it lost the fare and the days without the trip.
- `gameplay.fix_travel_speech_delay` (default on) leaves as soon as the horseman's or boatman's farewell (the room's
  sound 2) ends, capped by the vanilla 1.5 s for a coach and 2.5 s for a boat. The party's travel speeches 71 and 72
  have no voice line in MM6.exe.
- Stables take part in the price-fixing quest. Each agrees once, and when all nine have agreed the quest is done.

See [travel and maps](mm6-travel-and-maps.md) for the arrival side.

## Hirelings and professions

- Two hireling slots. Hiring (MM6.exe 0x4A3B69), the hireling's gold share (price / 100, at least 1) and its cut of
  found gold match the shared code.
- Profession effects the port uses: merchant points (above), travel days (above), effective reputation for event
  compares and teachers (Bard +200, Pirate, Gypsy, Duper and Burglar -200 each), a hired Enchanter's +20 resistance,
  diplomacy bonuses for Counselor, Barrister and Negotiator, and weapon skill points (Arms Master 2, Weapons Master 3,
  Squire 2) in the attack, melee damage and shooting bonuses, with the Squire's 2 in every armor class term
  (`MM6CombatRules.h`, MM6.exe 0x483930).
- Event variables 214 and 223 hire, dismiss and test hirelings, see [MM6 events](mm6-events.md).
- House residents marked as joinable can't be hired yet. The MM6 house window shows them by name and runs their scripted
  topics only.

## Street NPCs

Map load gives a generated street NPC to each peasant actor that carries an NPC link (see
[combat and monsters](mm6-combat-and-monsters.md)). The generator draws a portrait from MM6's male and female tables
in one random call (`MM6StreetNpcPortraits.h`). The sex, which also picks the name list, comes from a byte per monster
ID at MM6.exe 0x4BF8B4 (read at 0x469210): the Peasant F1 and F2 families are women, Peasant M1 men
(`mm6::nativeStreetNpcSex`). That table sits one ID below the `monsters.txt` rows, 120 to 131 are female where the
Peasant F rows are 121 to 132, which leaves the three street families alone.

The conversation (`MM6StreetNpcTalk.h`) opens in one of three modes, chosen at its start from fame, reputation and the
NPC's state:

- Topics: the profession's topic of the day (`proftext.txt`), Join, and the news (`npcnews.txt`, picked once per NPC
  from the current map's rows). Join leads to More Information and Hire.
- A fame refusal.
- Beg, Threat and Bribe. Whether the NPC goes along depends on its personality (`npcBTB.txt`). Each costs reputation,
  `max(0, base - D)` with base 10, 50 and 20. A bribe costs `max(10, (100 - D) * (bribes + 1) * 50 / 100)` gold.
  Diplomacy D is `(Diplomacy level + 4 Counselor + 8 Barrister + 4 Negotiator) * (2, 3 or 4 by mastery)`.

Begged, bribed and threatened states live in the street NPC table, which MM6 regenerates on every map load and never
saves. The party's bribe count is saved in `PART` v5. A street NPC's news survives regeneration within one game, as in
vanilla. An MM6 New Game clears the table.

MM6 refuses to open a street NPC conversation with nobody active, and so does the port.

## Rest

- `mm6::hostileMonstersNearParty` (MM6.exe 0x42A680) refuses rest when a monster that isn't friendly, dying, dead or
  removed is within 2560 units indoors or 5120 outdoors, by MM6's octagonal distance. Hostility uses
  `isMonsterHostileToParty`.
- Resting on volcanic ground costs 4 food (`foodRequiredForTile` reads the MM6 tile's tileset). Without food the rest
  refuses with the error sound and no speech.
- A rest encounter needs a monster that can act and isn't a peasant (`restEncounterCanSpawn`). Waits never roll one.
- When a wait ends everyone wakes.
- The day changes at 3 am by MM6's rule (`MM6NewDay.h`). Reputation decays by 1% a day, truncated, clamped to plus or
  minus 1500, and days without rest bring weakness, hunger, death and insanity. `gameplay.fix_new_day_at_month_start`
  (default on) starts a new day when a jump crosses 3 am into the next month, which vanilla skipped.
- `mm6::restAndHealDetachedParty` applies a full rest to a party copy for the border confirmation.

## Source

- [MM6House.h](../src/GUI/UI/Houses/MM6House.h), [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp),
  [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp), [MM6ShopPrices.h](../src/GUI/UI/Houses/MM6ShopPrices.h),
  [MM6ShopStock.h](../src/GUI/UI/Houses/MM6ShopStock.h), [MM6Guild.h](../src/GUI/UI/Houses/MM6Guild.h),
  [MM6SkillGuild.h](../src/GUI/UI/Houses/MM6SkillGuild.h), [MM6Temple.h](../src/GUI/UI/Houses/MM6Temple.h),
  [MM6Training.h](../src/GUI/UI/Houses/MM6Training.h), [MM6Tavern.h](../src/GUI/UI/Houses/MM6Tavern.h),
  [MM6Transport.h](../src/GUI/UI/Houses/MM6Transport.h), [MM6TransportRules.h](../src/GUI/UI/Houses/MM6TransportRules.h)
- [MM6HouseServiceData.h](../src/GUI/UI/MM6HouseServiceData.h), [HouseTable.cpp](../src/Engine/Tables/HouseTable.cpp),
  [MM6StreetNpcTalk.h](../src/GUI/UI/MM6StreetNpcTalk.h), [MM6NpcDialogue.h](../src/GUI/UI/MM6NpcDialogue.h)
- [MM6HouseExit.h](../src/Engine/MM6/MM6HouseExit.h), [MM6ShopStockFields.h](../src/Engine/MM6/MM6ShopStockFields.h),
  [MM6ShopGuildClockFields.h](../src/Engine/MM6/MM6ShopGuildClockFields.h),
  [MM6ShopRefreshBoundary.h](../src/Engine/MM6/MM6ShopRefreshBoundary.h),
  [MM6StreetNpcPortraits.h](../src/Engine/MM6/MM6StreetNpcPortraits.h), [MM6PartyRest.h](../src/Engine/MM6/MM6PartyRest.h),
  [MM6NewDay.h](../src/Engine/MM6/MM6NewDay.h)
- Unit tests: [src/GUI/UI/Houses/Tests](../src/GUI/UI/Houses/Tests), `MM6HouseServiceData_ut.cpp`,
  `MM6HouseType_ut.cpp`, `MM6HouseExit_ut.cpp`, `MM6Shop*_ut.cpp` in `src/Engine/MM6/Tests`
- Game tests: `GameTests_MM6Houses.cpp`, `GameTests_MM6HouseExit.cpp`, `GameTests_MM6Transport.cpp`,
  `GameTests_MM6Hirelings.cpp`, `GameTests_MM6NpcDialogue.cpp`, `GameTests_MM6StreetNpcs.cpp`,
  `GameTests_MM6FirstCombat.cpp` (rest) in [test/Bin/GameTest](../test/Bin/GameTest)

See also: [Events](mm6-events.md), [Travel and maps](mm6-travel-and-maps.md), [Save format](mm6-save-format.md), [Media and UI](mm6-media-and-ui.md), [GrayFace](mm6-grayface-patch-reference.md)
