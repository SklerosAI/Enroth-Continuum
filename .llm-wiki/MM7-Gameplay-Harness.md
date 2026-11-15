---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[MM7-Offscreen-Frame-Capture]]", "[[MM7-Quest-Behavior-Catalog]]", "[[Agent-Play-Knowledge-and-Control-Contract]]"]
tags: [mm7, testing, movement, combat, barrows, promotions, graphics]
---

# MM7 Gameplay Harness

The MM7 gameplay harness supplies bounded input-driven movement, attack-input combat, authored Barrow traversal and opt-in hidden visual evidence without claiming autonomous navigation.

## Runner and contracts

`tools/run_mm7_gameplay_harness.py` fails closed unless the built GameTest binary contains both `MM7GameplayHarness` and `MM7CombatHarness`; `--with-barrows` also requires `MM7BarrowTraversal`. Each suite runs in a fresh headless process against an explicit private MM7 data path.

`driveMovementInput` posts one real key press, advances a bounded 1–512 frames, posts the matching release and advances a release frame. Its result records map, position, yaw and release state. An RAII guard releases the key even when a contract fails. Setup may teleport before the measured segment; the helper itself cannot teleport.

`fightActorWithAttackInput` repeatedly presses the attack key, advances bounded frames and invokes a caller-supplied target reacquirer. It records actor identity, map/screen/title, attack count, initial/lowest/final HP, damage observation and terminal/dead state. Tests must identify any positioning or party-strength fixture separately.

## Integrated proof

The composed Wine Cellar case ends all setup teleporting before measurement, walks forward through held/released input, reduces distance to the authored Elder Vampire, then uses mouse targeting plus attack input until HP falls from 374 to zero and the actor settles in `Dead`. The hidden OpenGL variant asserts the same movement, release, damage, death, title, map and screen conditions before writing a 640×480 PNG. The inspected frame shows the cellar, full HUD, the fallen red-clad target and the killing-blow message.

The shipped Wine Cellar data stores actor zero, the uniquely named group-5 Vampire C checked by event 2, disabled. Event 1 makes group 5 aggressive but did not activate that quest target. A narrow MM7-only compatibility repair now clears its AI-disabled flag during that exact map/event/group/quest-active transaction. Finished or already-killed quests do not revive it, and the other two disabled group-5 actors stay disabled.

The Barrow suite visits Barrow Downs, Barrows I–XV and Zokarr's Tomb through 27 authored transitions, including lever-selected and deterministic random branches. It is transition coverage with floor-resolved face approaches, not continuous natural walking. Zokarr's meditation pool additionally covers Monk-to-Initiate and honorary outcomes, exact XP/awards/topics and duplicate prevention. The Light Arch Druid packet uses Green's real offer, proves the apparent Hothfarr/event-178 handoff is unreachable dead original content, acquires the skull through live Nighon Tunnels event 196, deposits it at Zokarr's face and completes Green's exact qualifying/honorary reward and duplicate guard.

Ranger-to-Hunter, Druid-to-Great-Druid, Archer-to-Warrior-Mage, Knight-to-Cavalier and Paladin-to-Crusader first promotions now use their real dialogue and physical/event routes. Ranger covers the Hall Under the Hill audience and qualifying/honorary transaction. Great Druid proves zero/partial refusals and physically activates the Evenmorn, Tatalia and Avlee Stonehenges before Green's exact return. Archer covers the supplied belt, physical Red Dwarf Mines sabotage face, return rewards and duplicate prevention. Knight covers Frederick Org's dialogue and the Haunted Mansion map-leave clear predicate, including the authored no-gold reward. Crusader kills Wromthrax through attack input, waits through the authored 210-second completion timer and verifies Quixote's no-gold promotion, deferred dismissal and duplicate protection.

## Visual findings

Corrected hidden captures at authored starts show coherent Tatalia and Avlee terrain, HUD and minimaps. Shoals' dense green view matches its explicit underwater contract: weak/strong fog distances are 50/2000 and both fog and terrain light use Eucalyptus green. Wetsuits gate access and drowning, not fog.

The older Breeding Zone scene used literal indoor `(0,0,0)` and intersected a near-plane face, producing a large black occluder. Replacing that diagnostic placement with the existing authored, floor-resolved start removed the occluder; no renderer change was required.

Selected inspected PNGs remain private under ignored `local-game-data/progress-captures/`, with a hard rolling limit of 100. They are evidence aids, not redistributable assets.

## Limits and resume point

- The harness does not provide pathfinding, obstacle avoidance, camera planning or natural-start route solving.
- The Wine Cellar approach is short and begins from a setup-aligned melee lane.
- The Barrow test proves authored connections but teleports to floor-resolved face approaches.
- Exact Shoals pixel parity with the original executable is not established even though the green short-range fog is authored.
- Run focused checks for new slices; reserve the full harness, campaign chain and style sweep for meaningful integration milestones rather than repeating known-green tests.

The independent [GrayFace patch reconciliation](MM7-GrayFace-Patch-Reconciliation.md)
confirms the Wine Cellar target defect and supplies a public priority index for
later map and quest checks without licensing its implementation into this fork.

See also: [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Quest Behavior Catalog](MM7-Quest-Behavior-Catalog.md), [MM7 GrayFace Patch Reconciliation](MM7-GrayFace-Patch-Reconciliation.md), [Campaign Status](Campaign-Status.md), [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md).
