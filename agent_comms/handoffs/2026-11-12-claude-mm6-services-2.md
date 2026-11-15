# Claude MM6 services lane 2, 2026-11-12

Branch `claude/mm6-services-2`, rebased onto master `fd207083a` (after the guild menus merge). Follows
`2026-11-12-claude-mm6-services.md`.

## Commits

- `Rent rooms and fill packs at MM6 taverns`: Rent Room takes the price, leaves the house and rests until 6 am
  without eating food. Fill Packs fills food to the tavern's Val or refuses full packs. `UIMSG_RentRoom` in
  `Game.cpp` takes MM6's sleep length on MM6 (shared file, one branch). `MM6Houses.TavernRentRoom`,
  `MM6Houses.TavernFillPacks`, unit test `MM6Tavern.RoomRest`.
- `Teach Expert mastery at MM6 expert teachers`: even topics 200 to 258 show the teacher's offer and a learn line
  with MM6's checks and prices, learning sets Expert. `MM6Houses.ExpertTeacher` makes Serena a Fire Magic expert at
  Isao Magistrus (house 465) by input.
- `Take donations at MM6 temples`: Donate takes Val, raises reputation by 200 to at most 200 (Temple Baa takes it
  back), thanks the party. `MM6Houses.TempleDonate`.
- `Serve drinks at MM6 taverns`: Have a Drink costs 1 gold, "Hic...", and rolls the got drunk reaction, the drunk
  condition or a stat bonus the way MM6 does. `MM6Houses.TavernDrink`. `UnimplementedServiceRefuses` moved to Tip.

Item 1, the guild membership topics, was dropped on the coordinator's word because the guild lane ported it
(`MM6Houses.GuildMembership`). My version is parked as
`scratchpad/mm6-services-2/parked-guild-join/0001-Join-MM6-guilds-through-their-recruiters.patch`, not committed.
Differences from the merged port, from the same trace (0x496A96): MM6 also clears the NPC's third topic (record
+44) after a join, sets the award through the award variable (0x4407E0, which plays the award sound and the award
reaction for each character), and plays the joined guild reaction (speech 86) on the active character. The merged
port does none of those three.

## Verified against MM6.exe

Everything above, addresses in the commit messages and in the Git-ignored
`local-game-data/private-wiki/mm6/SERVICES_2_PE_TRACE_2026-11-12.md`. Texts checked in MM6 `npctext.txt`,
`npctopic.txt`, `global.txt`, prices against `2devents.txt` and `npcdata.txt` from
`local-game-data/mm6/research-2026-11-05/icons/`. Door positions of houses 465 to 487 come from the oute3 face
events through a throwaway game test (deleted).

Not traced: what msg 113 (the Exit button's message, also posted after a learn) does in a house. The port goes back
to the NPC's topics after a learn, the way the merged guild join does.

## Fixtures

Gold with `SetGold` (2000 for the teacher test), Serena's Fire Magic set to rank 4 for the
teacher test, food set to 2 for fill packs, HP and SP lowered for rent room. Memberships, mastery and everything else
come from input.

## Test results

On the branch rebased onto `fd207083a`: unit tests 1124 passed. Full MM6 suite 234 passed, 2 skipped (the two
`MM6ZSaveHeadless` private research tests). `MM6Houses.*` 31 passed. MM7 controls (`Game.cpp` changed) 147 passed,
2 skipped (the two `VendorRefreshIntegration` tests). `check_style` clean. `check_tidy` not run, there is no
clang-tidy 22 on this machine.

Revert and fail: with each fix's source reverted, its tests fail. Rent room and fill packs fail on gold, food, screen,
clock, HP, SP, status and sounds. The expert teacher test fails at the click on the learn line (checked again after
the rebase onto the guild lane's structure). Donate fails on gold, reputation, status and portrait. Drink fails on
gold, status, sound, drunk and portrait.

Capture `20261112-221247-mm6-services-2.png` (the tool's first outdoor frame in New Sorpigal): HUD shows 7 food and
200 gold, the status line reads "Rest" because the cursor rests on the Rest button, nothing off. It doesn't show
the house screens this lane changed.

## Open

- Master teachers (odd topics 201 to 259): a per-skill switch of class, stat and award checks, not ported, they
  still log "no verified global event". New Sorpigal has none.
- Temple donate blessings: with effective reputation above 200 the right donation of the visit casts Guardian
  Angel, Wizard Eye, Day of the Gods, Hour of Power or Day of Protection. MM6 fixed-skill casts are refused by
  `requireVerifiedPlayerCastSkill`, left as a TODO in `GUIWindow_MM6House::donate`.
- Tip the Bartender: needs the per-tavern drink bit and the rumour picker at 0x43BC20, untraced. Still says "not
  available yet".
- Janice's Bounty Hunt (topic 399, 0x4A30B0): a monthly target per town hall, a kill flag set elsewhere and a
  reward. Traced in outline only, not ported.
- The offers live in the MM6 house window. A guild or teacher topic reached from a conversation outside a house would
  still take the old route. Not checked whether any shipped NPC has one there.
