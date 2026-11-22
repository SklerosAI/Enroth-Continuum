# MM6 rules from MM6.exe

How the MM6 port reads rule tables out of the original `MM6.exe` through `mm6::Executable`, which tables come from
it, and which rules live in source instead.

Some MM6 rules live only in the original executable's data sections, not in any `.txt` table of the LOD archives:
class creation arrays, spell costs and recovery, house room descriptors, transport routes and shop stock rules. The
port reads them from the player's own `MM6.exe` at startup or first use. Nothing extracted from the executable is
committed to the repository.

There is no rules JSON any more. The port used to read `rules/*.json` from the MM6 profile, filled from a private
extraction. That folder, the `--rules-path` option, the `OPENENROTH_MM6_RULES_PATH` and `MM6_RULES_PATH` variables
and the Python extraction scripts are gone. Every value the JSON held was checked equal to the executable before the
switch, except the two noted under [Rules kept as source constants](#rules-kept-as-source-constants).

## Accepted executable

Only one build is accepted: `MM6.exe` of the GOG release of Might and Magic VI, version 1.2, SHA-256
`b804fda03f56c38cfeecd1a620ac97c2eeebe916e3048189520b34d43688e3e6`. Any other file, including a patched one, is
refused before a table is read. The error names the file and both hashes. The port carries the GrayFace patch's
fixes as `gameplay.fix_*` flags instead of reading a patched executable, see
[GrayFace patch reference](mm6-grayface-patch-reference.md).

The executable is looked up as `mm6.exe` in the MM6 data folder through the data filesystem, which is
case-insensitive. It is not on the startup data-path validation list (`mm6ValidateList` in `PathResolver.cpp`), so a
missing or wrong executable surfaces as an exception at engine initialization, when the creation and spell rules
load.

## How it reads

`mm6::Executable` (`src/Engine/MM6/MM6Executable.h`) takes the whole file as a `Blob`:

1. It hashes the file with `sha256` from `src/Utility/Sha256.h` and compares it to the GOG 1.2 hash.
2. It checks the `MZ` and `PE\0\0` signatures and that the optional header is PE32.
3. It reads the image base and the section table. A section maps `[imageBase + virtualAddress, + size)` to its file
   offset, where size is the smaller of the virtual and raw sizes. Bytes past the raw size are zero-filled at load
   and are not in the file.
4. Readers `u8`, `i16`, `i32` and `u32` take a virtual address, the same address the original code uses, and throw
   when it isn't backed by file bytes. `string` reads a zero-terminated string and returns `std::nullopt` for an
   address in a runtime-filled buffer.

Addresses in the decoders are the original's virtual addresses, so they can be compared directly with a
disassembly or with GrayFace's MMExtension structure definitions, where the table locations were first found.

`mm6::executable()` reads and checks the file once and keeps it for the process. Decoders take a
`const Executable &` so unit tests can pass their own instance.

## Tables read from the executable

| Table | Address | Shape | Decoder | Loaded |
|---|---|---|---|---|
| HP base by class family | `0x4C2630` | 6 bytes | `mm6::loadCreationRules` in `Character.cpp` | Engine init |
| SP base by class family | `0x4C2638` | 6 bytes | same | Engine init |
| HP factor by class | `0x4C2640` | 18 bytes | same | Engine init |
| SP factor by class | `0x4C2654` | 18 bytes | same | Engine init |
| Starting stats by family | `0x4C2668` | 6 × 7 bytes | same | Engine init |
| Starting skills by family | `0x4C2694` | 6 × 31 bytes, skill codes 0 to 3 | same | Engine init |
| Spell cost and recovery | `0x4BDD70` | 100 rows of 14 bytes, row 0 unused | `mm6::decodeSpellRules` in `MM6SpellRules.cpp` | Engine init |
| House room descriptors | `0x4BE888` | 119 rows of 16 bytes | `mm6::decodeRoomDescriptors` in `MM6RoomDescriptors.cpp` | First use of the house data |
| Transport routes | `0x4C3F20` | 36 rows of 32 bytes | `mm6::decodeTransportRules` in `MM6TransportRules.cpp` | First use |
| Transport house slots | `0x4C43A0` | 21 houses × 3 route indices, houses 48 to 68 | same | First use |
| Shop stock rows | `0x4C433C`, `0x4C4546`, `0x4C459C` | Per shop: treasure level and item kinds, two banks | `mm6::decodeShopStockRules` in `MM6ShopStock.cpp` | First use |
| Shop visible slots | `0x4C45F1` | 4 bytes, by house type | same | First use |
| Guild spellbooks | `0x4C48B0` | 22 int16, spell guilds 119 to 140 | same | First use |
| Speech voice lines | `0x4C22F0` | 103 rows of 8 bytes, lines in bytes 0 and 1 | `mm6::decodeVoiceRules` in `MM6VoiceRules.cpp` | First speech |
| Voice line takes | `0x4C20D0` | 45 lines × 12 voices | same | First speech |

Row layouts:

- Spell row (14 bytes): Normal, Expert and Master spell-point cost as int16 at +0, +2, +4, then Normal, Expert and
  Master recovery as int16 at +6, +8, +10, then a flags word at +12 that the port doesn't read.
- Room descriptor (16 bytes): pointer to the movie name at +0, background at +4, NPC picture at +8 (int32), house
  type at +12, sound group at +13. Room 0's name pointer leads into a runtime buffer, so room 0 has no movie. The
  houses reference rooms 1 to 118.
- Transport route (32 bytes): map index at +0, seven weekday flags at +1, duration in days at +8, x, y, z as int32 at
  +12, yaw at +24, required quest bit at +28. The map index is the one-based position in the original `Games.lod`
  directory order. The retained `.entries` name list has the same names in a different order and gives wrong maps.
- Shop stock: shops 1 to 14 (weapons) have one row, shops 15 to 28 (armor) two, the second for slots 4 and up, shops
  29 to 41 (magic) a treasure level only, shops 42 to 47 (general stores) a level and six item ids where 0 means a
  generated item. The second bank's tables sit 540 bytes after the first.

Class families are the six starting classes, a class id divided by 3. Every family's starting stats sum to 73, and
every family has two skills granted (code 1) and nine to choose from (code 2). Those are pinned by the
`MM6Executable.CreationArrays` unit test. What the creation arrays mean in play is on
[Characters, items and spells](mm6-characters-items-spells.md). Rooms, shops and transport are on
[Houses and services](mm6-houses-and-services.md) and [Travel and maps](mm6-travel-and-maps.md).

## Reaching the rules

| Rule | Call |
|---|---|
| Creation arrays | `mm6::creationRules()` after `mm6::loadCreationRules`. Throws if not loaded |
| Spell cost and recovery | `mm6::spellManaRecovery(spellId, mastery)` for spells 1 to 99 and Novice to Master. The cost comes from `spells.txt` while `gameplay.fix_spells_cost` is on |
| Rooms | `mm6::decodeRoomDescriptors(mm6::executable())`, used by `MM6HouseServiceData.cpp`, and `mm6::selectHouseRoom` |
| Transport | `mm6::transportRules()`, then `offeredRoutes`, `transportPrice`, `transportDepartureDelayMs` |
| Shop stock | `mm6::shopStockRules()`, `mm6::shopVisibleSlots` |

Creation and spell rules load eagerly in `Engine::MM7_Initialize` when the title is MM6. The house tables load
lazily into function-local statics.

## Rules kept as source constants

Some rules sit in the executable's code, not in a table, as immediate operands. The port keeps them as constants in
source with a comment naming the original address:

| Rule | Value | Source | Original |
|---|---|---|---|
| First Aid heal | 5, 7, 10 by mastery | `MM6FirstAidRules.cpp` | `0x427E25` |
| Power Cure heal | 10 + 2 × skill, every mastery | `MM6PowerCureRules.cpp` | `0x428591` |
| Condition cure window | Seconds per skill point, see below | `MM6TimedCureRules.cpp` | `0x426355`, `0x426B01`, `0x427534`, `0x427741`, `0x4279FC`, `0x427D8C` |
| Bless, Heroism and Shield duration | 64 min + 5 min per skill point, 15 min at Master | `blessAndHeroismDuration` in `MM6PlayerSpell.h` | `0x42472A` (Shield), `0x426C21` (Heroism) |
| Heroism power | 5 + skill | `MM6CombatBuffRules.cpp` | `0x426C21` |
| Skill bonuses to attack, damage, shooting, armor class, HP and SP | See [skill bonuses](mm6-characters-items-spells.md#skill-bonuses) | `MM6SkillBonus.h` | `0x483930` |

The condition cures give Stone to Flesh and Remove Curse 180, 3600 and 86400 seconds per skill point at Novice,
Expert and Master. The original gives Remove Fear, Cure Paralysis, Cure Insanity and Cure Weakness 180, 10800 and
259200, three times longer than their descriptions. `gameplay.fix_condition_spells`, on by default, uses the
described windows for those four, as the GrayFace patch does.

The JSON these constants replaced was wrong in two places. It gave the buffs 3600 base seconds, copied from the printed
"1 hour", where the executable adds 3840 (64 minutes). It also gave all six condition cures the shorter window.

The spell table and `spells.txt` disagree on one cost out of 297: Static Charge at Expert. The executable gives 2/2/0
by mastery, row 13 at `0x4BDD70 + 14 * 13`. `spells.txt` gives 2/1/0, and its Expert text says "Casting cost 1 spell
point". The GrayFace patch copies every `spells.txt` cost over the table at startup (C12).
`gameplay.fix_spells_cost`, on by default, does the same: `mm6::spellManaRecovery` then takes the cost from
`spells.txt` and only the recovery from the executable.

## Not from the executable

Most MM6 data comes from the text tables and binary files in the LOD archives, read through the inherited table
loaders with MM6 branches: `items.txt`, `stditems.txt`, `spcitems.txt`, `rnditems.txt`, `potion.txt`, `potnotes.txt`,
`useitems.txt`, `spells.txt`, `class.txt`, `skilldes.txt`, `stats.txt`, `awards.txt`, `autonote.txt`, `quests.txt`,
`npc*.txt`, `proftext.txt`, `2devents.txt`, `mapstats.txt`, the map `.evt` and `.str` files and the monster tables.
Spell costs exist both in `spells.txt` and in the executable table. The original's cast path reads the executable
table. The port reads `spells.txt` through `mm6::decodeSpellCosts` unless `gameplay.fix_spells_cost` is off.

## Tests

Every decoder has a unit test that runs it over the real executable. These tests need `OPENENROTH_MM6_PATH` set to
the MM6 data folder and skip without it. `MM6Executable.RejectsOtherFiles` also flips one byte of the real file and
checks the refusal.

## Source

- [MM6Executable.h](../src/Engine/MM6/MM6Executable.h), [MM6Executable.cpp](../src/Engine/MM6/MM6Executable.cpp)
- [MM6CreationRules.h](../src/Engine/MM6/MM6CreationRules.h), loader in [Character.cpp](../src/Engine/Objects/Character.cpp)
- [MM6SpellRules.h](../src/Engine/Spells/MM6SpellRules.h)
- [MM6RoomDescriptors.h](../src/GUI/UI/MM6RoomDescriptors.h)
- [MM6TransportRules.h](../src/GUI/UI/Houses/MM6TransportRules.h)
- [MM6ShopStock.h](../src/GUI/UI/Houses/MM6ShopStock.h)
- [MM6FirstAidRules.cpp](../src/Engine/Spells/MM6FirstAidRules.cpp), [MM6PowerCureRules.cpp](../src/Engine/Spells/MM6PowerCureRules.cpp),
  [MM6TimedCureRules.cpp](../src/Engine/Spells/MM6TimedCureRules.cpp), [MM6CombatBuffRules.cpp](../src/Engine/Spells/MM6CombatBuffRules.cpp)
- [Sha256.h](../src/Utility/Sha256.h)
- Tests: [MM6Executable_ut.cpp](../src/Engine/MM6/Tests/MM6Executable_ut.cpp),
  [MM6SpellRules_ut.cpp](../src/Engine/Spells/Tests/MM6SpellRules_ut.cpp),
  [MM6RoomDescriptors_ut.cpp](../src/GUI/UI/Tests/MM6RoomDescriptors_ut.cpp),
  [MM6TransportRules_ut.cpp](../src/GUI/UI/Houses/Tests/MM6TransportRules_ut.cpp),
  [MM6ShopStock_ut.cpp](../src/GUI/UI/Houses/Tests/MM6ShopStock_ut.cpp),
  [MM6TestData.h](../test/Testing/Unit/MM6TestData.h)

See also: [MM6 characters, items and spells](mm6-characters-items-spells.md), [MM6 houses and services](mm6-houses-and-services.md),
[MM6 travel and maps](mm6-travel-and-maps.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md),
[Source and privacy](source-and-privacy.md)
