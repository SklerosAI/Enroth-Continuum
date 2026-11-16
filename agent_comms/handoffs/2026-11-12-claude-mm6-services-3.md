# Claude MM6 services lane 3, 2026-11-12

Branch `claude/mm6-services-3`, off master `251632a40` (services 2 and guild menus merged). Follows
`2026-11-12-claude-mm6-services-2.md`. Private trace: `local-game-data/private-wiki/mm6/SERVICES_3_PE_TRACE_2026-11-12.md`.

## Commits

- `adc167d5e` Finish MM6 guild joins the way MM6.exe does. The join (0x496A96) now sets the award through the award
  variable write (0x4407E0, var 12), so each character plays the award chime and reaction, clears the recruiter's third
  topic (record +44, where all 34 MM6 recruiters keep the membership topic) and plays speech 86 on the active
  character. `MM6Houses.GuildMembership` tapes Harold Hess's topics, the four chimes and the portraits.
- `c6485b4f4` Tip the barkeep at MM6 taverns. Have a Drink sets a per-tavern drink bit (house - 92). Tip refuses with
  "Have a Drink first..." without it, takes 1 gold with it, picks the news once per tavern with 0x43BC20 (region = the
  map's 1-based position among the 67 base maps at the front of the Games.lod directory, Oute3 = 40, then first +
  rand() % count of that region's npcnews.txt entries, region 1 when it has none), keeps it, and plays speech 70 only
  on the first pick. The news shows in the dialogue panel until the next drink. Entry 0 is stored as "none" in MM6 and
  shows nothing, ported as is. `MM6Houses.TavernTip`, unit test `MM6Tavern.TipNews`. `UnimplementedServiceRefuses` is
  gone, no service of a plain MM6 house is unimplemented now.
- `9aefadf6e` Teach Master mastery at MM6 master teachers. The odd topics 201 to 259 use the teacher offer with the
  per-skill switch at 0x497198 (full table in the trace doc: rank 8/10/12/7, stats 30/40, promotion class or its
  honorary award, reputation +-1000, fame 200, a blaster in the inventory, Axe free with no check). Prices 0 to 5000.
  `MM6Houses.MasterTeacher` makes Serena a Fire master at Jezebel's (house 455).
- `d946c5429` Hunt bounties at MM6 town halls. Topic 399 (0x4A30B0) rolls a monster per hall per month, pays 100 x
  level with the hireling share, award 81 for all four, reputation + level, bounty count + 1. The three MM6 halls use the
  first three town hall slots of the party's bounty arrays, so the shared `Actor::Die` sets their kill flags.
  `MM6Houses.BountyHunt` asks Janice, kills the named Acidic Ooze outside, collects 1800 gold, asks again.
- `58f9bc976` test(mm6): Jezebel lives on the Misty Islands. The teacher commit's message and test said Castle
  Ironfist. House 455 is on OutE2.odm, the Misty Islands, next to New Sorpigal. Castle Ironfist (OutD3) has Bishop
  Iquisitorio, a Spirit master.

## Verified against MM6.exe and the data

Addresses above and in the commit messages. Data scanned: MM6 `npcdata.txt` (all recruiters, teachers and the three
Bounty Hunt clerks), `npctext.txt` (259..266, 366..370), `npctopic.txt`, `npcnews.txt` (279 entries, contiguous per
map), `monsters.txt`, `awards.txt`, `class.txt`, `items.txt` skill groups, `2devents.txt` (taverns 92..112, town halls
89..91, teacher houses), `mapstats.txt`, `oute2.evt`, and the raw Games.lod directory order. Only MM6 was in scope.
Door of house 455 from a throwaway game test reading oute2 faces (deleted).

Not observable in tests: speech 86 after a join. MM6 voices don't load in OE, and its portrait (smile or wide smile)
was the award reaction's wide smile with the test's random state, so `GuildMembership` can't tell it apart.

## Fixtures

Gold 5000 and Fire 12/11 expert on Serena and the fourth character for the master test. The bounty monster is spawned
as a dummy outside the hall with 1 HP (an Acidic Ooze at 86 HP took 2000+ frames and killed the party first).
Everything else is input.

## Test results

The original lane reported `check_style` clean and individual revert-and-fail checks, but did not record aggregate
suite counts in this handoff. On 2026-11-16, after merging this lane with the completed MM6 menus/hirelings and
rest/time lanes on current MM7 main, a fresh combined build succeeded. Focused integration checks passed:

- 60/60 MM6 gameplay tests across menus, hirelings, houses/services, combat, potions, turn-based play, rest,
  conditions, and party death.
- 26/26 active focused unit tests passed; 14 private-fixture cases skipped because their optional environment inputs
  were not supplied to that unit-test process.
- 6/6 active MM7 movement/combat controls passed.
- 7/7 enabled MM7 vendor-restock and all-artifacts/storage mod checks passed; the disabled-mode inverse check skipped
  as expected while the mod flag was enabled.

The lane's original revert checks: guild join fails on options, topics, chimes and portraits. Tip fails on gold,
status, text, tip, portrait and drawn text. Master teacher fails at the click on the learn line. Bounty hunt fails with
no monster rolled (it asserted in `spawnMonster` before the `ASSERT_NE` was added).

## Open

- Temple blessings above reputation 200: not done. They need MM6's fixed-skill queued cast (0x422000 flags 0x30) cost
  and recovery rule, and Guardian Angel, Day of the Gods, Hour of Power and Day of Protection aren't verified MM6
  player spells. Outline in the trace doc.
- The tavern drink bits, the picked tips and the MM6 bounties aren't in the MM6 save yet (PART has no fields for them).
- `masterTeacherPrice` reads stats with the shared `GetActual*` getters. MM6's getters (0x47D960 ..) add age and item
  bonuses the same way as far as traced, not compared line by line.
- The character screen shows award 81 through MM7's award switch, not checked for MM6.
