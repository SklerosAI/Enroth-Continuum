# Claude MM6 services lane, 2026-11-12

Branch `claude/mm6-services`, rebased onto master `1d823dca7` (the UI leftovers merge). Board row P4.

## Commits

- `Test identify and repair at the New Sorpigal weapon shop`: test-only, `MM6Houses.WeaponShopIdentify` and `MM6Houses.WeaponShopRepair`.
- `Move gold in and out of the MM6 bank`: Deposit and Withdraw take a typed amount, clamp it to the party's gold or the balance, and move it. `MM6Houses.BankDepositAndWithdraw`.

A first commit, `Teach skills at MM6 guilds` (`c4335545b`, kept as a patch in the lane scratchpad), was dropped in the rebase. The UI lane's `GUIWindow_MM6Guild` on master already teaches magic guild skills, and the coordinator gave the mercenary and thieves guilds to another lane. Its MM6.exe findings for those two guilds are below for that lane.

## Verified against MM6.exe

- Bank (0x49E460, from the UI lane's trace): prompt "%s\n%s" (Deposit or Withdraw, 112 How Much?) at y 146, typed text at 186, the amount clamped to gold or balance with house sound type 2, reaction 81 on deposit. Our buttons stay in place during the prompt, MM6 draws the prompt instead of their lines.
- Shop menus (0x498490): weapon, armour and magic shops list five options, the general store two. No MM6 shop teaches skills, so the board's "learn-skill options at shops" doesn't exist in MM6.
- Mercenary guild (type 18, 0x498EC4, by house 141..145, else the 143 list) and thieves guild (type 17, 0x4991E3, 147/149/151 one list, the others another). Option = 36 + MM6 skill index. 141 sword, axe, spear, staff, leather. 142 and 144 mace, bow, chain, shield, bodybuilding. 145 sword, axe, spear, staff, repair. Others chain, bow, shield, plate, repair. Thieves 147/149/151 dagger, merchant, identify, perception, disarm trap, others leather, diplomacy, identify, perception, disarm trap. Price (0x49C4CD) 250 times value for type 18, 100 for type 17, merchant discount floored at a third. Menu (0x49C7CE) needs the membership award (0x4C3CB8) and CanAct. Learning (0x49C547) checks the class table and the skill byte, then gold: short purse is status 155 plus house sound type 4, success takes gold, sets the skill to 1 and plays house sound type 2. Can't learn or known plays sound 27. No line at all draws "Seek knowledge elsewhere %s the %s\n \nI can offer you nothing further." at (174 - h) / 2 + 138.
- Guild joins (NPC topics 381..397, 0x4994A0): text npctext 138 + k, award 64 + k, price table 0x4C3E10 {100, 100, 25, 50, 50, 25, 50, 50, 50, 50, 50, 50, 50, 50, 50, 1000, 1000}, one option button param 76 (label 0x4978F9). Not ported: our MM6 NPC topic route logs "no verified global event" and does nothing, so a player can't join any guild. The same holds for the expert teachers (topics 200..259, 0x499610) and Janice's Bounty Hunt (399, 0x4A30B0).
- Tavern rent room (0x49F319): takes gold, house sound type 2, leaves the house and posts message 409, whose handler (0x42F31F) rests (hours until dawn + 1) * 60 - minute minutes. Not ported.

## Fixtures

Membership awards set on characters, gold set with `SetGold`, the identify and broken flags cleared or set on the starting longsword.

## Test results

On `5001f152c`: unit tests 1123 passed. Full MM6 suite 211 passed, 2 skipped (the two `MM6ZSaveHeadless` private research tests). `MM6Houses.*` 21 passed. `check_style` clean. `check_tidy` not run, there is no clang-tidy 22 on this machine. No shared MM6/MM7 code changed, so the MM7 controls weren't run. With the bank change reverted, `BankDepositAndWithdraw` fails on the gold and balance tapes. The identify and repair tests are test-only and guard existing code. The lane capture `20261112-204143-mm6-services.png` (first outdoor frame) shows the HUD with 7 food and 200 gold and nothing off.

## Open

Tavern rent room, fill packs, drink and tip, temple donate, guild joins, expert teachers, Bounty Hunt, the inn (New Sorpigal has only the tavern, house 92). The mercenary and thieves guild menus belong to another lane.
