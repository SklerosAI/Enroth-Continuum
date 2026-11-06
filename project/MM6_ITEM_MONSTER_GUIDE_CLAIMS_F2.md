# MM6 item and monster guide claims: F2 reconciliation queue

Status: **direct visual item-catalog pass, complete per-ID evidence queue, F2 open**
(2026-11-05). This packet extends [item rules](MM6_RULES_F2_CLOSURE.md),
[monster/combat semantics](MM6_COMBAT_SEMANTICS_F2.md), and the
[guide chapter ledger](MM6_GUIDE_F2_REVIEW.md). It does not read original
game archives, launch MM6, settle the original executable's rules, or close
the F2 gate.

## Method and evidence

The owner's retained 412-page [Prima strategy guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf)
was checked as rendered scans at PDF pp. **159–231** (the complete printed
item-catalog span, 73 pages) plus earlier selected monster and rules pages
**36–38, 40, 42–45, 47–48, 50, 52–54, 59, 158 and 232–233**.
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
review status, retained field schema and current consumer for **every 581 item IDs (0–580)**
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
**ambiguous** and **absent** are OCR results, not scanned-guide absences;
**generic category only** means the relevant scanned page illustrates or
discusses a category without listing that ID; **not listed in printed catalog**
requires inspection of the complete relevant printed table. The queue now
reports **460 matched, 97 generic-category-only, two confirmed not listed,
nine OCR-ambiguous, ten OCR-absent and three nameless items**. The monster
queue remains **82 matched, 52 ambiguous and 39 OCR-absent**.

Rendered pages resolve **460 item IDs to individual printed entries** and
**39 monster IDs**. Among items, **403 selected field sets match** and **12
disagree**; two entries have identity checks only, and 43 more printed
entries have no field/effect comparison yet. **164 of 581 item semantic rows
remain unreviewed**, down from 402 at the start of this pass. These 164 are
43 printed entry identities, 97 generic-category IDs, two printed catalog
absences, 19 unresolved OCR locators and three blank retained rows. OCR-only
value agreements have been superseded where visual pages were checked; they
never established complete effects. The monster field results remain **25
matches, 14 disagreements and 134 unreviewed**. No original executable
effect was verified by the guide pass.

The two confirmed printed-catalog absences are the final spell-scroll and
spellbook IDs. A spell description elsewhere in the guide does not establish
an individual item listing. Ten other item and 39 monster OCR absences still
have unknown printed status. The three blank item rows have no retained name
to seek in the guide.

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
band; their counts sum to 581. Selected visual checks now span weapon,
armor, wand, herb, potion, scroll, spellbook, artifact, relic, quest and
miscellaneous sections. Matching name/value/skill/damage cells do not
establish actual casts, damage, merchant price, quest role, identify or repair
behavior.
The 12 selected field disagreements split into seven weapon prices, four
armor/headgear rows (one also differs on armor bonus), and one quest shield
identify level. Exact guide and retained cells remain in the ignored private
ledger. The 99 scroll and 99 book retained spell tokens are complete static
sequences; the guide lists 98 entries in each printed table. Their final
IDs and all cast/learn effects still require executable evidence.
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

### High-risk item role and consumer pass

The private queue now classifies all **404 IDs** in the wand, spell-scroll,
spellbook and 400–580 special-item bands from retained type, rarity, spell
token and description markers. It also records the current C++ consumer and
inherited ID predicates per ID. This is complete **static role/consumer
coverage** for this scope, not a guide effect check: **164 of 581 item
rows remain semantically unreviewed**. The subsequent rendered catalog pass
used the owner-retained guide, without reopening original game archives.

| Retained role | IDs | Static C++ finding |
| --- | ---: | --- |
| Wands | 25 | Retained spell tokens and [spellForWand](../src/Engine/Objects/ItemEnumFunctions.cpp) agree for 9 IDs and disagree for **16**; the numeric wand range aligns, so the wrong spell can be selected without a range error. |
| Spell scrolls | 99 | All retained rows carry sequential spell tokens 1–99, but [Character item use](../src/Engine/Objects/Character.cpp) calls an inherited scroll lookup for IDs 300–398; MM6 scroll IDs are 200–298. All 99 are outside its range. |
| Spellbooks | 99 | All retained rows carry sequential spell tokens 1–99, but Character calls an inherited book lookup for IDs 400–498; MM6 book IDs are 300–398. All 99 are outside its range. |
| Artifacts and relics | 30 | Retained rarity marks 15 of each at IDs 400–429. [MM6ItemTable](../src/Engine/Tables/MM6ItemTable.cpp) loads that rarity, while inherited `isArtifact`, generation and [bonus map](../src/Engine/Objects/Item.cpp) use IDs 500–552. None of these 30 enters that artifact-ID path. |
| Other special items and message scrolls | 151 | 50 retained message scrolls occupy 500–549; inherited `isMessageScroll` uses 700–781 and inherited `isArtifact` treats 500–552 as artifacts. The other 101 rows include 34 with a `Quest Item:` description marker; 67 have only a broad special or miscellaneous role from the retained table. None lies in inherited `isQuestItem` range 600–699. |

The retained [quest graph](MM6_EVENT_CAMPAIGN_SPEC.md) has held-item variable
operands referencing **98 distinct IDs** in the 400–580 band: two artifact
IDs, 53 IDs at 430–499, 15 message-scroll IDs and 28 IDs at 550–580.
The private queue retains exact event/offset locators. An operand is evidence
of a script reference, not proof of an item's quest purpose or complete use
flow. `Quest Item:` is a retained description marker, not an engine predicate.

The ID collisions are active source seams: MM6 item types route scrolls,
books and messages into [Character.cpp](../src/Engine/Objects/Character.cpp),
whose lookups or [message window](../src/GUI/UI/UIMessageScroll.cpp) use
inherited numeric ranges. Inherited `isSpellbook` also matches MM6 IDs
400–498, including artifacts and special items; inherited `isArtifact` matches
three MM6 memory-crystal IDs beyond the message band. The inherited level-six
artifact picker can select IDs 500–528, which MM6 retains as message scrolls.
Static inspection cannot settle the original executable's spell targets and effects, scroll
consumption/cancellation, wand charge rules, artifact bonuses and rarity
policy, message text/display, or quest item interactions and persistence.
F3 must confirm those against the original installation after the gate;
F4 needs title-specific ID dispatch and isolated tests, and F5 needs cast,
equip, read and quest/save-reload play checks.

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
found two separate guide-result disagreements among 25 safe pairs. These
are concrete reasons to keep guide text as a hypothesis and recheck original shipped data
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

1. **Before F3:** resolve the nine ambiguous and ten OCR-absent item locators,
   then compare meaningful fields/effects for the 43 visually identified but
   semantically unreviewed entries. Preserve the 97 generic-category-only,
   two printed-table-absent and three blank-name exceptions per ID. Resolve
   52 ambiguous and 39 OCR-absent monster names against rendered pages and
   compare all 134 semantically unreviewed variants. Reconcile item
   generation, the potion matrix, monster labels, guide treasure differences
   and the two-row tail. A status/count must remain available for each ID in
   the private queue.
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
