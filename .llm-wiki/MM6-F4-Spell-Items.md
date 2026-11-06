---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, spells, items]
---

# MM6 F4 Spell Items

The [F4 spell-item packet](../project/MM6_F4_SPELL_ITEMS.md) records the F3-matched 99-scroll and 99-book item-to-spell mapping and its guarded consumers.

The retained MM6 bands are scroll IDs 200–298 and book IDs 300–398, each in spell-ID order for 1–99. A tracked verifier checks all joins and categories against ignored reports and their original-matched F3 `items.txt`/`spells.txt` hashes. `MM6SpellItems.h` bounds the MM6 lookup independently of inherited MM7 item enums. The guild popup uses it and omits an MM7 Grandmaster description.

The corrected F3 PE creation matrix, already loaded from a private profile, supplies a necessary class/school availability gate across all 18 class IDs and 99 spell IDs. Codes 0–3 have the MMExtension documented availability meanings. Book use checks the class/school gate and an existing trained school skill before the inherited MM7 mastery gate; all other MM6 books still reject without learning or consumption. The retained spell text, captured PE spell records and owned guide introduction do not establish per-spell required mastery, so the rank gate remains closed. A later original-routine trace or F5 Normal/Expert/Master learning matrix must settle it.

Scroll use and guild stock generation reject before inherited MM7 rules or scroll consumption. Target/effect routes, guild stock, spellbook icons and F5 visual/save checks remain open. MM7 mappings are unchanged. The offline tests cover every mapped item ID, 18×99 synthetic class/spell school checks and invalid edges; the private verifier ties the class matrix to F3. This does not establish a working MM6 spell.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Spell Rules](MM6-F4-Spell-Rules.md), [MM6 F3 Read-Only Preflight](MM6-F3-Read-Only-Preflight.md).
