---
type: concept
up: "[[Campaign-Status]]"
related: ["[[MM7-Atomic-Save-File]]", "[[MM7-Private-Portrait-Mod]]"]
---

# MM7 Dragoon Mod

Atlas exposes an MM7-only, default-off Dragoon class family using the original portraits, voices and paperdolls.

The implemented family is Dragoon (36), Lancer (37), Dragon Knight (38), Dread Lancer (39). Original class IDs and their data remain intact. The current user specification supersedes the broader [custom-class plan](../docs/plans/mm7-dragoon-alchemist-classes.md): only Dragoon is implemented, mandatory skills are Spear/Air, Air reaches GM, Shield reaches Master, HP advances 4/6/8 and SP 2/3/5 using Intelligence. These rates supersede the initial 5/7/9 HP and 2/4/6 SP proposal, placing each tier one growth point below Knight HP and Sorcerer SP. The supplied icon ships with Atlas and is staged into the private mod folder on enabled launches.

The November 21 starting-skill correction supersedes the initial Archer-choice rule. The nine optional choices are Bow, Leather, Water, Repair, Meditation, Perception, Disarm Trap, Armsmaster and Learning; each is trainable by Dragoon. Fire is excluded. The family retains at least a five-SP pool even at low Intelligence, preventing the default Goblin's negative stat modifier from erasing its starting SP. Existing saves retain their current spent SP and can refill by normal rest/recovery.

The Knight promotion adapter operates only in global events 71–76, including topic eligibility and class changes. Other class comparisons retain exact equality. Final promotions add independently resisted 10 Air and 10 Water damage to successful melee hits. There is no Aegis or other final ability. Blaster ceilings derive from Knight, while creation does not grant Blaster.

Custom classes are structurally valid save data regardless of the toggle. Loading them with the toggle off refuses before party reconstruction, leaving the selected primary intact instead of choosing an older backup. Main-menu loading handles refusal before preparing a world. Existing original-class saves remain usable with the mod off. The opt-in is only the exact `ENROTH_MM7_DRAGOON_CLASS=1`; no persistent marker overrides Off.

## Headless checks

`scripts/run_mm7_dragoon_tests.py --data-path <private-MM7-data>` runs the non-MM6 test set with mods off, the MM7 test set with Dragoon on, and the dedicated existing-mod coexistence suites with their flags on. Tests use the harness's RAM filesystem and do not use owner saves. Related checks are the launcher Swift contract tests and Atlas page tests.

New game tests cover creation draws and empty skill slots, starting the actual creation UI party and save/reload, tiered masteries and vitals, no-Knight Cavalier/Champion/Black Knight quests, melee packets with independent immunities, exact generic class comparisons, and refused loads from the game and main menu. Existing private-image tests cover the mod icon loader. A review also found an order-dependent older award-animation fixture; its initial animation state is now explicit.

Graphical play acceptance is delegated to the owner. Headless results do not establish a completed Dragoon campaign.

On 2026-11-21, the complete matrix finished with no failures: 729 passed / 20 opt-in skips with Dragoon off; 299 passed / 13 other-mod skips with Dragoon on; 21 passed / one off-only skip in mod coexistence. Swift launcher contracts, Atlas page checks and changed-source lint also passed. The installed normal Atlas bundle preserves the existing MM6/upstream helpers and media notices, and Dragoon is explicitly set off. A recoverable pre-update app copy is retained locally outside Git.

The 2026-11-22 cleanup preserves the previously uncommitted implementation in source history and adds descriptions for all four custom classes in right-click help. The [app replacement policy](Agent-Handoff.md#app-replacement-policy--2026-11-21) supersedes the earlier retained-app-copy practice.

See also: [Campaign Status](Campaign-Status.md), [Atomic Save File](MM7-Atomic-Save-File.md), [Private Portrait Mod](MM7-Private-Portrait-Mod.md).
