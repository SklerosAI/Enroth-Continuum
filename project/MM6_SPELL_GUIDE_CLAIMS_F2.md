# MM6 spell-guide claim audit: all 99 rank summaries

Status: **guide claims and bounded C++ source screen complete; original behavior and live validation open, 2026-11-05**. This packet joins all 99 retained MM6 spell IDs to the owner's Prima guide spell headings and records a full spell-section page-image pass for identity, base cost, target, damage type and Normal/Expert/Master rank clauses. It does not establish runtime spell effects, reopen original archives, run MM6, or close F2. Exact per-ID joins and observations remain under ignored `local-game-data/mm6/research-2026-11-05/`.

## Reproducible coverage

`scripts/mm6_probe_spell_guide_claims.py` reads the already retained Prima PDF, its indexed hash, and `spell-effect-probe.json`. It writes an ignored `spell-guide-claims-probe.json` with a guide PDF page and current engine comparison for each of the **99 spell IDs**. All 99 have a guide heading: 98 normalized names agree with the retained table, and one OCR heading was corrected from a directly rendered page. One decorative glyph also required a scan-checked heading locator. This is **identity and locator coverage**, not all 99 spell mechanics checked.

All **56 PDF pages** in the spell section were rendered and inspected, including a continuation page after the last Spirit heading. The ignored `spell-guide-claims-matrix-private.json` records each of the **99 entries** with guide page, base cost, target class, damage channel, and Normal/Expert/Master clause summaries alongside the retained MM6 text and inherited enum/static data/case. The 99 visually checked guide **base** casting costs agree with the retained MM6 table; **22** differ from the inherited static MM7 base cost at the same numeric ID. The existing spell-effect probe's 27 cost-triple differences also include mastery-rank changes, so the two counts have different scopes. Some guide rank clauses are qualitative, one spell's rank effects are explicitly unknown in the guide, and one food formula needed a higher-resolution scan to read its division signs. The scanned clauses are intended-behavior evidence, not verified original executable or current C++ behavior. Private notes record exact IDs, values and page locators; no guide passage is copied here.

The visual pass found two guide entries that omit an Expert cost change present in the retained table, one entry whose prose and damage-result cell disagree, and one whose prose describes an object target while its target cell names the party. Comparing all rank summaries with retained spell text identified **four explicit guide/table effect disagreements**: two duration clauses, one item-to-gold Expert/Master yield, and one condition duration. These are **guide/table or guide-internal ambiguities**, not facts to hardcode. The guide explicitly leaves the final Dark spell's rank effects unknown. Each case is identified in the ignored private matrix/notes for the consolidated F3 original-data comparison and later live validation.

The scan establishes the following **PDF page starts** for the nine schools: Fire 100, Air 106, Water 112, Earth 118, Spirit 124, Mind 131, Body 137, Light 143, Dark 149. These correct the approximate printed-page ranges in the earlier [spell effect audit](MM6_SPELL_EFFECT_F2.md), which end one page early for several later schools. Use the per-ID private PDF locator instead of the earlier range when reviewing a claim.

## Bounded C++ consumer comparison

All 99 now have normalized guide rank summaries. Across the 297 Normal/Expert/Master cells, the private matrix marks **207 quantitative**, **87 qualitative** and **3 explicitly guide-unknown** clauses. The ignored matrix also records, for every ID, the inherited cast branch or direct path, static mana/recovery row, target-route locator, damage-tag loader, source-level effect assessment, and an F4/F5 acceptance check. The 17 prior source hashes still match this checkout; the matrix fingerprints the additional C++ files inspected. No original game archive was reread or partial MM6 launched.

| Source facet, 99 IDs each | Conflicting | Candidate | Absent | Unresolved |
| --- | ---: | ---: | ---: | ---: |
| Same-ID dispatch and MM6 rank access | 55 | 44 | 0 | 0 |
| Static Normal/Expert/Master mana | 27 | 72 | 0 | 0 |
| Static or source recovery rule | 2 | 1 | 0 | 96 |
| Target selection | 20 | 47 | 0 | 32 |
| Damage or duration claim | 20 | 29 | 11 | 39 |
| Source-level effect | 70 | 27 | 0 | 2 |

These are **facet screens, not verified MM6 spell outcomes**. “Candidate” means an inspected C++ branch or value resembles a limited guide claim, even if another facet conflicts; “absent” means the guide makes no damage or duration claim for that ID; “unresolved” means the current source or guide does not support a safe decision. Dispatch conflicts comprise 49 wrong or absent inherited same-ID labels and six more same-label spells gated at inherited Grandmaster. The source-effect conflicts include 21 same-label cases with directly visible differences in rank targeting, durations, effect magnitudes, projectile counts, damage channel or use limits. Examples span Haste, Meteor Shower, Shield, Fly, Stone Skin, Shared Life, First Aid, Power Cure and Armageddon. The two unresolved effect paths open separate travel or beacon UI flows, whose downstream behavior was not traced here.

The guide and retained table are evidence for intended MM6 behavior, while `src/Engine/Spells/Spells.cpp` still leaves inherited `pSpellDatas` in place and `src/Engine/Spells/CastSpellInfo.cpp` routes numeric IDs through inherited cases. `pushSpellOrRangedAttack` in the latter file supplies some target-selection evidence; `src/Engine/Objects/Actor.cpp` supplies the projectile-hit follow-through for the examined stun case. The [spell effect audit](MM6_SPELL_EFFECT_F2.md) documents the separate scroll/book, event and monster paths. A same-name case or matching damage formula cannot establish recovery, resistance, restrictions, success/failure handling, visual/audio mapping or persistence. The private matrix lists the exact source locator and F4/F5 checks for each ID.

## Remaining decisions

The guide page image, retained table and C++ source must be evaluated together; OCR excerpts are search aids. F3 must confirm original shipped constants and adjudicate guide/table disagreements in its planned single read-only pass. F4 needs MM6-specific definitions and dispatch for all 99 IDs, including full recovery and downstream UI/projectile checks. F5 needs representative visible and audible casts in every school plus save/reload checks for persistent effects. No spell has been accepted as correct at runtime. The [F2 gate review](MM6_F2_GATE_REVIEW.md) remains open.
