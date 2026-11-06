# MM6 item and monster guide claims: F2 reconciliation queue

Status: **partial direct guide review, complete per-ID OCR catalog queue, F2 open**
(2026-11-05). This packet extends [item rules](MM6_RULES_F2_CLOSURE.md),
[monster/combat semantics](MM6_COMBAT_SEMANTICS_F2.md), and the
[guide chapter ledger](MM6_GUIDE_F2_REVIEW.md). It does not read original
game archives, launch MM6, settle the original executable's rules, or close
the F2 gate.

## Method and evidence

The owner's retained 412-page [Prima strategy guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf)
was checked as rendered scans at PDF pp. **36–38, 40, 42–45, 47–48, 50,
52–54, 59, 158–159, 164, 168, 180, 189, 201, 203–204, 206, 209,
213, 219, 224, 228 and 232–233** (32 pages).
The earlier [potion review](MM6_RULES_F2_CLOSURE.md) visually checked
pp. 238–239. The chapter text layer was searched across all monster catalog
pages 35–96 and item catalog pages 157–240; it has visible OCR mistakes. The private
`item-monster-guide-selected-claims-private.md` records exact selected guide
versus retained-row comparisons and page references, without copying them
into Git.

The tracked [queue probe](../scripts/mm6_probe_item_monster_guide_claims.py)
consumes already retained, ignored `item-range-probe.json`,
`combat-semantics-probe.json`, `guide-coverage-probe.json`, retained
`icons/items.txt`, the retained page-marked Prima text, and private visually
reviewed overrides locked to the PDF hash. It writes an ignored
`item-monster-guide-queue.json` with candidate guide section, exact normalized
OCR-name page hits, OCR base-value claims, catalog match status, semantic
review status and current consumer for **every 581 item IDs (0–580)**
and **173 monster IDs (1–173)**. Input fingerprints travel with the queue. A
candidate section or OCR hit is a work locator, not proof that a printed row
or its effect is correct. Reproduce with:

```sh
python3 scripts/mm6_probe_item_monster_guide_claims.py \
  local-game-data/mm6/research-2026-11-05 \
  local-game-data/mm6/research-2026-11-05/item-monster-guide-queue.json
```

The catalog statuses have narrow meanings: **matched** means a unique exact
normalized name in the candidate guide section or a visual override;
**ambiguous** means a single-word, repeated or out-of-section hit; **absent**
means no exact name in the extracted chapter text, **not** absent from the
scanned guide; **unreviewed** means the retained row has no name. Visual
inspection found both an item and a monster variant that OCR had missed. The
queue reports **174 matched, 263 ambiguous, 141 OCR-absent and 3 unreviewed
items**; **82 matched, 52 ambiguous and 39 OCR-absent monsters**. Direct page
images cover **16 item IDs and 39 monster IDs**. For items, 11 selected field
sets match, three selected base values disagree, two have identity checks
only, and 163 additional base values agree through OCR extraction alone;
**402 item semantic rows remain unreviewed**. For monsters, 25 selected field
sets match and 14 disagree; **134 monster semantic rows remain unreviewed**.
The 166 unique OCR item value claims include the three disagreements, all
confirmed in rendered scans. OCR agreement is a partial value check, not a
complete entry, effect, or runtime check. These denominators keep lexical,
partial value, rendered-row, and rule coverage separate.

No retained ID is yet classified as **not listed in the printed guide** or
**unreadable in its scanned entry** without direct page evidence. The 141
item and 39 monster OCR absences remain unresolved; they must not be
silently converted into a printed absence. The three blank item rows have
no retained name to seek in the guide.

The source-level diagnoses below have **high confidence** where a retained
field and current C++ path directly show a mismatch. Guide behavior has
**medium confidence** until original shipped behavior or live play settles
it. A checked name or value is not an item/monster runtime check.

## Item-family queue

| Candidate guide section (PDF pages) | Retained IDs queued | Current engine seam and next direct claim check |
| --- | ---: | --- |
| Weapons (159–180) | 65 | Compare every equipment skill, dice, bonus and identified state with [MM6ItemTable.cpp](../src/Engine/Tables/MM6ItemTable.cpp), then equip/hit/save in F4/F5. |
| Bodily protection and jewelry (180–200) | 69 | Compare armor base/magic modifiers, break/repair, skill and slot rules; the shared [item predicates](../src/Engine/Objects/ItemEnumFunctions.h) need MM6 ranges. |
| Wands (201–203) | 25 | Verify spell reference, charges, targeting, repair and item use, not just names and shop value. |
| Herbs (203–204) | 3 | Verify potion input/power and bottle action. |
| Potions and alchemy (204–205, 236–240) | 34 | Reconcile effect/age/condition rules and all 29×29 combination cells; [UIPopup.cpp](../src/GUI/UI/UIPopup.cpp) still uses MM7 potion IDs and Alchemy restrictions. |
| Spell scrolls (206–208) | 99 | Join each item to the MM6 spell ID, cast target, scroll consumption and visible effect; inherited ranges classify these incorrectly. |
| Spellbooks (209–212) | 99 | Join learning eligibility, MM6 spell ID, skill/mastery and save/reload; inherited scroll range overlaps this band. |
| Artifacts (212–218) | 15 | Retained rarity field and MM6 loader identify these rows; reconcile unique bonuses, generated versus placed and equip use. |
| Relics (218–224) | 15 | Retained rarity field and MM6 loader identify these rows; reconcile tradeoffs, equip effects and save persistence. |
| Other special, quest or miscellaneous candidate (212–231) | 70 | **Unresolved classification** for remaining 400–499 rows; type/ID band alone does not establish quest or ordinary role. |
| Quest and miscellaneous documents (224–231) | 50 | Join message-scroll IDs with quest/dialogue read and persistence paths. |
| Quest or miscellaneous candidate (224–231) | 31 | **Unresolved classification** at 500+; join exact event/quest usage, tradability and display. |
| Unclassified item (157–240) | 6 | Blank/gold/other types require direct row and consumer inspection before any default behavior. |

