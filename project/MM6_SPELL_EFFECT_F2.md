# MM6 spell-effect F2 audit

Status: **source and retained-table reconciliation, not implemented spell behavior** (2026-11-05). This packet covers all 99 MM6 spell IDs structurally for the integration owner. It did not reopen an original archive, launch the partial MM6 runtime, or establish the meaning of every spell effect. Exact spell text, per-ID joins, event offsets, and monster labels stay in ignored `local-game-data/mm6/research-2026-11-05/spell-effect-probe.json`.

## Evidence and method

`scripts/mm6_probe_spell_effects.py` reads the retained `spells.txt` and existing item, event, and monster indexes; verifies their source hashes; and compares every spell ID with current `SpellEnums.h`, `Spells.cpp`, `CastSpellInfo.cpp`, item ranges, event-cast operands, and monster-spell label mapping. It fingerprints the owner's manual and Prima guide without copying their text. The guide's printed spell chapter starts (checked against its locally retained table of contents) are per-school locators in the private report. The [manual](https://www.mocagh.org/nwc/mm6-manual.pdf) has spell rules on PDF pp. 42–54; the [Prima guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) covers spells on printed pp. 89–148. These locators have not yet been reconciled spell by spell against the current effect code.

The probe confirms 99 retained spells in nine blocks of eleven. At the same numeric ID, 50 normalized names match the inherited enum, 21 names occur at another inherited ID, and 28 are absent or renamed. The first three mana costs differ at 27 IDs. Eleven retained electricity, cold, or poison damage tags fall to `DAMAGE_PHYSICAL` in `SpellStats::Initialize`. These are lexical/source comparisons; even a matching name and mana cost do not prove the same effect.

## Current dispatch gaps

| Path | Current source behavior | Required MM6 contract and check |
| --- | --- | --- |
| Spell table and learn gate | The MM6 branch of `SpellStats::Initialize` fills names, descriptions and damage tags, but does not update `pSpellDatas`. The static `pSpellDatas` table therefore retains MM7 mana, recovery, damage factors, flags and required mastery. Nine inherited rows require Grandmaster; `Character.cpp` compares that threshold when reading a spellbook. | Define MM6 costs, recovery, factors and three-rank learnability for all 99 IDs; prove each book is learnable at its intended MM6 rank and no Grandmaster path is exposed. Source: `src/Engine/Spells/Spells.cpp`, `src/Engine/Objects/Character.cpp`. |
| Player effect dispatch | `CastSpellInfo.cpp` has inherited switch cases for 97 numeric IDs and direct pre-switch branches for the remaining two portal/beacon IDs. Every ID has a current path, but the code selects that path by the inherited numeric enum. | Specify all 99 MM6 effects at Normal, Expert and Master, including targets, durations, recovery, damage, terrain/movement, buffs, conditions and restrictions. Implement title-specific dispatch and exercise each ID offline plus representative live casts. |
| Scrolls and books | The retained MM6 item index has 99 scrolls at 200–298 and 99 spellbooks at 300–398. `ItemEnumFunctions.cpp` indexes inherited scrolls at 300–398 and books at 400–498; `Item.h`/`Character.cpp` route typed MM6 items into those helpers. All 198 retained spell items lie outside their respective inherited mapping range. | Map MM6 scroll/book IDs to spell IDs explicitly, validate range before indexing, and test every item for correct cast or learning, including cancellation and persistence. Keep MM7 mapping intact. |
| Event casts | The retained active scripts have 129 `EVENT_CastSpell` records using 15 numeric spell IDs. Twenty-two records use an ID whose retained name differs from or is absent from the inherited same-ID label. `EvtInterpreter.cpp` passes the numeric ID to `eventCastSpell`; no title-aware semantic translation was established. | Reconcile each event's intended spell and mastery, then exercise the event effects in context. The count is a routing risk, not proof that all 22 execute in a reachable branch. |
| Monster casts | The retained monster audit finds 55 nonempty spell cells. The current name parser recognizes 42; 13 fall to `SPELL_NONE`. Among the recognized labels, 15 map to a current numeric ID different from the retained spell table's same label; only 27 match by this lexical join. | Give monster spells MM6 label-to-ID mapping and cast rules; reject unknown active labels during offline validation. Test ranged/caster enemies, damage/resistance, buffs and save/reload. |
| Sound and sprite | `SpellSoundIds` and `SpellSpriteMapping` in `Spells.cpp`, and `AudioPlayer::playSpellSound`, remain inherited tables keyed by numeric spell ID. This audit did not compare them per resource or hear/render them. | Join every MM6 spell to its shipped effect sprite/sound and verify cast/impact visuals and audio at F5. |

The current 97 switch cases plus two special branches are *path presence only*. The private report gives each spell's retained text, same-ID inherited label, mana triple, current static data, case line, guide school locator, candidate scroll/book ID, and event/monster references. It does not assign correctness to a case.

## School coverage and acceptance scenarios

| MM6 school block | Prima printed locator | Required representative live scenario after full offline ID coverage |
| --- | --- | --- |
| Fire, IDs 1–11 | pp. 92–97 | Direct and area damage, light/buff, resistance, cast/impact rendering and Master behavior. |
| Air, IDs 12–22 | pp. 98–103 | Projectile and area effects, movement/fall/fly, detection, resistance and Master behavior. |
| Water, IDs 23–33 | pp. 104–109 | Projectile/cold or poison resistance, item enchant/recharge, travel/return, and Master behavior. |
| Earth, IDs 34–44 | pp. 110–115 | Damage and control, terrain/telekinesis, protection, cure and Master behavior. |
| Spirit, IDs 45–55 | pp. 116–121 | Blessing, undead interaction, curse/death recovery, party health sharing and Master behavior. |
| Mind, IDs 56–66 | pp. 122–127 | Fear/charm/condition, mind resistance, control and Master behavior. |
| Body, IDs 67–77 | pp. 128–133 | Heal/condition cure, regeneration, damage, protection and Master behavior. |
| Light, IDs 78–88 | pp. 134–139 | Undead/area effects, party buffs, conditions, time restrictions and Master behavior. |
| Dark, IDs 89–99 | pp. 140–148 | Undead/corpse, direct/area damage, draining, restrictions and Master behavior. |

Each school needs one or more graphical observations, but that is only representative F5 sampling. F4 must cover all 99 IDs, three mastery ranks, scrolls, books, monster/event casts, and relevant negative cases with explicit expected effects derived from the owner's shipped data and guide reconciliation. The current retained descriptions are evidence of intended text; they are not verified executable rules.

## Before F3 and implementation

1. Reconcile all 99 retained descriptions and mastery clauses against the owned manual and guide, with conflicts recorded privately by page and source hash. Determine exact mana, recovery, targets, duration, damage factors and resistance taxonomy; the retained text table does not supply every numeric constant.
2. At the planned single F3 original-file preflight, confirm `spells.txt`, scroll/book/wand ID mappings, and any executable-only spell constants. Resolve source disagreements before freezing the F4 table and dispatch contract.
3. Implement independent MM6 spell definitions and effect routing, including source-time validation that no active spell/item/monster/event ID silently maps to an unrelated MM7 effect or `SPELL_NONE`.
4. At F5, inspect actual graphics and sound separately, cast in all nine schools, exercise scroll/book and monster/event paths, and save/quit/relaunch/load affected conditions, buffs and world state. Preserve the protected MM7 runtime.

See also: [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md), [rules closure](MM6_RULES_F2_CLOSURE.md), [monster/combat specification](MM6_MONSTER_COMBAT_SPEC.md), [guide reconciliation](MM6_GUIDE_RECONCILIATION.md), and [media/UI specification](MM6_MEDIA_UI_SPEC.md).
