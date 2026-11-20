# MM7 remaining quests and areas — 2026-11-20

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
- A fixture-free New Game probe walked from the authored Emerald start to the
  first Dragonflies with only real forward/strafe/turn input and untouched
  party/actors. Their flying approach crossed outside the fixed viewport before
  reliable selection; the ordinary party died while seeking another angle.
  Preserve the existing representative movement/combat proof rather than
  forcing this route until the harness can hold movement and camera input
  together.

## Recommended quest order

| Priority | Quest group | Areas | Work remaining |
| --- | --- | --- | --- |
| 1 | War alternatives and Arbiter | Erathia, Tularean Forest, Avlee, Harmondale | The optional `mm7-war-dark` C0-C7 chain now branches at C2, completes both real war assignments, takes the neutral trumpet path and installs the Dark Arbiter before continuing through the existing Dark ending. The 27 effective result/recipient states remain covered separately. The shipped incomplete-result default remains documented rather than changed. |
| 2 | Malwick, Niles and the tapestries | Emerald Island, Harmondale, Tatalia, Castle Lambent, Castle Gloaming | Both full C0-C7 chains accept Malwick's Wand during Emerald C0, carry it through the authored Arbiter choices at C3, then reach the six-week letter and Niles's real offer in C4. Light physically takes Lambent's tapestry and Dark takes Gloaming's; each returns it before saving and reaches C7 in its own fresh marked chain. Dark C6 waits through the real rest UI for Gloaming's dawn throne hours after the changed campaign clock. The deadline/invasion alternatives retain focused and durable coverage rather than continuous-chain placement. |
| 3 | Dark campaign strengthening after C0-C7 | Harmondale, Deyja, Castle Gloaming, the Pit, Celeste, Thunderfist Mountain, Clanker's Laboratory, Breeding Zone, Colony Zod, Shoals and Lincoln | The separately marked Dark profile passes C0-C7. Dark C3 approaches Harmondale Castle through measured forward input; Dark C5 now also walks toward Kastore's Pit door before the real offer. Representative movement/combat has a reusable input contract, the Breeding Zone hidden frame is corrected, and Shoals' dense green fog is source-classified as authored underwater rendering. Other approach teleports and exact original Shoals pixels remain optional strengthening. |
| 4 | Light campaign strengthening after C0-C7 | Wine Cellar, Castle Gloaming, the Pit, Land of the Giants, Colony Zod, Shoals and Lincoln | Light C6 follows the authored Mount Nighon–Thunderfist–Tunnels–Land route through Xenofex and the return. The real Wine Cellar target now activates only for its live quest, input combat kills it, and Light C5 follows that fight with measured forward input to Crag Hack's Celeste house before the real hand-in. Tolberti's complete Blaster reward transaction is asserted. The Walls protected-group failure branch is covered synthetically because the shipped map has no group-1 actor. Remaining: reducing other approach teleports and broader graphical play. |
| 5 | Regional strengthening and durability | Emerald Island, Erathia, Bracada Desert, Stone City, Bandit Caves, White Cliff Cave, the Maze and the Erathian Sewers | All six contest sources are physical and their five inventory/NPC items persist through normal save/load. Lantern, Davrik and Griffin have exact production-Save/fresh-process-normal-Load boundaries. All 17 side/objective rows are durable. Thief-to-Rogue now adds an independent fresh-process checkpoint after the physical Markham Vase pickup: the normal Load retains the held vase, quest, classes, XP and gold; Lasker's hand-in then grants exact rewards without changing saved bytes. Other promotion and regional continuity checkpoints remain optional strengthening. |
| 6 | First promotions complete | All nine first-promotion routes | Every first promotion is behavior-covered through its real offer and objective. Priest physically loots the Tidewater map, verifies the exact Falk transaction and proves the Evenmorn boat unlock; Great Druid activates all three Stonehenges; Crusader uses attack-input Wromthrax combat; Wizard physically acquires and installs every golem component, covers both heads and proves the animated-golem Castle outcome. Remaining work is durability/regional continuity, not another first-promotion behavior row. |
| 7 | Second promotions complete | All eighteen Light/Dark second-promotion endpoints | All 27 promotion endpoints are behavior-covered. Ranger Lord physically returns the Mercenary Guild Heart of Wood; Ninja follows Stephan Sand's Erathian cipher through the Hidden Tomb sarcophagus; Bounty Hunter kills an authored Dragon C target through attack input and claims the real Harmondale Town Hall bounty; Black Knight uses Navan guard combat, treasury movement and the physical quest chest before Frederick Org's return. Remaining work is durability and regional continuity rather than another promotion behavior row. |

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
- **Barrow II:** the final hidden authored-start frame was inspected. Its very
  dark room and close foreground geometry are expected without Torch Light;
  the owner accepted the rendering. The complete 27-transition Barrow graph
  and the Lantern durability boundary remain the behavioral evidence.
