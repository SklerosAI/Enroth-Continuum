# MM6 monster and combat specification (F2)

Status: **retained-table and source-parser audit, 2026-11-05**. This is a
public, asset-free gap register. The owner data and detailed per-row results
remain ignored locally. It does not establish correct combat or any playable
MM6 encounter. The consolidated F3 original-file preflight has not happened.

## Structural evidence and current consumers

The retained `monsters.txt` has 173 numeric records of 33 fields. The retained
`dmonlist.bin` probe has 173 fixed 148-byte descriptions, and
`TableSerialization.cpp` selects `MonsterDesc_MM6` by title. This is bounded
structural coverage, not proof of an ID-to-sprite or behavior mapping.

`MonsterStats::Initialize` in `src/Engine/Objects/Monsters.cpp` rearranges
MM6 columns into an MM7-oriented runtime structure. It then calls shared
attack, projectile, spell, special-attack and treasure parsers. A retained
label-to-parser comparison found these aggregate mismatches:

| Active MM6 field family | Source-level result | Required resolution |
| --- | --- | --- |
| Attack damage type | 49 of 212 nonzero attack-type cells use labels that the shared parser silently converts to physical. Twelve `Ener` cells explicitly map to Earth under an inherited legacy-bug comment. Sixty-one monster rows have at least one affected attack tag. | Define MM6's own elemental, magic, energy and physical damage taxonomy; decide whether the original energy behavior is intended or a bug to preserve. Test damage and resistance together. |
| Missile/projectile | 46 of 82 nonzero missile labels become `NONE`. | Map each active MM6 projectile label to a spawned, rendered and colliding projectile with the right damage/effect. |
| Monster spells | 13 of 55 nonzero spell cells (ten distinct names) become `SPELL_NONE` under the inherited parser. A matching name still may have a different MM6 spell ID/effect. | Resolve every active monster spell to the MM6-specific 99-spell vocabulary and mastery/target behavior. |
| Special attacks | 11 of 93 nonzero labels become `NONE` because MM6 names or abbreviations miss inherited prefixes. | Define every active special attack, save and condition effect, and immunity interaction. |
| Treasure | The active treasure category suffixes are recognized lexically. | Verify generated items and fixed drops against MM6 item IDs, category ranges, probabilities and save/revisit behavior; the shared generator still uses MM7 assumptions. |

These are parser/inventory comparisons, not observed attack selection or
damage. A silent default is not an acceptable F4 policy for an active label.
The detailed comparison and source hashes must remain in an ignored local
report; the public checklist may retain aggregate counts and open semantics.

## Title identity and behavior gaps

- `src/Engine/Objects/MonsterEnumFunctions.h/.cpp` derives family, tier,
  peasant status, arena/bounty eligibility, supertype and fixed drops from
  MM7 numeric monster groups. At least the first retained MM6 family differs
  from the inherited MM7 first family, so same-sized groups do not establish
  compatible meanings. `Actor.cpp` consumes those classifications for combat
  and drops.
- `HostilityTable::InitializeMm6` currently sets every monster-to-monster
  relation friendly while faction behavior remains unmapped. `Actor.cpp`
  consumes the table during target selection. This is an explicit temporary
  default, not MM6 faction AI.
- `Actor.cpp` applies MM7 elemental resistance switches and has no dedicated
  `DAMAGE_MAGIC` or `DAMAGE_ENERGY` branch. The MM6 column adapter currently
  aliases several non-elemental resistance channels to one magic column.
  Establish MM6's actual resistance and immunity semantics before combat
  balance can be assessed.
- The shared spell and item systems are still MM7-oriented. Monster casting,
  loot and status attacks depend on the title-specific spell/item/character
  rules documented in the other F2 specifications.

## F2/F3 closure and F4/F5 acceptance

1. In F2, map all active monster IDs to their MM6 family, variant, sprite,
   map-spawn and behavior definitions; compare the guide's combat description
   with the retained table and current code. Classify every active attack,
   projectile, spell, special, resistance, immunity, faction and loot label.
2. At F3, verify the frozen mapping, binary description boundaries and
   cross-table links against the original shipped resources in the planned
   read-only pass. Record detailed mismatches privately.
3. At F4, require zero silent fallback for active labels. Cover melee,
   ranged, caster, status, neutral/faction, flying and loot scenarios; damage,
   resistance and immunity; attack choice in real-time and turn mode; death
   drops and save/re-entry. The MM7 monster behavior remains separate.
4. At F5, inspect real opening and later encounters graphically and audibly,
   including projectile visibility, enemy targeting, combat outcome and
   persisted loot. A successfully parsed row is not proof of combat behavior.

See also: [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md),
[class and skill specification](MM6_CLASS_SKILL_SPEC.md),
[event and campaign specification](MM6_EVENT_CAMPAIGN_SPEC.md), and
[local status](LOCAL_STATUS.md).
