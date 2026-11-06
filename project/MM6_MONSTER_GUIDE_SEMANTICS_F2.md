# MM6 monster guide catalog and semantic coverage: F2

Status: **retained table, owned guide, and C++ source audit; no encounter run** (2026-11-05). This is a monster-only companion to the [item/monster guide queue](MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md) and [combat semantics](MM6_COMBAT_SEMANTICS_F2.md). Exact per-ID names, guide page/fields, retained rows and parser labels remain in ignored `local-game-data/mm6/research-2026-11-05/monster-guide-semantics-private.json` and its ignored builder/input. No original installation archive was reopened and no MM6 game was launched.

## Identity denominator and evidence strength

The retained table has **173/173 contiguous monster IDs**. The owner guide's catalog describes **57 three-variant groups**, and the retained first **171** IDs form 57 numeric triplets. The guide's first group spans PDF pages 38–39; subsequent groups follow one page each through PDF page 95. Direct page-image checks of PDF 39 and 96 confirmed the first continuation and the page after the final group. The final **two** retained IDs are not entries in this 57-group catalog; their unique/special, sprite and spawn meaning remains open.

This packet directly inspected **19 PDF page images**: 17 group tables on pages **46, 49, 57, 61, 68, 71, 73, 76, 77, 79, 80, 82, 83, 84, 86, 94, 95**, plus continuation/art pages 39 and 96. The prior queue packet had directly checked **13 different group pages**. A combined **90/173** ID identities therefore have direct page-image evidence (51 new, 39 prior); **68** more have an exact normalized OCR name on the expected group page, and **13** have only a family-page-order candidate. Two are outside this catalog. The 68 OCR matches are **locators**, not visual confirmation; repeated variant names can occur in several printed groups. The private ledger records one of these evidence states for every ID.

Direct visual review resolved two entire groups absent from OCR name search, and disambiguated several repeated-name groups by printed heading, page order and differing treasure/attack fields. Thus a raw name match or numeric triplet alone must not determine sprite, faction or loot behavior. The catalog page assignment for unreviewed rows remains a candidate until its page image is checked.

## Field coverage and discrepancies

On the **51 newly image-checked variants**, this packet transcribed and compared four numeric fields—level, HP, armor class, XP—giving **204 cells**. **15 cells differ** from retained data. It separately compared **51 treasure strings** after a lexical normalization of the item-chance token's placement in guide versus table notation; **three** differ. This normalization is a comparison aid, not proof of the original loot formula. These selected comparisons yield **40 variants with no selected-field discrepancy and 11 with at least one**. The prior packet's different selected-field set found 25 matches and 14 disagreements among 39 variants. Neither selected set proves full row equivalence or runtime behavior; **81 catalog variants** still lack a direct semantic comparison and the two tail rows have no catalog row to compare.

| Visually checked group | Checked difference or agreement | Limit |
| --- | --- | --- |
| Fire Lizard family, PDF 49 | The guide and retained table use different gold-die sizes for all three variants; item chance/level labels align after format normalization. | Printed guide or retained data may be wrong; F3 must choose the shipped rule. |
| Gargoyle family, PDF 61 | All three guide level/HP pairs differ from retained values; checked armor and XP agree. | No observed monster HP or level. |
| Titan family, PDF 94 | Guide armor differs for all three, and the top variant's guide level/HP differs; checked XP agrees. | The same page's electrical missile/spell labels also need parser resolution. |
| Werewolf family, PDF 95 | The first two guide level/HP pairs differ; the third selected numeric set agrees. | Page order resolves names but not spawn identity. |
| Repeated Peasant and Cannibal headings, PDF 73, 79–80, 82–83, 86 | Different printed group headings, illustrations and treasure fields distinguish same-name rows; an OCR name hit cannot choose among them. | The printed picture is not yet joined to every retained sprite or map spawn. |

The guide table supplies attack types, projectile names, special attacks, spell names, AI, preferred targets and qualitative resistance labels. This packet records for **every ID** whether those effects have a direct page-image, prior selected-field, OCR-only or no-catalog status, and attaches the retained per-ID parser flags. It does **not** claim a complete guide-to-code effect comparison for all 173; qualitative resistance labels cannot be equated to retained numeric resistance cells by OCR alone.

## Current engine semantics to close

[Monsters.cpp](../src/Engine/Objects/Monsters.cpp) loads the retained 33-column rows but its shared parsers silently turn unmatched attack types into physical, unmatched projectiles into none, unmatched spells into none, and unmatched special attacks into none. The retained per-ID [combat probe](MM6_COMBAT_SEMANTICS_F2.md) marks **61** rows with attack-type fallback or Energy-to-Earth alias, **46** with an unrecognized active missile, **13** with an unrecognized monster spell and **11** with an unrecognized special attack; these sets overlap. The private catalog ledger attaches the relevant flags to each row.

Several newly rendered guide pages make those flags concrete: the VARN guardian family prints an Energy second attack while the inherited parser maps retained `Ener` to Earth; the lizard archer page prints a fire-arrow second projectile that the parser does not recognize from the retained abbreviation; the Ooze page prints poison attacks and a poison spell while the parser substitutes physical damage or no spell for affected rows; the Titan page prints electrical projectiles and a psychic spell, while retained abbreviations become physical/no missile and one spell spelling falls to none. These are **static guide/data/source discrepancies**, not witnessed combat. [Actor.cpp](../src/Engine/Objects/Actor.cpp) and inherited [MonsterEnumFunctions.cpp](../src/Engine/Objects/MonsterEnumFunctions.cpp) still need title-specific attack, resistance, AI, loot, spawn and save-state joins described in the combat packet.

## F3/F4/F5 acceptance

**F3:** Directly check the remaining **81** catalog variants' page images, reconcile all field and effect claims for 171 catalog rows, and establish an explicit no-catalog status for the two tail IDs. At the planned original-file pass, resolve the selected guide/table disagreements, attack/projectile/spell/special labels, six resistance channels, printed AI claims and 173 picture/sprite/map-spawn identities. Keep exact values and copyrighted text private.

**F4:** Implement title-scoped, exhaustive MM6 monster IDs, family/tier/unique status and all active parser labels; reject or explicitly preserve unresolved labels rather than silently substituting MM7 semantics. Focused checks must cover the four named discrepancy families, duplicate-name families, Energy, fire-arrow, poison and electrical projectiles/spells, treasure format, immunity and actor persistence.

**F5:** Observe representative encounters graphically and in save/reload: verify displayed sprite/name, actual level/HP where inspectable, damage channel and projectile, spell/condition, AI target/retreat, loot amount/item and no reroll after map re-entry. A catalog match or title screen is insufficient evidence of playability.
