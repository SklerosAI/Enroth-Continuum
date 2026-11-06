# MM6 spell-guide claim audit: identity and selected scans

Status: **partial F2 claim reconciliation, 2026-11-05**. This packet joins all 99 retained MM6 spell IDs to the owner's Prima guide spell headings and records a small direct visual sample. It does not resolve all spell effects, reopen original archives, run MM6, or close F2. Exact per-ID joins and selected observations remain under ignored `local-game-data/mm6/research-2026-11-05/`.

## Reproducible coverage

`scripts/mm6_probe_spell_guide_claims.py` reads the already retained Prima PDF, its indexed hash, and `spell-effect-probe.json`. It writes an ignored `spell-guide-claims-probe.json` with a guide PDF page and current engine comparison for each of the **99 spell IDs**. All 99 have a guide heading: 98 normalized names agree with the retained table, and one OCR heading was corrected from a directly rendered page. One decorative glyph also required a scan-checked heading locator. This is **identity and locator coverage**, not all 99 spell mechanics checked.

Eleven PDF pages were rendered and visually inspected: the first page of each of the nine school blocks, plus two pages with OCR exceptions or high-risk collisions. This directly covers **17 spell entries**. Their guide casting costs agreed with the retained MM6 table in all 17; **four** differ from the inherited static MM7 spell-data cost at the same numeric ID. The selected scans also corroborate that a matching ID or label can lead to a different effect or target in the inherited dispatcher. Private notes record the selected IDs, values and page locators; no guide passage is copied here.

The scan establishes the following **PDF page starts** for the nine schools: Fire 100, Air 106, Water 112, Earth 118, Spirit 124, Mind 131, Body 137, Light 143, Dark 149. These correct the approximate printed-page ranges in the earlier [spell effect audit](MM6_SPELL_EFFECT_F2.md), which end one page early for several later schools. Use the per-ID private PDF locator instead of the earlier range when reviewing a claim.

## Engine consequence and remaining work

The guide and retained table are evidence for intended MM6 identity, cost and descriptive behavior, while `src/Engine/Spells/Spells.cpp` still leaves MM7 static `pSpellDatas` in place and `src/Engine/Spells/CastSpellInfo.cpp` routes by inherited numeric IDs. The [spell effect audit](MM6_SPELL_EFFECT_F2.md) already documents scroll/book, event, monster and targeting collisions. This new locator lets F2 reviewers tie each such collision to its actual guide page.

Before F3, **82 unscanned spell entries** still need direct page-image review and each of the 99 needs a claim-level comparison for target, Normal/Expert/Master effect, duration, damage, restrictions, recovery and guide-versus-table disputes. The PDF OCR is a search aid; its values must not be treated as verified without scan inspection. F3 can confirm the original shipped table and executable constants in the planned single read-only pass. F4 then needs title-specific spell data and dispatch for all 99 IDs, with F5 visible and audible checks in every school and durable save/reload checks. The [F2 gate review](MM6_F2_GATE_REVIEW.md) remains open.
