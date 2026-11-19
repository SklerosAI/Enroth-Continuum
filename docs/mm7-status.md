# MM7 status

Where Might and Magic VII stands through 2026-11-16: what is playable and verified, the audit and its fixes, the
harnesses that prove it, the runtime paths and what remains.

## Summary

Codex owns MM7. It is the fork's first playable title and was released as `ECv0.1.0` on 2026-11-16 (see
[Releases](releases.md)). Its working gate closed on 2026-11-11: the current build launched through The Atlas, the
owner played visibly with music, voices and combat sound, saved, relaunched and loaded, and a chained test run reached
the Light ending through real saves. The full audit (stage 3 of the delivery order) continues alongside MM6 work.

## Working gate (closed 2026-11-11)

| Row | Evidence |
| --- | --- |
| W1 | The integrated source builds a staged app. A headless Save, quit, relaunch and Load passes in an isolated profile. |
| W2 | The Harmondale raid test passes on the real quest route |
| W3 | Owner session on the installed candidate: title music, NPC voice, combat sound, an NPC talk and a fight |
| W4 | Chapters C0 to C7 run from New Game through the shared opening and the Light branch to its ending, through real saves |
| W5 | The candidate replaced the installed app under The Atlas, relaunched and loaded a new save with existing saves intact |
| W6 | No known player-blocking MM7 defect |

## Playable and verified

- **Both endings.** The Light chain (`GameTests_MM7CampaignChainC0.cpp` to `C7`) and the Dark chain (common C0 to C2,
  then `GameTests_MM7DarkCampaignChainC3.cpp` to `C7`) each run eight fresh processes from New Game to their ending.
  Both endings clear their final quest bit from the Quest Book. Named fixtures remain: C5 party strength and some
  approach teleports.
- **Promotions.** All 27 promotion endpoints (nine first promotions, eighteen Light and Dark second promotions) are
  covered through their real offers, objectives, refusals, rewards and duplicate checks. `GameTests_MM7*Promotion*.cpp`.
- **Maps.** All 76 shipped MM7 maps have runtime game test coverage. The Barrow Downs region is covered through its
  27 authored transitions. `GameTests_MM7BarrowTraversal.cpp`.
- **Side quests and contests.** The opening contest acquires all six items physically. The war alternatives resolve
  all 27 result and recipient states under both arbiters. Malwick and Niles, the tapestries, Stone City, the Griffin
  hunt, Haldar, Darron, the Lantern, Davrik and the other regional quests have focused behavior tests.
- **Durable save boundaries.** Eighteen independent quest routes have exact production Save and fresh-process normal
  Load pairs with byte-identical slots. With five older technical seams, 23 writer and reader pairs run in 46
  processes. The routes are listed below.

Durable quest routes: Season's Stole, Kerrid and the Faerie King, Parsons' Quill, Missing Contestants, Three Paintings,
Three Statuettes, Arcomage Champion, the obelisk puzzle, the balanced war with neutral trumpet and Dark Arbiter,
Malwick and Niles success, failure and the 1,007-hour pre-trigger boundary, Stone City Troglodytes, Haldar's Remains,
Darron's deck, the Lantern of Light, Davrik's Signet Ring and the Griffin hunt.

The last consolidated gate (2026-11-15) passed the full build, every style target, the unit tests, the default MM7
game tests with only the opt-in mod cases skipped, the opt-in mod tests, all checkpoint pairs, both fresh chains and
both hidden capture tests.

## Audit and results

The 2026-11-07 source audit read the inherited engine against MM7 data in three parts: rules, campaign and events, and
platform, saves and identity. Its findings drove the fixes below. The audit's remaining rows are rules catalogs
(spells, items, classes, monsters, NPCs and routes, the campaign dependency graph) and Mac interface, media, mods and
long visible play. Items it found that have no fix yet are in [Open questions](open-questions.md).

### Combat audit fixes

All MM7-only behind title checks, each with a focused test that failed first:

- Break Weapon picks an equipped weapon, and an unarmed target is safe.
- MM7 liches take no Mind or Body damage.
- Divine Restoration clears the target's conditions, not the drinker's.
- `FireAr` resolves to the flaming archer arrow.
- Divine Intervention: higher Light skill shortens recovery. A Zombie keeps its Zombie condition.
- Monster spell projectiles carry the selected spell's ability, not the first slot's.
- `Ener` in a monster attack field parses as Energy, not Earth.
- Event Day of the Gods at Master and Grandmaster matches the ordinary cast's power.
- Death explosions use their own special damage dice and type.
- Spirit Lash out of range keeps its mana and takes failure recovery.
- Charged wands use the novice recovery row for both cast and display.
- Character Fate adds its one-use hit bonus, monster Fate subtracts it.
- Squire adds 2 to every weapon and armor skill, Sage adds 6 to Item ID and Monster ID.
- The turn queue no longer skips actor index 10.

### GrayFace reconciliation

GrayFace MM7 Patch 2.5.7 is a bug index and compatibility oracle, never code to copy. Each finding is reproduced
against shipped data first, then fixed with an exact-signature compatibility seam that disengages on patched or custom
data. Closed so far: the Wine Cellar Vampire's Cape carrier, Loren Steel and Riverstride timeouts, Roland's key and the
Colony Zod movie replay, Castle Harmondale banners, advisor flags and early golem, the quest-item chests in Titan's
Stronghold, the Breeding Zone, Tidewater and Barrow II, reappearing paintings, Haldar's Remains, the Champion Arena
total, the Avlee rock, Mount Nighon doors and peasants, the final quest left in the journal, Deyja's guards, the
Giants' wishing well charge, Kastore's swapped texts, the Spy greeting, Lady Carmine's dagger, Tor Anwyn's greeting and
the Lich jars.

