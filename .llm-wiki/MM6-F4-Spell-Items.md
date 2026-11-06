---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, spells, items]
---

# MM6 F4 Spell Items

The [F4 spell-item packet](../project/MM6_F4_SPELL_ITEMS.md) records the F3-matched 99-scroll and 99-book item-to-spell mapping and its guarded consumers.

The retained MM6 bands are scroll IDs 200–298 and book IDs 300–398, each in spell-ID order for 1–99. A tracked verifier checks all joins and categories against ignored reports and their original-matched F3 `items.txt`/`spells.txt` hashes. `MM6SpellItems.h` bounds the MM6 lookup independently of inherited MM7 item enums. The guild popup uses it and omits an MM7 Grandmaster description.

Book use, scroll use and guild stock generation reject before inherited MM7 rules or scroll consumption. Three-rank learnability, all target/effect routes, guild stock, spellbook icons and F5 visual/save checks remain open. MM7 mappings are unchanged. The offline test covers every mapped ID and invalid edges; this does not establish a working MM6 spell.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Spell Rules](MM6-F4-Spell-Rules.md), [MM6 F3 Read-Only Preflight](MM6-F3-Read-Only-Preflight.md).
