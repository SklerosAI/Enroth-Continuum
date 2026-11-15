---
type: source-summary
up: "[[Home_Enroth-Continuum]]"
source: "GrayFace MM6 Patch 2.5.7, https://github.com/GrayFace/Misc tag MM6Patch-2.5.7"
tags: [mm6, grayface, vanilla-bugs, data-fixes, reference]
---

# MM6 GrayFace Patch Reference

The GrayFace MM6 Patch 2.5.7 is pinned locally and cross-referenced against our MM6 runtime, so every vanilla MM6 bug the patch fixes has a recorded answer: reproduced, fixed, deliberately vanilla, or not reachable yet.

## What was pinned (2026-11-15)

| Item | Location (all ignored by git) |
| --- | --- |
| Patch source at tag `MM6Patch-2.5.7`, commit `5d95fa3a`, the latest MM6 patch commit on upstream master | `local-game-data/public-references/GrayFace-Misc-MM6Patch-2.5.7/` |
| Hook tables with one-line fix comments | `MMPatches/MM6Patch/Hooks.pas:2913-3169`, `MMPatches/MMCommon/MMHooks.pas:4018` and `:4180` |
| Shipped readme with the full fix history | `MMPatches/Install/MM6patch/Files/MM6Patch ReadMe.TXT` |
| Inflated patch data and diffs against the owner's unpatched GOG 1.2 files | `local-game-data/public-references/GrayFace-MM6Patch-2.5.7-decoded/` |

The owner's GOG MM6 install carries no GrayFace files. The engine reads official 1.2 data.

## What the patch changes

- About 90 engine hooks that fix vanilla MM6 logic. The tracked cross-reference is [MM6 GrayFace Patch Reference](../project/MM6_GRAYFACE_PATCH_REFERENCE.md), tables C1 to C29 (combat and spells) and W1 to W29 (world, events, saves, UI).
- 34 patched event scripts and 21 patched map files, shipped as `Data\00 patch.*.lod` override archives. The scripts add outdoor entrance hints, fix chest and quest-item handling in eleven dungeons, the Oracle hints, the Monolith and Agar's Laboratory rewards, the circus and Slicker Silvertongue repeats, and add the Free Haven Smuggler's Guild entrance. Four scripts use a new opcode 69 `RefundChestArtifacts`.
- Platform and QoL work (32-bit colour, window sizing, mouse look, quick saves, MP3) that the engine already covers or doesn't need.

## Headline findings

| Group | Count | Examples |
| --- | --- | --- |
| Vanilla bug we still reproduce, two of them half | 14 | stronger-but-shorter buff refused (C14), monsters jump into lava (C24), fare taken before the travel autosave (W7), chest overflow items hidden forever (W13), shops buy blasters (W26), unconscious members identify in shops (W28), found-artifact flags never cleared on respawn (W12) |
| Deliberately vanilla, pinned by tests | 4 | Static Charge exe cost (C12), 426-tick turn (C18), item roll bias (C25), flying border offer (W9) |
| Fixed, matching the patch | 24 | monster spell element (C1), AOE on paralyzed (C3), Scholar (C7), timed cures (C10), Water Walk drain (C13), negative resistance (C17), timer catch-up burst (W1), new day at month start (W4), SFT timing (W24) |
| Not reachable, spell or feature not admitted | 8 | monster Paralyze (C2), weapon enchants (C4), Finger of Death (C9), Town Portal and Lloyd's Beacon (W10), wands (W20), scrolls (W21) |
| Inherited MM7 shape that is wrong for MM6 | 4 | recovery enchant +50% (C6), weapon-skill hirelings (C8), armor recovery table (C22), stats-screen condition (W19) |

Eight fixes live in shared code with only MM7 tests or none: C3, C7, C21, C27, W4, W6, W17, W24.

## How to use it

- Before claiming a vanilla MM6 behaviour, grep the hook comments. A hook means the original is buggy there and the choice is ours.
- Before testing chests, fountains, altars or quest rewards on a patched map, read that map's record-level diff in the decoded folder. A test that pins the original script may pin a bug.
- Map problems listed under the readme's "Fixes in maps" are data, not engine. Using the patched data needs opcode 69 and an archive overlay the engine doesn't have yet.

## Superseded claims

None. This page is new.

See also: [MM6 Implementation Checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md), [MM6 F4 Spell Effect Dispatch](MM6-F4-Spell-Effect-Dispatch.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Campaign Status](Campaign-Status.md).
