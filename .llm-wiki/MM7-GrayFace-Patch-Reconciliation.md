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
| Roland could give his key repeatedly and the Colony Zod exit replayed Archibald's trial movie. | Light/Dark C6 consumes the one-use key. The exact original Colony exit event now jumps from the completed qbit-105 check past the Family Reunion movie and History 24 write; first completion, repeat completion, patched-shape disengagement and the adjacent Xenofex map-leave path pass 3/3. | Closed; retain the one-use key and exact one-time movie guard. |
| Castle Harmondale could show quest zero, choose wrong/missing alignment flags or show the golem early. | All three classes are reconciled. Quest-zero presentation was already fixed while retaining durable qbit 271. The early golem requires Thomas's animation bit. Advisor flags now require matching alignment, and the 12 complete war-result combinations select human/neutral/elf banners while incomplete or contradictory states retain base textures. | Closed; preserve Issue2903, the golem gate and the focused flag matrix. |
| Quest-item chests in Titan's Stronghold, Breeding Zone, Tidewater Caverns and Barrow II needed better handling. | All four are independently reconciled. Barrow II records only a successful source-chest-2 open in map variable 2, preserves it through close/reload and award state, and routes fresh award state without the marker to fallback chest 0. Trap timing, lifecycle, legacy fallback and custom-shape cases pass with the existing Lantern route. | Closed for all four quest-chest contracts; GrayFace opcode-45 artifact refunds remain separate. |
| Haunted Mansion and Gryphonheart paintings could reappear after reentry. | Independently reproduced for the Angel, Roland and Archibald paintings: their taken bits persisted but the wall portrait texture returned. Exact original-event reload restoration now reapplies each authored empty-wall texture; the walls remain clickable without duplicating the item, and patched-shaped data disengages. | Closed; retain the three-map reentry and sibling-isolation controls. |
| Maze Haldar's Remains could disappear if left in the chest on first open. | Independently reproduced. Opening source chest 1 set qbit 161 before pickup, so a later click selected chest 0 and stranded the item. An exact Maze event-176 seam sets map variable 0 after successful source open and uses it to reopen chest 1; legacy marker-zero routing remains unchanged. | Closed; fresh-process quest durability remains separate from the chest lifecycle. |
| Champion promotion should accept Lord as well as Knight Arena difficulty. | Independently reproduced. The actual compatibility rule is a combined Knight+Lord total of five, including mixed totals. A narrow MM7 global-event-72 repair now passes Leda dialogue cases for 5+0, 0+5 and 4+1 while rejecting 2+2 and lower difficulties. | Closed; preserve the event-scoped sum and generic counter semantics. |
| Barrow II had a wall-spawned rat; Avlee had a flying rock; Nighon had door/actor problems. | All three are independently reconciled. Avlee's unique original `rock01` is repaired; Barrow II's exact-center rat is already on a valid floor. In Nighon, the two changed ODM bytes are model 0 face 38 event 12→11 and model 12 face 17 event 10→9, while patched event 1 assigns groups 26/27 to Warlock faction 74. Exact shipped signatures now apply those same behavior corrections; both physical doors open and the two peasants are mutually friendly with Warlocks. | Closed without a general spawn-radius or opcode-55 implementation; retain the four focused Nighon cases and patched/custom disengagement. |
| Final quest remained in the journal after the ending. | Independently reproduced for both branches. Successful events 170/172 cleared item, Oracle bits and topic but left active qbits 130/131. An exact success-path seam now clears only the active bit while preserving History 25/26; no-item refusals keep the quest active. | Closed; update broad chain expectations at the next integration milestone. |
| Deyja northern guards were always hostile. | Independently reproduced. Three group-4 guards retained hostile Goblin faction 25 on normal Dark/neutral visits. An exact Deyja map-load seam applies friendly faction 78 to normal/expired states and restores default hostile factions for Light, active 30-day anger or stored hostility; wilderness group 5 is unchanged. | Closed; retain the 719/720-hour faction controls. |
| Land of the Giants well could grant its effect without charging 5,000 gold. | Independently reproduced through the real face: qualifying wishes were free and repeatable. An exact event-205 compatibility seam charges 5,000 before the authored random branch while preserving the peculiar 4,998/4,999 insufficient-funds behavior. | Closed; retain the physical face and boundary controls. |
| Kastore's altar-piece response texts were swapped and William Lasker's post-Spy greeting said Rogues. | Both defects are independently reproduced and repaired. The Spy greeting uses an exact original-English normalization. Kastore's exact 22-step global event 158 has its two one-piece response references corrected only when the complete original signature matches; `KastoreNamesTheMissingAltarPiece` drives both cases through Kastore's physical Pit dialogue. | Closed; retain the exact-string Spy guard and full-signature Kastore two-branch control. |
| Celeste's Dark map-leave recovery check could grant Lady Carmine's dagger before she was killed. | Independently reproduced. Event 378 tested an empty actor group, which passed vacuously and enabled Oracle recovery while actor zero was alive. An exact Celeste event/group compatibility seam now evaluates Lady Carmine herself; generic empty-group semantics remain unchanged. | Closed; preserve the real target kill and retained-dagger transaction. |
| Warlock completion assigned Tor Anwyn's congratulatory greeting to Anthony Green. | Independently reproduced. Global event 103 targets NPC 50 even though NPC 51 is the promoter and greeting 83 is Warlock-specific. GrayFace preserves the bad target; an exact event/instruction compatibility seam redirects only this greeting to Tor. | Closed; preserve the authored reputation direction separately from the corrected recipient. |
| The Lich promotion's empty jars could become inaccessible after completing the adviser Soul Jars quest. | Independently reproduced. Walls event 176 used unrelated award 80 to route the active Lich quest to empty chest 1 and never read its own persistent chest marker. An exact original-event seam now uses active qbit 48 or decor 23 to select chest 0 and preserves that route after reload. | Closed; retain the 3/4 refusal, four-jar conversion and fresh inactive fallback. |

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

