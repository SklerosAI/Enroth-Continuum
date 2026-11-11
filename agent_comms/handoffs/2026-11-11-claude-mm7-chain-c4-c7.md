# MM7 chain chapters C4-C7 handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm7-chain-c4-c7` in the `mm7-death-regression` checkout, rebased onto `master` `e63e1faf7`. Commits `304f5a8cb`, `ae10b08ac`, `fddd7e265`, `bbdb2b9d3`, `cbf4bce7d`, `5daab13bc`. Not pushed. The `codex/mm7-light-pit-xenofex-seam` ref is unchanged.
- Files: `GameTests_MM7CampaignChainC4.cpp` to `C7.cpp`, CMake lines, and additions to `MM7CampaignChain.{h,cpp}`. The one edit to an existing helper: `loadCampaignChainSlot` scrolls the Load menu once there are more than seven slots.

## Chapters

- C4: Bracada to the Celeste portal, the Castle Lambent throne offer in daytime, the Walls of Mist lever puzzle, the three pillar keys from their chests, the pedestals, the opened exit and the hand-in. The closing tapes pin NPC 79-83 houses and topics.
- C5: both altar pieces from the temple chests, the Wine Cellar exit route, the soul jars from Castle Gloaming, Robert's request, the Pit, the real Tolberti fight with the cube looted from the corpse, and the hand-in. The one party-strength fixture is applied here (level 200, speed and endurance 200, body resistance 1000, a melee damage bonus) and carried in later saves. Tolberti heals faster than a C0-strength party damages him, and his attack eradicates on a failed body-resistance roll.
- C6: the Xenofex request, real healing at the Celeste temple through dialogue, the real Xenofex fight, the map exit, the hand-in and the final-task topic.
- C7: wetsuits from the Castle Lambent chests, equipped through the inventory and paperdoll UI, the Avlee border into the Shoals, Lincoln, the console and the Overthruster, the exit, then the throne hand-in to the game-over window and the Harmondale return. It checks the certificate and a fresh autosave, removes the certificate so the profile stays clean, and writes save007.
- No quest-bit or NPC-field writes anywhere. Teleports between maps are named. No NoActors.

## Checks

- Fresh `--through 7`: exit 0, eight slots and markers, about 25-42 s. A rerun says already passed.
- Behavior guards: a key left in its chest, a skipped temple chest, no attacks on Xenofex, or a missing fourth wetsuit each fail the chapter with no slot or marker.
- In-memory `MM7Light*:MM7Xenofex*:MM7Overthruster*:MM7PitCube*:MM7Campaign.*` 27/27. Legacy `MM7LightWalls`, `MM7OverthrusterRoute`, `MM7LightSeam` pairs pass. cpplint and `git diff --check` clean. Full headless MM7 binary 538 passed, 2 skipped.

## Open

- Once in about 16 fresh runs, the Walls of Mist exit stayed closed after all three keys were looted. The chapter now retries each pedestal click until its key is consumed, which could mask an intermittent click defect. Stage 3 audit item.
- The strength fixture proves the quest mechanics, not combat balance at a natural level.

Privacy: no game data, saves or logs in Git.
