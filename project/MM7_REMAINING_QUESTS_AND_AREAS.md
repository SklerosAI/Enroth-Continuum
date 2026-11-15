# MM7 remaining quests and areas — 2026-11-15

This is the concise selection board for Codex's MM7 lane. Current source and
tests remain authoritative; the detailed contract is the
[MM7 quest catalog](../.llm-wiki/MM7-Quest-Behavior-Catalog.md), and the broader
engine/acceptance work is in the
[MM7 audit checklist](MM7_AUDIT_CHECKLIST.md).

The MM7 working gate is closed. This list is remaining audit and coverage work,
not a claim that MM7 is presently unplayable. Hidden offscreen captures are
allowed evidence again; selected inspected PNGs stay private and the local
progress-capture folder has a hard rolling limit of 100 images.

## Owner-recorded coverage decision

- Representative **real movement** coverage is established by a bounded
  press/hold/release helper and a composed Wine Cellar approach. Setup may
  teleport to a deterministic lane, but the measured movement and combat
  segment cannot teleport. Autonomous pathfinding remains outside this proof.
- The **complete Barrow Downs region** now has authored-transition coverage:
  the outdoor map, Barrows I-XV and Zokarr's Tomb are visited across 27 real
  transition events. Monk-to-Initiate and the complete Light Arch Druid route
  are also covered: Green's offer, the live Nighon Tunnels skull source,
  Zokarr's deposit face and Green's return. This proves the graph and local
  quest events, not natural walking.

## Recommended quest order

| Priority | Quest group | Areas | Work remaining |
| --- | --- | --- | --- |
| 1 | War alternatives and Arbiter | Erathia, Tularean Forest, Avlee, Harmondale | Real-item, decoy-item and ignored/timeout outcomes now pass for both 28-day assignments, including physical Loren and plans acquisition; the two broken timeout fallthroughs are fixed. The authored 56–84-day trumpet window, physical chest, Catherine/Parson/Judge Grey hand-ins, balanced real/real neutral result, 112-day messenger, Dark judge installation and a durable checkpoint pass. Remaining: expose/treason failure branches, determine whether the unplaced envoy hand-ins are reachable, cover balanced decoy/decoy and the other bit-derived result combinations, and add the war packet to a continuous campaign chain. |
| 2 | Malwick, Niles and the tapestries | Emerald Island, Harmondale, Tatalia, Castle Lambent, Castle Gloaming | Behavior and durability complete. Malwick's real street conversation covers refusal, repeat offer and acceptance; the exact six-week Harmondale trigger, two-week Niles-visit deadline and four-week tapestry-return deadline pass through the authored clock event. Both alignment-specific Niles offers, physical Castle Lambent/Gloaming tapestry faces, successful returns, +5 reputation-only reward, exact 719/720-hour allied-castle hostility boundary and both missed-deadline invasions pass. Separate fresh-process checkpoints preserve the success and invasion outcomes; the existing raid test proves clearing both attacker groups reopens services. Remaining only at campaign level: connect the optional branch to a continuous Light or Dark chain rather than reopening its focused contract. |
| 3 | Dark campaign strengthening after C0-C7 | Harmondale, Deyja, Castle Gloaming, the Pit, Celeste, Thunderfist Mountain, Clanker's Laboratory, Breeding Zone, Colony Zod, Shoals and Lincoln | The separately marked Dark profile passes C0-C7. Representative movement/combat now has a reusable input contract, the Breeding Zone hidden frame is corrected, and Shoals' dense green fog is source-classified as authored underwater rendering. Remaining: reduce approach teleports where practical and eventually compare exact Shoals pixels with the original executable if parity matters. |
| 4 | Light campaign strengthening after C0-C7 | Wine Cellar, Castle Gloaming, the Pit, Land of the Giants, Colony Zod, Shoals and Lincoln | Light C6 follows the authored Mount Nighon–Thunderfist–Tunnels–Land route through Xenofex and the return. The real Wine Cellar target now activates only for its live quest, input combat kills it, the composed harness walks then fights without measured-segment teleporting, and Tolberti's complete Blaster reward transaction is asserted. Remaining: the Walls protected-group predicate audit, reducing other approach teleports and broader graphical play. |
| 5 | Regional strengthening and durability | Emerald Island, Erathia, Bracada Desert, Stone City, Bandit Caves, White Cliff Cave, the Maze and the Erathian Sewers | Acquire all six Emerald contest items physically. Add Griffin refusal, XP, gold and award assertions across Erathia/Bracada. Add direct Troglodyte combat. Add fresh-process checkpoints for behavior-covered but not yet durable routes: Lantern of Light, Davrik's Ring, Darron's brother, Haldar's remains, Troglodytes and Thief to Rogue. |
| 6 | First promotions complete | All nine first-promotion routes | Every first promotion is behavior-covered through its real offer and objective. Priest physically loots the Tidewater map, verifies the exact Falk transaction and proves the Evenmorn boat unlock; Great Druid activates all three Stonehenges; Crusader uses attack-input Wromthrax combat; Wizard physically acquires and installs every golem component, covers both heads and proves the animated-golem Castle outcome. Remaining work is durability/regional continuity, not another first-promotion behavior row. |
| 7 | Second promotions, twelve remaining | Temple of Baa, Heart of Wood route, Perfect Bow route, Alice route, Sun altar, Watchtower, Ashwar Nog'Nogoth, bounty route and Navan treasury | Druid to Arch Druid and Warlock, Knight to Champion, Sorcerer to Archmage and Lich, and Thief to Assassin are behavior-covered. Lich physically retrieves all four Walls of Mist jars, proves 3/4 refusal and durable source-chest selection after the unrelated-advisor-award collision repair. Cover the other six Light and six Dark second promotions. |

