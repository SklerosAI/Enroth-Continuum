# MM7 remaining quests and areas — 2026-11-12

This is the concise selection board for Codex's MM7 lane. Current source and
tests remain authoritative; the detailed contract is the
[MM7 quest catalog](../.llm-wiki/MM7-Quest-Behavior-Catalog.md), and the broader
engine/acceptance work is in the
[MM7 audit checklist](MM7_AUDIT_CHECKLIST.md).

The MM7 working gate is closed. This list is remaining audit and coverage work,
not a claim that MM7 is presently unplayable. All graphical items remain
deferred while the owner's headless-only instruction is active.

## Recommended quest order

| Priority | Quest group | Areas | Work remaining |
| --- | --- | --- | --- |
| 1 | War alternatives and Arbiter | Erathia, Tularean Forest, Avlee, Harmondale | Real-item, decoy-item and ignored/timeout outcomes now pass for both 28-day assignments, including physical Loren and plans acquisition; the two broken timeout fallthroughs are fixed. The authored 56–84-day trumpet window, physical chest, Catherine/Parson/Judge Grey hand-ins, balanced real/real neutral result, 112-day messenger, Dark judge installation and a durable checkpoint pass. Remaining: expose/treason failure branches, determine whether the unplaced envoy hand-ins are reachable, cover balanced decoy/decoy and the other bit-derived result combinations, and add the war packet to a continuous campaign chain. |
| 2 | Malwick, Niles and the tapestries | Emerald Island, Harmondale, Tatalia, Castle Lambent, Castle Gloaming | Malwick's real street conversation now covers refusal, repeat offer and acceptance; the exact six-week Harmondale trigger, two-week Niles-visit deadline and four-week tapestry-return deadline pass through the authored clock event. Missing either deadline starts the invasion, empties the bank and closes a Harmondale service; the existing raid test proves clearing both attacker groups reopens services. Remaining: acquire and return both alignment-specific tapestries through their physical castle sources, prove the one-month allied-region hostility window, add the successful reward path and preserve the chain across a fresh-process checkpoint. |
| 3 | Continuous Dark campaign D1-D7 | Castle Gloaming, the Pit, Celeste, Thunderfist Mountain, Clanker's Laboratory, Breeding Zone, Colony Zod, Shoals and Lincoln | Focused Dark contracts exist, but the durable chain does not. Link physical altar-piece acquisition, Robert's cube by combat or theft, Roland/control access and combat, physical Lincoln retrieval and the continuous Dark ending. |
| 4 | Light route seams | Wine Cellar, Castle Gloaming, the Pit, Land of the Giants, Colony Zod, Shoals and Lincoln | Cover real Elder Vampire combat, the L5 Blaster reward, Roland/key/control access before Xenofex, and the retrieved Overthruster through the final hand-in. Audit the Walls protected-group predicate without weakening the no-kill player contract. |
| 5 | Regional strengthening and durability | Emerald Island, Erathia, Bracada Desert, Stone City, Bandit Caves, White Cliff Cave, the Maze and the Erathian Sewers | Acquire all six Emerald contest items physically. Add Griffin refusal, XP, gold and award assertions across Erathia/Bracada. Add direct Troglodyte combat. Add fresh-process checkpoints for behavior-covered but not yet durable routes: Lantern of Light, Davrik's Ring, Darron's brother, Haldar's remains, Troglodytes and Thief to Rogue. |
| 6 | First promotions, eight remaining | Dwarven Barrows/Zokarr's Tomb, Hall Under the Hill, the three Druid monolith regions, Red Dwarf Mines, Haunted Mansion, Wromthrax's lair, the pirate-map/Evenmorn route and the six golem-component areas | Cover Monk to Initiate, Ranger to Hunter, Druid to Great Druid, Archer to Warrior Mage, Knight to Cavalier, Paladin to Crusader, Cleric to Priest and Sorcerer to Wizard. Thief to Rogue is already behavior-covered. |
| 7 | Second promotions, eighteen remaining | Temple of Baa, Heart of Wood route, Zokarr's Tomb, Perfect Bow route, Arena, Alice route, Sun altar, Watchtower, Breeding Zone, Ashwar Nog'Nogoth, bounty route, dragon egg route, Navan treasury, Lady Carmine and Proving Grounds | Cover all nine Light and all nine Dark second promotions, including qualifying and honorary outcomes, XP, and wrong-alignment or missing-prerequisite refusals. Batch these geographically rather than as one monolithic test. |

## Area-focused acceptance still open

- **Tatalia and Avlee:** the progress-capture placement defect is fixed
  headlessly, but corrected graphical frames still need a later hidden
  recapture and visual inspection.
- **Shoals:** underwater fog remains visually unclassified. Reinspect a corrected
  image before deciding whether it is a renderer defect.
- **Land of the Giants, Colony Zod, Shoals and Lincoln:** retain the complete
  access and item chain in both Light and Dark continuity tests rather than
  accepting destination teleports as campaign proof.
- **Representative regions generally:** the full audit still needs graphical
  play through outdoor and indoor regions, combat, travel, branch choices and
  an ending, plus representative UI, movie, music and sound checks on the Mac.

## Already durable; do not reopen without new evidence

These nine independent routes have exact production-Save and
fresh-process normal-Load pairs with unchanged slot bytes:

- Season's Stole
- Kerrid and the Faerie King
- Parsons' Quill
- Missing Contestants
- Three Paintings
- Three Statuettes
- Arcomage Champion
- Obelisk puzzle
- Balanced war, neutral trumpet and Dark Arbiter

They are independent route boundaries, not a continuous regional campaign.
The current regional side/promotion selection is 14/14; there are eighteen
exact writer/reader checkpoint tests across the nine routes. The war checkpoint
is a main-path boundary, not a regional side quest.

## Gate for each new quest slice

1. Exercise the authored offer or trigger and one meaningful refusal or
   alternate outcome.
2. Verify physical item or combat dependencies where the route requires them;
   name any teleport or party-strength fixture explicitly.
3. Assert quest bits, awards, XP, gold, reputation, class/alignment and world
   state as applicable.
4. Add a production-Save/fresh-process-normal-Load pair when the slice changes
   durable campaign state, with missing-profile and no-save admissions closed.
5. Run the focused test, the current regional gate, changed-file lint and the
   full style gate. Reconcile neutral `master` before integration.
6. Inspect every generated progress image before accepting it as evidence, and
   keep private data, saves and raw evidence outside Git.

