# MM6 unresolved item effects and action seams — F2

Status: **static triage, 2026-11-05; F2 remains open**. This packet extends the [item/monster guide queue](MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md). It does not promote any item to original-effect verification, read the installed game, or launch the partial MM6 engine. Exact item IDs/names, event locators, page locators and source fingerprints are in ignored `local-game-data/mm6/research-2026-11-05/item-effect-seams-private.json`.

## Evidence and triage

`scripts/mm6_probe_item_effect_seams.py` joins the private 581-ID guide queue to current tracked item-use, enum, predicate and message-window source. It requires 164 still-unreviewed guide-semantic rows and 67 broad special/miscellaneous roles, records source hashes, and writes only to ignored local storage. The owner's [Prima item chapter](https://www.mocagh.org/nwc/mm6-hintbook.pdf) was previously visually reviewed at PDF pages 159–231; this pass rechecked rendered miscellaneous pages 228–230. Those pages give examples and short descriptions, but do not define every item action or event effect. A printed name, a `Quest Item:` description marker, or an event held-item operand is not an executable rule.

| Retained role among the 164 unreviewed rows | Count | Immediate F2 question |
| --- | ---: | --- |
| Broad special/miscellaneous role | 66 | Resolve exact script-held, direct-use, map/door, sale and consumption behavior. One further member of the 67-role family has printed identity only, so its effect remains open too. |
| Message scroll | 50 | Determine text, read-window and event/state effects for each MM6 item. |
| Quest-described miscellaneous | 29 | A description marker is not sufficient to decide action or tradability. |
| Outside the prior high-risk band | 15 | Nine potion/alchemy, three gold, three blank rows require distinct source/default checks. |
| Quest-described equipment | 2 | Verify equip bonuses and script-held behavior together. |
| Spell scroll and spellbook | 1 each | These are the two visually confirmed absences from the printed item tables; retained token and original cast/learn behavior still matter. |

Across the 164, **93** have a retained event held-item operand and **43** have individual printed-entry identities without a selected field/effect comparison. These groups overlap. The 67 broad special/miscellaneous rows have `N / A` retained type and no `Quest Item:` description marker; **50** have held-item operands and **17** have an individual printed-guide identity. The absence of a marker or operand does not prove ordinary saleability or non-use. Inherited numeric predicates classify **43 of 67** as spellbook IDs, while **none** falls in the inherited quest-item range. This is an ID-policy conflict, not proof of the original MM6 quest classification.

The 164 count remains unchanged. This pass classifies **where evidence must come from**, without filling action effects from flavor text or MM7 behavior.

## Current C++ action seams

- [MM6ItemTable.cpp](../src/Engine/Tables/MM6ItemTable.cpp) maps `N / A` to `ITEM_TYPE_MISC`, and `Item::type()` exposes that type. When one of the 67 broad special/miscellaneous items is applied to a character, [Character.cpp](../src/Engine/Objects/Character.cpp) reaches its “Everything else” branch. Its seven explicit direct-use IDs are all above MM6's retained maximum item ID 580; the branch therefore has no MM6-specific direct-use case. The default reports that the item cannot be used that way. This says nothing about event, door, map-object or dialogue use.
- The same-named lamp and horseshoe examples appear in the retained MM6 items and printed guide, but the current direct-use branches address inherited MM7 numeric IDs. The guide pages inspected here give descriptions, not numeric effects. F3 must establish whether MM6 permits direct use and what it does before implementing a same-named action.
- [Item::canSellRepairIdentifyAt](../src/Engine/Objects/Item.cpp) checks inherited `isQuestItem`, `isArtifact` and `isMessageScroll` ID ranges before shop type. None of the 67 broad special/miscellaneous IDs reaches the inherited quest range. The source cannot express a title-specific no-sale rule for them by that predicate alone; actual shop acceptance also depends on house/type paths and has not been observed.
- All 50 MM6 message-scroll rows are type-classified for the `Character.cpp` read action, but `CreateMsgScrollWindow` in [UIMessageScroll.cpp](../src/GUI/UI/UIMessageScroll.cpp) additionally requires the inherited numeric `isMessageScroll` range (700–781). None of the retained MM6 message-scroll IDs (500–549) passes that window guard. This is a deterministic current-source path mismatch, not a claim that a player has reached the path or seen a blank window.

## F3, F4 and F5 handoff

**F2 before freeze:** resolve the 19 OCR-ambiguous/absent item locators and compare fields/effects for the 43 individually identified but unreviewed entries where the guide actually states a rule. Preserve category-only, printed-absent and blank exceptions instead of inventing entries. For the 67 broad special/miscellaneous rows, join held-item operands to the exact event branch and map/door/NPC consumer; classify acquisition, use, consumption, persistence and sale restrictions separately. The private report is a candidate queue for those joins, not their semantic closure.

**F3 original preflight:** in the one planned read-only pass, verify the retained table and event hashes, exact item-type/ID alignment, executable direct-use cases, quest/sale conditions, message-scroll guard and effects. Compare the printed guide only as a claim. Record exact per-ID decisions privately; unresolved executable behavior needs a named F4 representation and F5 scenario.

**F4 offline implementation:** provide MM6-specific item-kind and action dispatch, including all 50 message-window IDs, and explicit handling for the 67 broad special/miscellaneous rows according to their confirmed role. No inherited MM7 same-name action or numeric quest policy should silently decide an MM6 result. Test read, apply, inspect, shop/repair, script-held checks, consume/retain and save-state changes with representative IDs from every role, plus negative tests for foreign-title ranges.

**F5 visible play:** verify representative document windows, inventory/character actions, item-bearing dialogue/doors, shops, and save–quit–relaunch–load after item acquisition/use. A successful parser or source branch is not visible proof.