The final Ranger Lord, Ninja, Bounty Hunter and Black Knight promotion packets
and the Temple of the Moon and Strange Temple routes likewise found no relevant
GrayFace 2.5.7 behavioral delta. Their focused physical/input-driven coverage
required no production compatibility change.

The war exposure topics and Harmondale result resolver are also byte-identical
in GrayFace 2.5.7. Both reachable death/failure branches, the five trumpet
handlers and representative/full state resolution therefore remain authored
behavior tests, not compatibility repairs.

The Priest audit found the same authored Falk and Tidewater event-176
transaction in GrayFace 2.5.7. The initial physical hand-in passed, but a
deeper lifecycle audit showed the missing legacy/fresh-state distinction.
An exact D17 event-176 seam now records source chest 1 in map variable 2,
preserves it across close and reload, and sends fresh recovery-bit or either
Priest award state to fallback chest 0. The focused Priest packet plus the
adjacent Breeding control pass 8/8, including custom-shape disengagement.

The two second-Priest routes are also byte-identical between the shipped global
events and GrayFace 2.5.7 for events 86/87/90/91. Both Light and Dark returns
subtract 10 current-location reputation, so the Dark direction is preserved as
authored rather than treated as a repair candidate. Real promoter dialogue,
physical Moon/Sun altar faces, refusals, exact rewards and duplicate stability
pass 6/6 with the adjacent first-Priest route.

A cumulative guard review hardened the exact Wine and Lady Carmine kill seams
against alert-filtered vacuous success by reading the validated target actor's
state directly. It also made the Castle golem pre-animation guard require the
complete original event-376 signature and disengage for already-patched or
custom qbit-46-gated data. The combined Assassin, Wine and Wizard controls pass
12/12 while generic empty-group semantics remain unchanged.

The Three Paintings reload audit reproduced a visible persistence defect in all
three source walls: the taken quest bit survived map reentry but the portrait
texture returned. A narrow map-load repair recognizes each exact six-record
pickup event and reapplies its own authored empty texture after normal reload
triggers. The 3/3 focused selection proves all three walls, no duplicate item,
Roland/Archibald sibling isolation and patched-shape disengagement.

The Avlee flying-rock audit found the exact GrayFace geometry defect in current
OpenEnroth data: the unique `rock01` at (-15088, -4288) loaded at Z 208 while
the non-water terrain floor is Z 148. A narrow post-reload correction requires
the complete original decoration signature and unique coordinate before using
the live floor height. The focused original and custom-shape controls pass 2/2.

The Colony Zod exit audit reproduced GrayFace's one-time presentation intent
against the current exact six-record event. A completed qbit-105 state now
jumps to the authored exit before the Family Reunion movie and History 24
steps; first completion still executes both. Two focused movie-probe cases and
the adjacent Xenofex map-leave control pass 3/3, while custom event shapes
disengage.

The Barrow II Lantern audit closes the last named quest-chest lifecycle. Exact
event 177 records map variable 2 only after source chest 2 successfully opens,
then preserves that identity across close, save/reload and award state. Fresh
award state without the marker retains fallback chest 0. The same packet proves
the authored rat center lies on a valid indoor floor, so the GrayFace radius
delta is not a current OpenEnroth behavior change.

See also: [MM7 Quest Behavior Catalog](MM7-Quest-Behavior-Catalog.md), [MM7 Gameplay Harness](MM7-Gameplay-Harness.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
