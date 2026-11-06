# MM6 spell damage source tags

Status: F4 typed table-source seam, 2026-11-06. Spell damage calculation and live effects remain gated.

The F3-matched retained `spells.txt` has 99 numbered rows with seven damage-tag labels: 50 `none`, seven Physical, nine Fire, four Electricity, three Cold, four Poison and 22 Magic. The inherited damage-school map recognized Fire and Magic, but treated the eleven Electricity, Cold and Poison rows as Physical. The MM6 loader now preserves the source label separately as `SpellInfo::mm6DamageTag`, rejecting an unknown label. MM7 clears that optional field and keeps its existing table behavior.

This changes source retention only. The inherited `SpellInfo::damageType` is still a provisional value for MM6, and `mm6::requireVerifiedSpellEffect` continues to reject all regular MM6 spell effects before inherited dispatch. A focused native test loaded all 99 retained rows and matched the seven tag counts. The original numeric reduction, projectile element, target, immunity and graphics contracts remain unproved.

Next F4 work should use the typed tag with each of the 99 reviewed spell effects, keeping non-damage spells distinct from Physical damage and reconciling Magic and Energy against the six monster-resistance columns. Test all 99 IDs and three mastery ranks with source-backed expected outcomes before narrowing the effect gate; F5 then checks visible and audible casts plus save/reload. See the [spell-effect audit](MM6_SPELL_EFFECT_F2.md) and [monster damage packet](MM6_F4_MONSTER_DAMAGE_AUDIT.md).
