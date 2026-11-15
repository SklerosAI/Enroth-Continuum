# Claude MM6 UI leftovers lane, 2026-11-12

Branch `claude/mm6-ui-leftovers`, off master `2d1d52c36`. Private trace:
`local-game-data/private-wiki/mm6/UI_LEFTOVERS_PE_TRACE_2026-11-12.md`.

## Commits

1. `f9886a9ba` fix(mm6): guild, bank and stable/boat menus laid out as MM6.exe does (0x49B790, 0x49E460, 0x49CE00,
   buttons 0x498490). Spell guilds serve members only, list Buy Spells plus the learnable skills under a skill cost
   line and teach on click. The bank shows its balance. Stables and boats stack their routes under the price.
   Adds the `titleLines()` tape (DrawTitleText lines with position and colour).
2. `251c60166` fix(mm6): chest exit button at (392,318) with BUTTESC1/2 and the view's side borders (0x41E9AD,
   0x41EA30, 0x40FF44), no more ib-bcu-a placeholder.
3. `8d4f2f61e` fix(mm6): minimap zoom pressed art, BUTTPLUS at (597,43) and BUTTMINU at (503,44) for one frame
   (0x418A48, 0x42F39C, 0x42F457, 0x410330). Not a toggle.
4. `6d39aa12d` test(mm6): DISABLED_FirstOutdoorCapture enters oute3 through party creation input, food 7 and gold
   200 now show.
5. `7cdf2a1bf` test(mm6): DISABLED_HousesCapture covers every New Sorpigal house kind.
6. `7d53fabae` fix(mm6): MM6 NPC profession names from MM6.exe's global.txt id table (0x6BA85C, filled at 0x445D33).
   Buford T. Allman read "the Time in Enroth", now "the Barrister".

## Tests

New: MM6Houses.GuildMenu, BankMenu, StableMenu, NpcHouseProfession, MM6ChestScreen.ExitButton, MM6Hud.ZoomPressedArt.
Each fails with its fix reverted and passes with it. UnimplementedServiceRefuses moved to the temple's Donate,
GuildBuySpellbook gives Zoltan the Elemental Guild award first.

Results at `7d53fabae`: unit 1122 passed (45 skipped as before), full MM6 suite 189 passed and 2 skipped, MM7
controls 147 passed and 2 skipped, check_style clean.

## Open items

- Mercenary (Blades' End) and thieves' (Buccaneers' Lair) guild menus are empty, types 17/18 (0x49C420) not ported.
- Bank Deposit and Withdraw still say "not available yet". Their input screens are traced (0x49E460), not ported.
- The Self guild door could not be clicked from any spot tried, so it has no capture. It uses the same window as the
  Elemental guild.
- Stable fix-prices line visibility quirk (slot 3 reads the next house's route byte) not ported.
- Option focus colour (keyboard focus) not ported for any house.
- "Travel Cost 25 gold" and some names wrap at the 148-pixel column. Not compared against MM6's own wrap.
- `check_tidy` not available locally.