### Items, artifacts and scrolls

- Level-seven treasure marks its artifact found and honours the artifact cap. An indoor spawned artifact keeps its
  item so it can be picked up. `GameTests_0500.cpp`.
- Item-, actor- and character-target scrolls return to the cursor when the target pick is cancelled, also when the
  caster is knocked out. `GameTests_MM7Scrolls.cpp`.

### Transition crash repair

An owner playtest on 2026-11-15 crashed when the Enter that accepted the Dragon's Cave transition also reached the
turn-based toggle. The nearby-AI list still held Emerald Island actor IDs, which indexed the smaller indoor actor list.
World preparation now clears that list before replacing actors, and turn-based admission rejects out-of-range IDs.
`MM7TransitionCombat.StalePreviousMapActorIsIgnoredAtTurnBasedStart`.

## Harnesses and runners

- **Gameplay harness.** `driveMovementInput` presses, holds and releases a real key for a bounded number of frames.
  `fightActorWithAttackInput` attacks through the attack key and records the target's HP. Setup may teleport, the
  measured segment may not. `tools/run_mm7_gameplay_harness.py` runs the `MM7GameplayHarness`, `MM7CombatHarness`
  and optional `MM7BarrowTraversal` suites in fresh headless processes.
- **Campaign chain runner.** `tools/run_mm7_campaign_chain.py` runs chain chapters in order in fresh processes, each
  loading the previous chapter's save from a marked disposable profile. `--title mm6` runs the MM6 chain.
- **Disposable save gate.** A writer saves into an owner-only temporary profile and a reader in a separate process
  loads it through the normal Load menu and checks the slot is byte-identical. `GameTests_MM7DisposableSave.cpp`.
- **Hidden captures.** `MM7OffscreenFrame.*` renders frames offscreen without a visible window. Every image is
  inspected whole before it counts as evidence.

## Saves

MM7 native saves are written atomically: complete bytes are staged beside the slot and one valid previous primary is
kept as `.bak`. Loading runs a bounded structural preflight over framing, decompression, map deltas, images, item,
actor and door references. A corrupt primary falls back to a valid backup without rewriting either file. Serialization
no longer mutates live state until the write succeeds. `src/Engine/MM7/MM7AtomicSaveFile.cpp`,
`src/Engine/MM7/MM7SaveValidation.cpp`.

## Runtime paths and save migration

The Atlas and the direct MM7 helper default to `Game Data/MM7` and `Profiles/MM7-Atlas` under
`~/Library/Application Support/Enroth Continuum/`. `ENROTH_CONTINUUM_MM7_PATH` overrides the data path, the older
`OPENENROTH_MM7_PATH` still works when the new one is unset. The helper writes `enroth-continuum-mm7.ini` and a rotating
`enroth-continuum-mm7` log, and reads a legacy `openenroth.ini` once without touching it. Startup no longer copies saves
out of the data folder.

Older save folders hold different saves with colliding names. They are never merged by filename. Saves move into
`Profiles/MM7-Atlas/saves` by hand, one explicit choice per collision, with both originals kept.

## Mods

The Atlas Atelier offers opt-in MM7 mods for a Private mods launch only: portrait moods, an armor AC override, vendor
stock refreshed on entry and All Artifacts + Storage (every spawnable artifact obtainable once, Castle Harmondale's
chests as storage). Details are in [Mods and Atlas](mods-and-atlas.md).

## What remains

- Visible Mac play through representative regions, combat, travel, branch choices and an ending, with UI, movie,
  music and sound checks.
- Fewer approach teleports in the chains, and ordinary movement and combat in more regions.
- The Strange Temple exit-placement parity trace and the Walls protected-group predicate audit.
- The rules catalogs and campaign dependency graph of the audit.
- The open items in [Open questions](open-questions.md).

## Source

- [MM7AtomicSaveFile.cpp](../src/Engine/MM7/MM7AtomicSaveFile.cpp), [MM7SaveValidation.cpp](../src/Engine/MM7/MM7SaveValidation.cpp)
- [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp), [MM7ProfilePath.swift](../launcher/MM7ProfilePath.swift)
- [MM7CampaignChain.h](../test/Bin/GameTest/MM7CampaignChain.h), [MM7GameplayHarness.h](../test/Bin/GameTest/MM7GameplayHarness.h)
- [GameTests_MM7TransitionCombat.cpp](../test/Bin/GameTest/GameTests_MM7TransitionCombat.cpp)
- [GameTests_MM7Scrolls.cpp](../test/Bin/GameTest/GameTests_MM7Scrolls.cpp)
- [run_mm7_campaign_chain.py](../tools/run_mm7_campaign_chain.py), [run_mm7_gameplay_harness.py](../tools/run_mm7_gameplay_harness.py)

See also: [Project overview](project-overview.md), [Releases](releases.md), [Open questions](open-questions.md), [Mods and Atlas](mods-and-atlas.md), [Testing](testing.md)
