# Claude MM6 guild menus and UI checks lane, 2026-11-12

Branch `claude/mm6-guild-menus`, off master `1d823dca7`. Private trace:
`local-game-data/private-wiki/mm6/GUILD_MENUS_PE_TRACE_2026-11-12.md`.

## Commits

1. `1679b387f` Teach skills at MM6 guilds. Blades' End and Buccaneers' Lair (types 18 and 17) list their five skills
   by house id (0x498EC4 table 0x499390, 0x4991E3 table 0x4993A4) to members (award table 0x4C3CB8) who can act,
   under "Skill Cost". Price 250 x value at a mercenary guild, 100 x value at a thieves' guild (0x49C4CD, the
   `and dl, 106` only touches the low byte), merchant discount floored at a third. Nothing left to learn: 544 +
   528 in yellow with global.txt's class name. Learn: room sound 2, short purse: 155 + room sound 4, can't learn or
   known: sound 27 (0x49C547). New window `MM6SkillGuild`, shared helpers in `MM6Guild`. This is the services lane's
   c4335545b reconciled with master. That patch was made before master had the spell guild menu and did not apply
   (MM6Guild.{h,cpp} add/add, conflicts in MM6House and the tests), so its mercenary and thieves half was redone on
   master's guild code. Its thieves price (100) and yellow "nothing" text were right, my first reading (356, white)
   was wrong. Its magic guild half is already on master.
2. `a232a0d9e` test(mm6): enter the Self guild through its door. Face 2801 (event 25), wall y -584, ground 576.
   Space from (-13952, -684, 576) facing north enters house 139. Added to the houses capture. The message's line
   about which faces the earlier lane tried is a guess, the event 26 faces are 64x64 faces at x -13340 about
   1000 units south of the door, not high on the same wall.
3. `81f37f100` fix(mm6): focus colour on house menu lines. MM6 starts the focus on the first option and moves it to
   the line under the mouse every frame (0x40FC79 -> 0x419D70 key 34). Arrows don't reach it in a house (cursor
   mode 13, 0x454720). (225,205,35) at shops, training and NPC topics, (255,255,155) at guilds, temple, bank,
   tavern, stables.
4. `476e458ba` test(mm6): houses capture enters both skill guilds again as a member. `a1b4fbf51` fixes its
   thieves visit, a second entry walked into Suzie the Fool in front of the door.
5. `7220c5f30` fix(mm6): guild membership topics 381..397 (Buford T. Allman, Violet Dawson, Hejaz Mawsil, Harold
   Hess in New Sorpigal). They have no event in MM6's global.evt (scanned, 278 events, 375-380 and 398 present),
   MM6.exe answers them itself (0x4994A0, label 0x4978F9, join 0x496A96). Joining takes the price (table 0x4C3E10)
   and gives award 64 + k to all four characters. Before it the membership NPCs did nothing when asked.

## Verified, no change needed (item 3)

- Text wrap at 148 px: MM6's 0x442F50 uses the same rule as OE's `WrapText` (strict `x + l + w + r < width`).
  "Travel Cost 25 gold" is 169 px in arrus and wraps after "25" in MM6 too. Only a word with no space before it
  differs (MM6 never splits a word and writes '\n' over byte 0). No shipped house text was checked for that case.
- Magic shop top shelf y = max(0, 152 - h) in both the buy (0x49AA93) and special (0x49B51D) screens, blitted into
  the frame buffer after the borders, so a tall staff starts at row 0 over the top frame. The capture shows that.
- Grey band under the sell/identify/repair grid: MM6's LEATHER icon is grey stone, and LEATHER (8,8) with fr_inven
  (14,17, 449x289) gives that band in MM6 too (composed from the icons).
- Selection ring: 0x4353F0 draws the aframe1 sprite centred on x 50/163/276/388, bottom row 467, every frame for the
  active character, before the windows. OE draws it, the first-map capture shows the gold oval on Zoltan.

## Tests

New: MM6Houses.MercenaryGuildMenu, ThievesGuildMenu, SelfGuildDoor, OptionFocusColor, GuildMembership. GuildMenu,
BankMenu, StableMenu now expect the focus colour. Each fix's test fails with the fix reverted (Mercenary and
Thieves: no option to click, focus: GuildMenu, BankMenu, StableMenu, OptionFocusColor, membership: no Join line).

Results at `7220c5f30`: unit 1123 passed (45 skipped), full MM6 suite 213 passed (2 skipped), MM7 controls 147
passed (2 skipped, run because `UIHouseEnums.h` is shared), check_style clean. `a1b4fbf51` only touches the
disabled capture test, which passes.

## Captures

`DISABLED_HousesCapture` frames inspected whole: merc and thieves refusal, Self guild, Elemental member menu with
the focused line, stable route in (255,255,155), weapon shop with "Buy Items" in gold, magic shop special stock with
the staff over the top frame, sell grid with the grey band, Buford's house topics. HUD food 7 and gold 200 right in
every frame. Member frames of Blades' End (Skill Cost 375, Spear, Staff) and Buccaneers' Lair (Skill Cost 150,
Merchant, Identify Item, Perception, Disarm Trap). First-map capture shows the selection ring on Zoltan. Copies in
`local-game-data/progress-captures/` (labels `mm6-guild-menus*`).

## Open items

- Skill and mastery teachers (topics 200..259, 0x499610) and Bounty Hunt (399) still take the guarded MM6 topic
  route and do nothing. Not traced here beyond the services lane's notes.
- In a skill guild with nothing left to learn, MM6 keeps the option buttons at their creation rects, so a click on
  the empty panel buzzes (sound 27). Not ported.
- Bank Deposit and Withdraw (services lane), stable price-fixing slot quirk (UI leftovers lane) unchanged.
- `check_tidy` not available locally (no clang-tidy 22).
