# MM6 monster spell name and ID routing

Status: F4 title-specific spell-name routing, 2026-11-06. Monster spell effects, targets and graphics remain guarded.

Update, 2026-11-07: monster casts of IDs 2, 4, 18 and 24 now pass the effect guard as single projectiles with original monster damage. See the [spell dispatch packet](MM6_F4_SPELL_EFFECT_DISPATCH.md). Every other monster spell stays guarded.

The F3-matched retained `monsters.txt` has 55 active spell cells using 20 distinct labels. The inherited name parser returned `SPELL_NONE` for 13 cells; 15 more recognized cells resolved to a numeric ID different from the same label in MM6's 99-row spell table. The MM6 loader now uses a bounded title-specific mapping from each of the 20 active labels to its retained MM6 ID. It recognizes the two distinct misspelled names in the shipped monster table. An unknown active label rejects during table loading. MM7 keeps the inherited parser.

The private [spell-effect audit](MM6_SPELL_EFFECT_F2.md) records every active monster join, the retained spell-table source and source hashes. A focused native test loaded all 173 retained monster rows and found 55 nonempty cast IDs. It also checked three differing inherited IDs and the two misspelled labels. This establishes name and ID routing only. `mm6::requireVerifiedSpellEffect` still rejects MM6 monster casts before the inherited effect dispatcher; spell mastery, cast chance/order, resistance, sound and animation need separate MM6 rules.

Next F4 work is to validate the spell-skill values and monster action priority, then implement all 20 active labels' rank-specific effects under the MM6 effect gate with representative retained actors. F5 needs visible and audible enemy casts, combat outcome checks and save/reload. The [monster combat specification](MM6_MONSTER_COMBAT_SPEC.md) and [99-spell effect audit](MM6_SPELL_EFFECT_F2.md) remain the source checklist.
