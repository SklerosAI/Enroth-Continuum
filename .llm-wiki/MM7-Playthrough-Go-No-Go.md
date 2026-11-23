---
type: synthesis
up: "[[Campaign-Status]]"
derived_from: ["[[Campaign-Status]]", "[[Release-Milestones]]", "[[MM7-Combat-Audit-Fixes]]"]
tags: [mm7, status, gate]
---

# MM7 Playthrough Go/No-Go

Whether every MM7 game system is ready for an owner's end-to-end playthrough, from New Game to an ending, and what
still stands in the way. The MM6 counterpart is `docs/mm6-playthrough-go-no-go.md` on `claude/mm6`.

## Verdict

**GO.** Assessed on `codex/mm7` at `617b94d23`, 2026-11-23.

All four conditions hold on that commit:
1. The full default MM7 headless suite passed, and so did the unit tests. The only skipped unit tests are MM6 tests
   that need MM6 data. The suite ran at `780722681`, whose source matches `617b94d23`.
2. The Light and Dark C0-C7 chains passed from new temporary profiles, each chapter in a fresh headless process.
3. The installed Atlas carries a Release MM7 helper from `014514250`. Every commit since then changes only
   documentation, so the helper runs the assessed source.
4. No player-reachable throw or assert is known. A read of every fork-added `throw` and `assert` and of the MM7
   gameplay paths found none a player can reach with the shipped MM7 data. The one MM7 crash on record, a bounds
   abort in party creation with the Dragoon class on 2026-11-21, was fixed in `264d8d800`.

The verdict drops back to **NO-GO** when a source change lands without the suite and both chains rerun, or when a
player-reachable crash is found. The known defects below don't block a playthrough.

## Systems

| System | State | Evidence or gap |
| --- | --- | --- |
| Launch, New Game, party creation | GO | Installed helper `014514250`. Owner session S1, 2026-11-10. Dragoon creation crash fixed in `264d8d800`. |
| Save, load, quick save | GO | Atomic native writer (`mm7::writeAtomicSaveSlot` in `SaveLoad.cpp`), load preflight, `MM7SaveFailure.*`, 28 checkpoint pairs in `tools/run_mm7_pretag_checkpoints.py` |
| Main quest, Light and Dark | GO headless | `MM7CampaignChain.DISABLED_Chapter0`-`7` and the Dark chain, both endings. Passed on `617b94d23`. |
| Promotions | GO headless | All 27 promotion endpoints, see [MM7 Quest Behavior Catalog](MM7-Quest-Behavior-Catalog.md) |
| Maps | GO headless | All 76 maps load and run, see [Campaign Status](Campaign-Status.md) |
| Combat and monsters | GO | [MM7 Combat Audit Fixes](MM7-Combat-Audit-Fixes.md), [MM7 Gameplay Harness](MM7-Gameplay-Harness.md). Known non-blocking defects below. |
| Houses, shops, guilds, NPCs | GO | Upstream coverage plus the fork's quest and vendor tests |
| Fortune's Turn, chest reroll, All Artifacts, vendor refresh, Dragoon | GO headless | Covered by the full suite on `617b94d23` |
| Visible play on the installed app | Partial | Owner session S1 on 2026-11-10. The graphical audit rows (interface, media, mods, long sessions) are open. |

## Known defects that don't block

From the 2026-11-07 audit, still open in source on `617b94d23`:
- A wand charge is spent before the spell resolves (`Character.cpp`).
- Monster Haste and Day of Protection are written but never read (`Actor.cpp`).
- The Hour of Power hit check reads the Shield buff's power (`Character.cpp`).
- `_SelectTarget`'s index never advances, and the indoor summon floor check is unchanged (`Actor.cpp`).
- The Assassins' and Barbarians' enchantments add no elemental damage (`Item.cpp`).
- Artifacts 530, 531 and 536 lack their Sleep, elf-slaying and drowning effects.
- Two grandmaster teacher requirements differ from their text (`NPCTopics.cpp`). The Archer and Warrior Mage Disarm
  Trap caps are unresolved against MM7.exe.

## Unknown risk

- Upstream issues #2068 (sporadic inventory click crash), #1560 (Haunted Mansion entry) and #2276 (Temple of the Moon)
  are open upstream and have no fork test or fix.
- With modified or GrayFace-patched event data, `Processor.cpp`'s painting fix and the `ItemTable.cpp` treasure level
  assert can be reached. Shipped MM7 data doesn't reach either.

See also: [Campaign Status](Campaign-Status.md), [Release Milestones](Release-Milestones.md), [MM7 Combat Audit Fixes](MM7-Combat-Audit-Fixes.md), [Open Questions](Open-Questions.md)
