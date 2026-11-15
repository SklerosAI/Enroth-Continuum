---
type: synthesis
up: "[[MM7-Quest-Behavior-Catalog]]"
source: "https://grayface.github.io/mm/MM7PatchReadMe/"
related: ["[[MM7-Gameplay-Harness]]", "[[MM7-Offscreen-Frame-Capture]]"]
tags: [mm7, grayface, bugs, quests, maps, reconciliation]
---

# MM7 GrayFace Patch Reconciliation

GrayFace MM7 Patch 2.5.7 is a public compatibility oracle and bug-index for the MM7 audit, not code to copy into the OpenEnroth fork.

## Pinned source and boundary

The reference is the official [`MM7Patch-2.5.7` tag](https://github.com/GrayFace/Misc/tree/MM7Patch-2.5.7), commit `83152de4bbab89436e3b6918ec2bc8d7164a285b`, plus the official [MM7 patch readme](https://grayface.github.io/mm/MM7PatchReadMe/). Its repository license is GPL v2-or-later with three additional clauses. Enroth Continuum may use its documented behavior and public patched-data deltas as evidence, but must not copy source or binary/map assets into tracked OpenEnroth files without a separate license decision. The pinned checkout and extracted comparisons stay in ignored local research.

GrayFace is not automatically the design authority: each row still needs reconciliation against current OpenEnroth source, shipped MM7 data and a focused behavior test. Optional changes and convenience features are not vanilla requirements.

## High-value reconciliation queue

| GrayFace finding | Current Enroth evidence | Action |
| --- | --- | --- |
| Wine Cellar's unique Vampire's Cape carrier did not appear; the Light quest should require killing it. | Independently reproduced. GrayFace's decompressed `d16.dlv` changes exactly three actor-zero bytes: clears `ACTOR_UNKNOW11`, changes AI state `Disabled` to `Standing`, and resets unique-name index 5 to 0. Enroth instead preserves the shipped identity and activates that exact actor only during the live quest event; attack input then kills it and completed/killed blockers prevent revival. | Closed; preserve the narrower event-time activation contract and false-exit guard. |
| Loren Steel and Riverstride Plans could reward after timeout. | Both timeout fallthroughs were independently found, repaired and covered across real/decoy/timeout matrices. | Closed; retain war tests. |
| Roland could give his key repeatedly and the Colony Zod exit replayed Archibald's trial movie. | Current Light/Dark C6 consumes the one-use key and covers the route; repeat-movie behavior is not separately asserted. | Keep the key guard; add a focused repeat-exit movie/event check only if the current source path remains ambiguous. |
| Castle Harmondale could show quest zero, choose wrong/missing alignment flags or show the golem early. | The early-golem defect was independently reproduced: assembled qbit 73/74 enabled group 6 and dismissed NPC 56 before Thomas set animation qbit 46. A narrow event-376 guard and full good/dented Wizard routes now pass. The quest-zero and alignment-flag presentation cases are not separately named. | Early golem closed; retain focused flag/quest-log assertions for the remaining presentation cases. |
| Quest-item chests in Titan's Stronghold, Breeding Zone, Tidewater Caverns and Barrow II needed better handling. | Barrow II Lantern acquisition works. The Breeding Zone defect was independently reproduced and repaired: closing source chest 0 before pickup no longer redirects the same map incarnation to chest 19, while a fresh retrieved map still prevents respawn. The Titan and Tidewater reopen/lifecycle cases remain separate. | Breeding Zone closed; add representative Titan/Tidewater chest persistence cases. |
| Haunted Mansion and Gryphonheart paintings could reappear after reentry. | Physical pickups, return and durable checkpoint pass. | Add a narrow reentry-visibility control rather than reopening the whole quest. |
| Maze Haldar's Remains could disappear if left in the chest on first open. | Physical acquisition and return pass, but the leave-item/reopen branch is untested. | Add the reopen branch alongside Haldar durability. |
| Champion promotion should accept Lord as well as Knight Arena difficulty. | Independently reproduced. The actual compatibility rule is a combined Knight+Lord total of five, including mixed totals. A narrow MM7 global-event-72 repair now passes Leda dialogue cases for 5+0, 0+5 and 4+1 while rejecting 2+2 and lower difficulties. | Closed; preserve the event-scoped sum and generic counter semantics. |
| Barrow II had a wall-spawned rat; Avlee had a flying rock; Nighon had door/actor problems. | Complete Barrow graph and corrected Avlee capture exist, but exact actor/geometry sites are not asserted. | Use the patched archive as a locator, then verify current OpenEnroth/shipped-data behavior before changing anything. |
| Final quest remained in the journal after the ending. | Both endings pass; exact final-log cleanup is not a named assertion. | Add quest-log/history cleanup to ending controls. |
| Deyja northern guards were always hostile. | Dark campaign visits Deyja but does not isolate this guard disposition. | Add a focused relation/hostility check if current data reproduces it. |
| Land of the Giants well could grant its effect without charging 5,000 gold. | Both campaign routes traverse the region; the well is not covered. | Add payment/refusal/duplicate assertions as a regional side interaction. |
| Kastore's altar-piece response texts were swapped and William Lasker's post-Spy greeting said Rogues. | State transactions are covered more strongly than these presentation strings. | Audit localized text only when the corresponding second-promotion/UI slice is active. |

## Use in future lanes

1. Search the readme and public patch archives for the map, quest or mechanic before broad source archaeology.
2. Treat the patch delta as a locator and hypothesis, not an automatic oracle.
3. Reproduce against current shipped data/OpenEnroth behavior.
4. Add the smallest refusal/success/duplicate or visual control that distinguishes the defect.
5. Prefer an independent implementation based on the behavior contract; never paste GrayFace code into this fork.
6. Record whether the result is vanilla parity, an optional GrayFace behavior or a current OpenEnroth-only repair.

The Arch Druid audit demonstrates the negative case: GrayFace 2.5.7 leaves
Green's global events 100/101, dead Hothfarr event 178 and Nighon Tunnels event
196 byte-identical to the shipped scripts. That evidence helped reject an
engine-parser theory and identify event 196 as the live skull source without a
production change.

The Great Druid audit is another clean baseline: global offer/return events 98
and 99 are byte-identical, and GrayFace supplies no overrides for the three
outdoor Stonehenge maps. The physical promotion packet therefore required no
compatibility repair.

The Priest audit found the same authored Falk and Tidewater event-176
transaction in GrayFace 2.5.7. The physical map hand-in and Evenmorn boat
unlock pass without a production repair; GrayFace's broader Tidewater
quest-chest work remains a separate lifecycle follow-up.

See also: [MM7 Quest Behavior Catalog](MM7-Quest-Behavior-Catalog.md), [MM7 Gameplay Harness](MM7-Gameplay-Harness.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