These are **candidate** buckets generated from retained type, rarity and ID
band; their counts sum to 581. The packet does not claim each printed guide
entry has been individually matched. Selected visual checks now span weapon,
armor, wand, herb, potion, scroll, spellbook, artifact, relic, quest and
miscellaneous sections. Matching name/value/skill/damage cells do not
establish actual casts, damage, merchant price, quest role, identify or repair
behavior.
The guide's generated-item page describes level and enchantment constraints;
[MM6ItemTable.cpp](../src/Engine/Tables/MM6ItemTable.cpp) loads 14 standard
and 59 special enchantment rows plus random weights, while shared
[ItemTable.cpp](../src/Engine/Tables/ItemTable.cpp) still contains MM7 artifact,
spellbook and item-category decisions. This is a consumer map, not proof of
generation rules. In particular, the guide's broad claim that top-level
random items are always magical conflicts with the current retained
weapon-special chance and [generator branch](../src/Engine/Tables/ItemTable.cpp)
that can return an unenchanted top-level weapon. Original shipped behavior
must decide whether this is a guide error, inherited-engine difference or
data interpretation issue.

## Monster-family and combat claims

The rendered guide introduction describes **57 three-variant groups**. The
retained numeric IDs form **57 complete triplets plus a two-row tail**.
Therefore the shape of the first 171 IDs is consistent with the guide, while
the final two need an explicit guide/picture/spawn/unique join. All 173 IDs
are queued for individual page/row matching; the queue does not call a
numeric triplet a semantic family.

Across 13 visually checked three-variant groups, **14 of 39 variants have a
selected guide/retained-data disagreement**: 11 printed treasure gold-dice
entries and three level/HP pairs. The checked monster fields include level,
HP, armor class, XP, treasure, attack and resistance labels; a selected-field
match does not prove all fields or behavior. The earlier potion comparison
found two separate guide-result disagreements among 25 safe pairs. These are concrete
reasons to keep guide text as a hypothesis and recheck original shipped data
at F3; neither source should be silently substituted for the other.

The guide also describes attack preference, secondary attack chance and
recovery, retreat thresholds, six resistance channels, immunity and
gold/item rolls. [Combat semantics](MM6_COMBAT_SEMANTICS_F2.md) maps every
retained row to current [Monsters.cpp](../src/Engine/Objects/Monsters.cpp),
[Actor.cpp](../src/Engine/Objects/Actor.cpp), and inherited
[MonsterEnumFunctions.cpp](../src/Engine/Objects/MonsterEnumFunctions.cpp).
The current parser silently changes or drops active attack, missile, spell
and special labels. Original Magic/Energy damage, AI/faction behavior,
sprite/spawn joins, condition effects, random/fixed loot and persistence
remain open. The rendered guide's qualitative resistance labels cannot be
mapped directly to retained numeric cells. Its retreat thresholds disagree
with the current [Actor.cpp](../src/Engine/Objects/Actor.cpp) path for all
three fleeing AI classes. A guide paragraph cannot resolve those
source/runtime gaps.

## F3, F4 and F5 handoff

1. **Before F3:** resolve the 263 ambiguous and 141 OCR-absent item names,
   and 52 ambiguous and 39 OCR-absent monster names against rendered guide
   pages; then directly compare each of the 581 item and 173 monster IDs
   against its scanned guide entry where one exists. Record an explicit
   not-in-guide or OCR-uncertain exception rather than a fabricated match.
   Reconcile every item category, generated-item level/bonus rule, monster
   label, guide treasure discrepancy and the two-row tail. A status/count
   must remain available for each ID in the private queue.
2. **F3 read-only original pass:** compare retained table hashes, exact
   disagreement rows and executable-only rules to the owner's original
   installation once the shared F2 gate freezes. If the printed guide differs,
   record the chosen shipped rule and evidence privately. Do not infer a
   dynamic rule from a text row alone.
3. **F4 implementation:** require exhaustive MM6-specific ID/category
   mappings and no silent parser fallback for active labels. Offline tests
   should cover item generation levels, enchantment classes, equip/identify/
   repair/use, all 841 potion cells, monster attack/target/resistance/loot
   branches, and saved map/actor state. Keep MM7 item and monster paths
   isolated.
4. **F5 live acceptance:** inspect rendered item, inventory, projectile and
   encounter outcomes; exercise melee/ranged/caster/status/neutral/flying
   cases, loot and representative items from every family. Save, quit,
   relaunch and reload after item changes and kills to detect rerolls or lost
   conditions. A successful table load is not a playable combat check.

This packet gives an exhaustive **queue** and selected direct evidence, not a
complete semantic review. The [F2 gate review](MM6_F2_GATE_REVIEW.md) remains
open until the integration owner reconciles the outstanding claims and
records an explicit freeze decision.
