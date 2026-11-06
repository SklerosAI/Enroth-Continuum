# MM6 `useitems.txt` combination matrix — F4 offline seam

Status: **bounded title-specific table load and lookup; mixing effects remain gated** (2026-11-06).

The ignored F3-matched `useitems.txt` copy is a decoded member of MM6 `icons.lod`, not `events.lod`. The F3 content report records pseudo-image zlib decoding, a matching retained hash and no capture discrepancy. `ItemTable::InitializeMm6` now reads that member from the current title's icons LOD, decompresses it with `lod::decodeMaybeCompressed`, and installs a fully validated `mm6::PotionCombinations` table. The MM7 `potionCombination` and `potionNotes` fields and loaders are unchanged.

The parser requires the 29 consecutive item IDs 160–188 on both axes, one matching header, 29 unique 35-column rows, all 841 nonempty cells, mirrored equality and a uniform two-letter diagonal marker. It bounds input to 64 KiB and 256 lines. A numeric cell must name an item on the same axis and is returned as a **candidate product**. A two-character letter-digit or two-letter cell is returned as an **opaque marker**. Unknown shapes, out-of-axis results, duplicates, missing rows, mismatched axes and asymmetry reject the entire load. Lookup rejects an unloaded table or an out-of-axis ingredient ID. The parser does not mutate the existing MM7 matrix.

The retained matrix passes all checks: 50 numeric cells represent 25 unordered ingredient pairs, and 791 cells carry opaque markers from five distinct codes. The private guide reconciliation covers those 25 numeric pairs: 23 printed outputs agree with the retained table and two disagree. The shipped table is the source for the candidate numeric IDs; the guide disagreements remain recorded privately. No exact recipe row, marker token or proprietary guide text is embedded in Git.

## Outstanding effect and interaction gates

1. Determine the five marker codes' actual effects, including the diagonal code, failure/side effects, consumption and character damage. Lexical form alone is not an effect rule.
2. Verify whether each numeric candidate product is actually produced in play, how ingredient order, potion power and empty-bottle return work, and whether any skill or other restriction applies. The inherited Alchemy skill gate is MM7-specific.
3. Route MM6 herb/bottle and potion/potion item IDs into a title-specific UI/action path. Current `UIPopup.cpp` still uses MM7 numeric predicates and its matrix, so this parsed table does not enable mixing.
4. Check black-potion and rejuvenation one-time effects, item use and save/reload separately; the matrix does not encode those rules.
5. Run F5 visible representative safe, marker and disputed-guide mixes, and verify inventory/character state after save, quit, relaunch and load. No partial MM6 launch was used for this F4 seam.

Three focused native tests cover all 841 synthetic cells, malformed axes/tokens/symmetry, bounds and the ignored retained aggregate. An independent retained probe produced the same 841/50/791 totals. The private F3 report/hash check verifies the corrected icons LOD source and decompression method. See [table consumer audit](MM6_TABLE_CONSUMER_SPEC.md), [rules closure](MM6_RULES_F2_CLOSURE.md) and [guide reconciliation](MM6_GUIDE_RECONCILIATION.md).