- **Land of the Giants and Colony Zod:** the complete authored access and
  return route now passes in both Light and Dark C6. Retain Roland's cage,
  one-use key, both controls, Xenofex combat and the return altar while reducing
  the remaining approach teleports during ordinary-movement strengthening.
- **Mount Nighon:** both formerly dead tavern/training door halves now open
  through physical viewport picks, and the two group-26/27 peasants use the
  Warlock faction so the town's Warlocks do not attack them. Exact original
  signatures keep patched/custom map data authoritative.
- **Temple of the Moon:** the Emerald event-101 entrance, physical floor-tile
  pickup, authored door interaction and event-100 exit now pass as one route.
  Re-entry proves that the removed contest item does not respawn, while the
  existing Judge transaction remains the downstream control.
- **Strange Temple:** the reusable bottle enters through a real portrait
  right-click, the interior event-454 purchase yields the empty potion bottle,
  and event 501 exits to Harmondale. The shipped exit coordinates are outside
  the destination bounds, so OpenEnroth logs and skips the explicit placement.
  Headless tests now pin both the out-of-bounds source coordinate and the
  in-bounds Harmondale landing.
  Static native evidence shows the original copies the signed 32-bit coordinate
  and defers overflow handling to its first outdoor update; GrayFace 2.5.7 has
  no override. Trace the native settled position/dialog state before choosing
  between clamping, travel handling or no compatibility change.
- **Shoals and Lincoln:** retain the complete access and item chain in both
  Light and Dark continuity tests rather than accepting destination teleports
  as campaign proof.
- **Representative regions generally:** the full audit still needs graphical
  play through outdoor and indoor regions, combat, travel, branch choices and
  an ending, plus representative UI, movie, music and sound checks on the Mac.
  The Light C6 chain now measures keyboard movement in Colony Zod after the
  live Xenofex fight and before the authored exit; its fresh C0-C7 chain passed.
  Ordinary movement before the Colony entrance still needs a stable test route.

All 76 shipped MM7 maps now have runtime GameTest coverage; there are no
inventory-only map rows left. The remaining work includes broader ordinary
movement/combat, promotion and regional durability, the original MM7 Strange
Temple exit-placement parity check, and graphical play.

## Already durable; do not reopen without new evidence

These twenty-two independent routes have exact production-Save and
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
- Malwick accepted through the real Arbiter choice at the 1,007-hour pre-trigger boundary
- Stone City Troglodytes
- Haldar's Remains acquired from the repaired Maze source chest
- Darron's deck acquired from the White Cliff Cave world sprite
- Lantern of Light acquired from the persistent Barrow II source chest
- Davrik's Signet Ring acquired from Bandit Caves chest 0
- Griffin hunt at the both-regions-clear, pre-report boundary
- Evenmorn map physically acquired in Tidewater Caverns before Falk's first Priest promotion
- Grand Temple of the Moon altar used before Rebecca's Light second Priest promotion
- Grand Temple of the Sun altar used before Falk's Dark second Priest promotion
- Four empty Lich jars physically taken from the Walls of Mist chest before Wynac's return

They are independent route boundaries, not a continuous regional campaign.
The current regional side/promotion selection is 14/14; there are eighteen
exact regional writer/reader checkpoint tests across nine routes. The war and
two Malwick checkpoints are main/optional-path boundaries, and Stone City adds
one regional writer/reader pair, bringing the total to twenty-four exact
checkpoint tests across twelve routes. Haldar adds another pair at the repaired
source-marker boundary, bringing the total to twenty-six exact checkpoint tests
across thirteen routes. Darron's White Cliff acquisition adds one more pair,
bringing the total to twenty-eight exact checkpoint tests across fourteen
routes. Malwick's accepted pre-trigger state adds another pair, bringing the
total to thirty exact checkpoint tests across fifteen routes. Lantern, Davrik
and Griffin add six more exact tests, bringing that quest total to thirty-six
tests across eighteen durable routes. The three Priest promotion checkpoints
and physical Lich jars checkpoint add eight tests across four more routes.
Including the five older technical save/campaign seams, the inventory is 27
writer/reader pairs, below the owner's cap of 48 pairs. The earlier full
checkpoint sweep passed 23 pairs (46 separate processes); the four newer
promotion pairs passed independently in eight separate processes. Copied-save
primary and backup load probes also passed for two owner slots without changing
their originals; these read-only probes are not writer/reader pairs.

The broader map, spell, item, monster and archive audit is planned for the week
of 2026-11-23 in [the full-audit handoff](MM7_FULL_AUDIT_NEXT_WEEK_PLAN.md);
it has not started.

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

