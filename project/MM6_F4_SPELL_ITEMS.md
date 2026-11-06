# MM6 spell-item IDs, F4

Status: **offline ID join and guarded consumers; casting, learning and guild stock remain closed** (2026-11-06). This packet uses the ignored F2 spell/item reports whose retained `items.txt` and `spells.txt` hashes match the original-file F3 content reconciliation. It did not reread the original installation or launch MM6. Do not copy the reports or individual game rows into Git.

## Accepted ID mapping

The retained MM6 item table has exactly 99 spell scrolls at item IDs **200–298** and 99 spellbooks at **300–398**, each in spell-ID order. Thus scroll `199 + spell ID` and book `299 + spell ID` map to spell IDs **1–99**. The [offline verifier](../scripts/mm6_f4_check_spell_items.py) checks all 198 candidate joins against the two ignored reports, verifies each item's category and name join, and ties both source hashes to the 348-member F3 content reconciliation. One retained spell label is abbreviated on its two items; this does not change the numeric join. The C++ test covers every mapped ID and wrong-kind/out-of-range edges.

`MM6SpellItems.h` implements the bounded mapping independently of the MM7 `ItemId` names and indexed arrays. IDs outside a band's bounds throw before lookup; the MM6 book and scroll bands are distinct even though they overlap the inherited MM7 scroll band.

## Consumer audit and current guard

| Consumer | Current MM6 behavior | Remaining work |
| --- | --- | --- |
| Retained item parser and `Item::isBook`/`isSpellScroll` | The MM6 parser reads item types from retained rows; the F3-matched bands carry book/scroll categories. | Validate live inventory presentation at F5. |
| Guild book popup, `showSpellbookInfo` | Uses the bounded MM6 book mapping and the MM6 text table's spell-ID row. It shows Normal, Expert and Master text, omitting inherited Grandmaster. | Visually inspect layout and source text at F5. |
| Book use, `Character::useItem` | Checks the MM6 ID before the inherited mastery gate and rejects without learning or consuming. | Derive the original three-rank learn gate and class/school restrictions; then test learnability and save/reload for all 99 IDs. |
| Scroll use, `Character::useItem` | Checks the MM6 ID and rejects before the inherited item-target branch, cast queue and item consumption. | Derive all 99 target/effect routes, including cancellation and recovery. The existing central spell-rules guard occurs too late to preserve a consumed scroll. |
| Guild stock generator | Rejects MM6 stock generation before the inherited `spellbooksForSchool` can create MM7-range items. | Derive MM6 guild inventory and mastery/quest rules, including combined guilds. |
| Learned-spell UI, `UISpellbook.cpp` | Uses numeric spell IDs and an inherited icon permutation, with no item-to-spell lookup. | Reconcile spell icons, school pages, and quickspell visuals against MM6 resources. |
| MM7 book/scroll mapping | Unchanged; inherited consumers continue to use their existing arrays. | Run MM7 smoke checks at integration. |

The mapping proves item identity, not spell behavior. The inherited numeric spell effect switch, event/monster/scroll cast behavior, guild stock and spellbook mastery remain separate F4 work. No graphical outcome is claimed.

## Offline checks

```text
python3 scripts/mm6_f4_check_spell_items.py <ignored-mm6-research-directory>
OpenEnroth_UnitTest --gtest_filter=MM6SpellItems.*
```

See also: [F2 spell audit](MM6_SPELL_EFFECT_F2.md), [F3 gate](MM6_F3_GATE_REVIEW.md), [spell mana/recovery](MM6_F4_SPELL_MANA_RECOVERY.md), [build graph](MM6_F4_BUILD_GRAPH.md).
