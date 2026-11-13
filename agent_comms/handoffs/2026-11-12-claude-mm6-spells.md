# Claude — MM6 player spells, batch 1, 2026-11-12 evening

- Owner: Claude Code, lane `mm6-spells`, worktree `../enroth-worktrees/mm6-spells`, branch `claude/mm6-spells`.
- Recipient: the lead, for review and merge.
- Branch rebased onto master `7024a995c`. The earlier uncommitted diff was reviewed against MM6.exe and committed in two
  commits.

## Commits

| Commit | What |
| --- | --- |
| `a84a3cffa` fix(mm6): roll monster resistance from MM6's own columns | `mm6::monsterIncomingDamage` (MM6.exe 0x421dc0) for every MM6 damage kind, `SPAWN_DUMMY` clears the MM6 resistances too. `requireVerifiedMonsterDamage` keeps its old set. |
| `3be132960` fix(mm6): cast the verified first batch of MM6 spells | The spells below, the MM6 Fly drop, Protection from Magic not blocking conditions, First Aid recovery, the MM6 skips of the MM7 Fly / Water Walk icons and isg buff icons, 12 game tests. |
| this file | Handoff. |

## Castable now, and where they are verified

| ID | Spell | MM6.exe |
| --- | --- | --- |
| 1 | Torch Light | 0x423021, shared case |
| 2, 4, 13, 15, 18, 28, 35, 45 | Flame Arrow, Fire Bolt, Static Charge, Sparks, Lightning Bolt, Ice Bolt, Magic Arrow, Spirit Arrow | launch 0x4230e1 / 0x424467, push flags 0x422000, damage 0x47f0a0, hit 0x431867, resistance 0x421dc0, impact 0x45c670 |
| 3, 14, 25, 69 | Fire, Electricity, Cold, Poison resistance | 0x4236e3, 0x42439d, 0x424f99, 0x427ebb, shared cases |
| 12 | Wizard Eye | 0x4242f6, shared case |
| 16 | Feather Fall | 0x42465d |
| 19 | Jump | 0x424915 |
| 21 | Fly | 0x424993, drain 0x487a56 |
| 27 | Water Walk | 0x4253d2, shared case |
| 36 | Protection from Magic | 0x426087, reader 0x483884 |
| 46, 51 | Bless, Heroism | 0x4266e4, 0x426c21 |
| 47 | Healing Touch | 0x4268fa |
| 67 | Cure Weakness | already castable |
| 68 | First Aid | 0x427e25 |
| 71 | Cure Wounds | 0x427f79 |

Details and the independent re-check: `local-game-data/private-wiki/mm6/PLAYER_SPELLS_PE_TRACE_2026-11-12.md` and
`PLAYER_SPELLS_2_PE_TRACE_2026-11-12.md` (Git-ignored).

## Still refused, and why

- 31 Town Portal: traced (0x425d3e, town table 0x4C1F98, last-town byte 9473275), but OE keeps no MM6 last-town state
  and the map change path isn't ported. Not guessed.
- 34 Stun: actor+108 and 0x432750 not identified.
- 17 Shield: the private `combat-buffs.json` base is 3600 s, MM6.exe has 3840 s for Bless and Heroism. Shield's own case
  wasn't traced.
- Everything else in the book: not traced in this batch.
- Scrolls: the override rule is traced (level 5 Master, mana zeroed, recovery by the override's mastery, cursed 50%
  still applies, flags 1 or 17). `Character.cpp` still throws on any MM6 scroll before the queue, and the flags source
  wasn't resolved. Not lifted.

## Tests

- On `3be132960`: unit tests 1123 passed. Full MM6 suite 191 passed, 2 skipped (the two `MM6ZSaveHeadless` research
  tests). MM7 controls 147 passed, 2 skipped (`VendorRefreshIntegration` magic guild and weapon shop). `check_style`
  clean. `check_tidy` not run locally (no clang-tidy 22).
- On `a84a3cffa` alone: unit tests 1123 passed, MM6 suite 179 passed, 2 skipped.
- Capture `local-game-data/progress-captures/20261112-190431-mm6-spells.png` is the default first-map render, with no
  spell active. The frame looks right except for the food and gold slots next to the apple and coins, which show no
  digits. That belongs to the HUD lane, not to this branch.
- Revert checks: refusing every spell but 67 fails all 12 `MM6PlayerSpells` tests. Reverting the Fly drop, the
  Protection from Magic rule, First Aid recovery, the Bless 64-minute base or Spirit Arrow damage each fails its test.
  Reverting the resistance reroute throws in `AttackSpells`.
- Fixtures: a new game through the creation screen into oute3, the Sorcerer taught the spell, a `SPAWN_DUMMY` monster
  with 100+ hit points and armor class 20 or less for the attack spells, the first water ahead of the start for Water
  Walk.

## Open items

- Town Portal, scrolls, Stun, Shield, as above.
- The Torch Light radius consumer (MM6 800 x power, OE 1024 outdoors) and MM6's doverlay.bin Fly / Water Walk overlays
  are not ported.
- The shared Water Walk path writes `decorVars[20 * overlayId + 119]`. With overlay 0 that is decor variable 119 in MM6
  as well as MM7. Not checked against MM6 map scripts.
- Healing Touch's recovery cut has no game test. Its formula is in the trace.
