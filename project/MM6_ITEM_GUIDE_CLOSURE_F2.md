# MM6 late item-guide and held-item review — F2

Status: **direct guide-page and retained-event review, 2026-11-05; F2 remains open**. This packet follows the [catalog queue](MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md) and [item action seams](MM6_ITEM_EFFECT_SEAMS_F2.md). It reads the already retained owner guide scans and ignored F2 reports; it does not reopen the original game installation or run MM6. Exact item names, rows, guide values, event locators, image hashes and judgments are in ignored `local-game-data/mm6/research-2026-11-05/item-guide-closure-private.json` and its private review override. The tracked `scripts/mm6_probe_item_guide_closure.py` validates their source hashes and coverage.

## Forty-three identified late-catalog entries

The 43 previously unreviewed entries with direct printed identity all occur on the rendered late item pages 224–230. Direct inspection finds **eight qualitative role claims** for keys and Oracle data objects. The other **35 have descriptions without a clearly specified gameplay rule** on their individual page. Some descriptions hint at possible interactions, but give no numeric action, bonus, price, consumption, sale or event rule. A role clue is a guide claim, not a complete engine effect. These 43 are now visually classified by *kind of guide evidence*, but their original executable behavior remains unverified; they should not be counted as 43 implemented or fully reconciled item effects.

The guide's introductory miscellaneous paragraph says some keys and messages are quest steps rather than independent quest items. That category statement does not settle any specific retained item's sale restriction, consumption, pickup, event branch or persistence. The printed individual pages likewise do not give those rules for the 35 description-only entries. F3 and the retained event graph must decide exact behavior.

## Nineteen uncertain OCR locators

The rendered **gem table at PDF p. 232** extends beyond the previous per-item page span. It lets this review check ten retained gem rows that formerly made up nine OCR-ambiguous names and one OCR-absent name. **Nine of ten name/value pairs agree** with the printed table. The tenth has a retained duplicate name with the guide's different distinct gem name at the same value. This is a specific guide/retained-content identity disagreement, not a license to rename shipped data. The exact row and words remain private for F3 original-byte confirmation. The gem table gives values, not acquisition, generation, shop price or use effects.

The other **nine OCR-absent rows** have no individual entry on the directly checked late item pages. Three names appear elsewhere in guide OCR: one in a shop name, one in a quest handoff, and one as a spell title/book reward. Direct page images were checked for those three contexts. Only the quest handoff supplies a candidate item role; none establishes an exact item-ID or direct-use rule. The other six still have no reliable guide locator. OCR absence is not proof that they are absent from every guide page; retain them as unresolved instead of fabricating a catalog entry.

## Sixty-seven broad special and miscellaneous roles

The retained event graph contains **400 held-item operand references** to **50 of 67** broad special/miscellaneous IDs across **13 EVT files**. The other 17 have no held-item operand in that graph; they could still be used through another path. Grouped by opcode presence per ID, the 67 split into: 27 Compare+Subtract, ten Compare+Add+Subtract, 11 Add only, two Compare only and 17 with no held-item reference. These correspond to current source opcode labels 14/16/17, but a static operand does not prove branch reachability, acquisition, consumption or quest completion. Exact event offsets and item IDs stay private.

Current `EvtInterpreter.cpp` forwards these operations to the inherited `VAR_PlayerItemInHands` handling in `EvtVariables.cpp`; the [event execution audit](MM6_EVENT_EXECUTION_SEAMS_F2.md) also shows that many map programs cannot load under the current parser. F3 must confirm the MM6 variable and event semantics before an operand signature becomes a gameplay rule. The [item action audit](MM6_ITEM_EFFECT_SEAMS_F2.md) separately finds that current direct-use cases and message-scroll window guards use MM7 numeric IDs. None of the 67 broad roles is made safe by an inherited predicate.

## Handoff

- **F2 remaining:** keep per-ID `guide role`, `retained table`, `event reference`, `current source consumer` and `unresolved behavior` distinct. The 43 printed late entries have a classified guide-evidence kind, and the gem table closes nine name/value comparisons; it does not close original effects. Resolve the six unlocated guide names only if a relevant page or source appears. The 17 no-operand broad-role rows need other consumer searches.
- **F3 one read-only original pass:** compare the retained item and event hashes, the one gem identity disagreement, exact table types and all event-held operands with the owner's original bytes. Inspect executable item action, sale/quest, generation and message-window policies where available. Record exact per-ID decisions locally; preserve uncertainty with an explicit F4/F5 route.
- **F4 offline:** implement title-specific item dispatch and typed held-item events. Test each of the five opcode-signature groups, plus guide-role clues, gem values, direct-use, shop/repair, document reading and save changes. Map scripts must load before claims about their held-item effects.
- **F5 visible play:** inspect acquisition, key/door and Oracle handoff, item application, inventory and shop behavior, relevant dialogue, and save–quit–relaunch–load. A guide page or static event check is not a witnessed outcome.

The private review remains hash-locked to the retained queue and PDF. The public aggregates do not reproduce proprietary item text, names, values or event maps.