## Area-focused acceptance still open

- **Tatalia and Avlee:** corrected authored-start hidden frames were visually
  inspected. Both show coherent terrain, HUD and minimaps; the earlier invalid
  placement evidence is superseded.
- **Shoals:** the inspected frame shows coherent seabed relief fading into the
  deliberately short-range Eucalyptus-green underwater fog. Source proves the
  color and 50/2000 distances are authored; exact original-pixel parity remains
  a separate optional comparison, not a current renderer defect.
- **Breeding Zone:** a large black occluder came from the diagnostic's literal
  indoor-origin placement. The authored floor-resolved start removes it; the
  corrected hidden frame was inspected and no renderer change was needed.
- **Land of the Giants and Colony Zod:** the complete authored access and
  return route now passes in both Light and Dark C6. Retain Roland's cage,
  one-use key, both controls, Xenofex combat and the return altar while reducing
  the remaining approach teleports during ordinary-movement strengthening.
- **Shoals and Lincoln:** retain the complete access and item chain in both
  Light and Dark continuity tests rather than accepting destination teleports
  as campaign proof.
- **Representative regions generally:** the full audit still needs graphical
  play through outdoor and indoor regions, combat, travel, branch choices and
  an ending, plus representative UI, movie, music and sound checks on the Mac.

## Already durable; do not reopen without new evidence

These eleven independent routes have exact production-Save and
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
- Malwick/Niles successful tapestry return
- Malwick/Niles failed return and Harmondale invasion

They are independent route boundaries, not a continuous regional campaign.
The current regional side/promotion selection is 14/14; there are eighteen
exact regional writer/reader checkpoint tests across nine routes. The war and
two Malwick checkpoints are main/optional-path boundaries, bringing the total
to twenty-two exact checkpoint tests across eleven routes.

## Gate for each new quest slice

1. Exercise the authored offer or trigger and one meaningful refusal or
   alternate outcome.
2. Verify physical item or combat dependencies where the route requires them;
   name any teleport or party-strength fixture explicitly.
3. Assert quest bits, awards, XP, gold, reputation, class/alignment and world
   state as applicable.
4. Add a production-Save/fresh-process-normal-Load pair when the slice changes
   durable campaign state, with missing-profile and no-save admissions closed.
5. Run the focused test and changed-file lint. Reserve the broader regional,
   campaign and style gates for meaningful integration milestones instead of
   repeatedly rerunning known-green coverage. Reconcile neutral `master`
   before final integration.
6. Inspect every generated progress image before accepting it as evidence, and
   keep private data, saves and raw evidence outside Git.

