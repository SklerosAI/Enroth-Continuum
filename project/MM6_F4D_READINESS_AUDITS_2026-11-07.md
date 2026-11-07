# MM6 F4d/F4c readiness audits — 2026-11-07

Three read-only audits run against `9883de1b7` to prepare parallel code lanes.
Static source and packet review only. IDs, counts and source references, no
game text. Lane assignments that followed are in
[SESSION_HANDOFF_2026-11-07.md](SESSION_HANDOFF_2026-11-07.md).

## Houses, shops, items, inventory, equipment

Live in MM6:
- House entry opens the room movie or a static proprietor portrait, then
  `createHouseUI` builds the display-only `GUIWindow_MM6HousePresentation`
  with one Exit button (`src/GUI/UI/UIHouses.cpp` ~295-326, 669-674). A missing
  icon or movie payload throws `std::runtime_error` inside an event.
- Direct and item-on-item use of herbs, bottles and potions is blocked
  (`MM6ConsumableRules.h`, `Character.cpp:3376`, `UIPopup.cpp:2066`). Wands
  throw. Event gold and item compare and write paths run.
- Equip has no MM6 branch, so inherited MM7 rules run.

Unreachable or test-only: transport menu and departure, guild spellbooks
(throws), training, temple, bank, tavern and shops. Shop purchase, refresh and
raw cursor publication helpers have no non-test caller.

Save constraints (`MM6V5LiveSaveCapture.cpp`): inherited MM7 shop arrays must
be empty, `captureBoundedInventory` rejects slot changes and record add/remove
(so equip, buy, sell and pickup cannot save yet), the held item must match the
raw HELD owner, capture only accepts `oute3.odm`.

Gaps in route order: F4b save/load route (dependency), house entry hardening
(S), service dispatch (M), temple heal and training (M, price fields
unresolved, MM6 houses 79-88 must not reach the MM7 training table), INVR equip
and move transitions (M), cursor and HELD sync (M), shop stock, price, buy and
sell (L), guild stock, transport, tavern and bank (L).

Stale packets: house service gate, entry preflight and service data packets
describe an entry that returns unavailable. Checklist service collisions are
unreachable. `MM6_MANUAL_SERVICES_CLAIMS_F2.md` describes an MM7 load path
that now returns empty. Two inventory packets cite an obsolete ITEM v1 125
limit (v5 uses INVR v2 with 138 slots).

The `mm6-f4-shop-save-audit` branch was fully integrated and has been retired.

## Events, NPCs, quests, indoor maps, travel, transport, calendar

Live MM6 event variables (`EvtInterpreter.cpp:432-698`): quest bits 16, item
in hand 17, gold 21, day of year 19, skills 56-86, conditions 87-104 (36 writes
guarded), map bytes 105-175, autonote 205, character bit 213, flying 222,
month 226. Refused: resistances 46-55 (53 uses) and IDs 214-216, 223-225 (80
uses). IDs 1-45 run through the MM7 executor with range checks only. Opcodes
12/24 are MM6 outdoor facet ops, 26 is an MM6 question, 20/27/28 are no-ops,
11/23 use the MM6 indoor face index, 39/40 go through `MM6NpcEventState`, and
21/34/41 still use MM7 rules.

No cross-map move works live:
- `startMapTransition` (`Engine.cpp:1785`) autosaves and MM6 `saveGame`
  throws (`SaveLoad.cpp:240`).
- `getTravelDestination` returns nothing for MM6 (`Outdoor.cpp:291-295`).
- Nothing writes the departing map's delta for MM6.
- Indoor maps decode (`Indoor.cpp:281-331`) but load only in test fixtures.
- `isMapIndoor` uses MM7 ranges, so MM6 outdoor maps 7 and 8 would take the
  indoor loader (`Engine.cpp:744,765`).
- The inherited foot-travel handler heals with zero food.
- `UITransition.cpp:126-160` is MM7-shaped.
- House NPC dialogue is unreachable (presentation-only window).

Route gaps: cross-map autosave (M, collides with F4b), departing-map store
with VIST-style indoor and outdoor records (L, collides), `isMapIndoor` (S),
border travel (M, collides), indoor transition dialog (M), house NPC dialogue
(M), remaining variables and opcodes (M-L), transport (M), calendar start year
and stale month cache (S).

Stale claims: the checklist engine table predates the MM6 indoor reader, the
MM6 outdoor delta path and the opcode 12/20/27/28/26 handling.
`MM6_F4_VARIABLE_GUARD.md` is superseded for the live families.
`MM6_F4_TRANSPORT_MANIFEST.md` describes a menu MM6 never builds.
`MM6_F4_V5_LATER_OUTDOOR.md` overstates world-load coverage.

## Spells, combat, monster AI

Live MM6 branches: monster loader with typed attack labels, projectiles,
spell IDs, special attacks and six resistances (`Monsters.cpp:374-604`),
encounter resolution, attack preference, all-friendly monster relations,
`rules/spells.json` mana and recovery in `castSpell`.

Fail-closed guards: `requireVerifiedSpellEffect` throws for every spell 1-99
at the player queue (`CastSpellInfo.cpp:147, 3085`), event casts
(`Spells.cpp:569`), spell damage (`Spells.cpp:836`) and monster casts
(`Actor.cpp:223, 1266, 1344`). Only Physical and Fire damage pass. Monster
missiles throw (`Actor.cpp:738`), special attacks throw (`Character.cpp:1531`),
scrolls, books and wands throw.

Implemented but unreachable behind the entry throws: First Aid, Power Cure,
timed cures (40, 49, 57, 60, 64, 67), Shield and Heroism. Party death is MM7
hardcoded (`Game.cpp:1661-1720`). AI, pathfinding, turn mode and sprite
impacts have no MM6 branch. No game test casts a spell or fights.

Gaps in route order: monster spell casts (L, 5 of 6 first-map encounter
variants can cast IDs 2, 4, 18, 24, two of which carry wrong inherited
labels), Electricity and Cold damage (M), `SPECIAL_ATTACK_BREAK_ANY` (M),
player cast entry starting with ID 67 (M), party death and respawn (M, needs
F4b).

Contradiction: `MM6_F4_MONSTER_DAMAGE_AUDIT.md` says consumers reject before
mutation, but `DamageMonsterFromParty` checks after `hp -=`, death and
experience, and the special-attack guard throws after `receiveDamage`.
