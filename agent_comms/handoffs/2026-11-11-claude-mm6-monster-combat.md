# MM6 monster missiles, spells and special attacks handoff

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-monster-combat`, six commits cherry-picked onto `master` as `458688930` (combat sweep), `1f559d1ca` (missiles), `8a464d1aa` (special attacks), `41dca4c5f` (monster spells), `491f95fee` (placed actors' attack bytes), `3b1be67dc` (cleanup).
- Shared code, MM6-gated hand-offs: `Actor.cpp`, `Character.cpp`, `SpriteObject.cpp`, and the monster loader in `Monsters.cpp`. MM6 logic in `src/Engine/MM6/MM6MonsterAttack.h` and the existing MM6 missile, spell and special-attack headers.

## Original behavior (MM6 executable)

- Missiles: each attack column has its own nine missile names, and any other cell is melee. Types 1-9 launch objects 500-580. Robots shoot Energy as object 590. Monster objects fly through monsters. Arrow and energy objects roll to hit and are halved by the Shield buff, Shielding items and two artifacts. Bolts turn into their impact objects, Fireball and Meteor explode in radius 512, and Finger of Death runs the Dead special attack with chance equal to its level.
- Monster spells: projectile spells use the shared object table. Meteor Shower works only outdoors. Dispel Magic ends party and character buffs on a failed save. Mass Curse curses each character who can act. Spell dice are one d2 for the listed spells and one d2 per skill point otherwise. Attack 1's bonus also applies to attack 2 and to spells.
- Special attacks: the cell is matched against 23 names, each with its own save stat, protective items and effect, with a `rand % (save + luck + 30) < 30` failure roll. Two original quirks are kept: Eradication tests Unconscious, and a stolen item is destroyed.
- Placed map actors kept raw MM6 attack bytes, which the combat sweep found. They are now translated like spawned ones.

## Checks

- New `MM6MapSweep.DISABLED_MonstersAttackParty` and `tools/run_mm6_combat_sweep.py`: each monster kind on a map attacks the party in turn. Main-quest maps throwing: 24 of 26 before, 0 after.
- New game tests: archer, skeleton fear, fire archer Fireball, harpy Mass Curse, hive reactor. Revert-and-fail on master's code. On the branch: 17 `MM6CombatHeadless` tests passed alone, MM6 suite 96, first-map gate 28 with the MM7 controls, MM7 in-process 107, full MM7 540/540, unit tests 1,111, `check_style` clean.

## Open

Monster area blasts on other monsters use the shared monster-on-monster path, which hits only non-friendly monsters with an unverified formula. Special-attack face animations, overlays and sounds are not mapped. The Curse and Shield buff checks use raw MM6 buff slots. Placed actors' movement, AI, hostility and preference bytes are still raw. Saves use float stats where the original truncates. The monster AI tree and the 5020 missile range limit are not checked. Paralyze and Toxic Cloud monster casts stay refused, and neither is on the main quest. The hive reactor test makes the reactor hostile by hand, because what turns it hostile in play is unknown.

Privacy: addresses, IDs and map stems only.
