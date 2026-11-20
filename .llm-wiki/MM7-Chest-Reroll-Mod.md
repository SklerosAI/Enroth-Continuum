---
type: synthesis
up: "[[MM7-Disposable-Save-Reload]]"
related: ["[[MM7-Quest-Behavior-Catalog]]"]
tags: [mm7, mods, chests, save]
---

# MM7 Chest Reroll Mod

The MM7-only private mod exposes a small Reroll button for unclaimed random
chest loot while its Atlas toggle and Private mods are on.

The [source contract](../project/MM7_CHEST_REROLL_MOD.md) defines the boundary.
The engine retains each authored random marker in a hidden chest record and
links its generated items to that marker through the native item's unused pad
byte. The marker's level and the map's current treasure level feed the same
range selector on each roll. Each click seeds a local gameplay RNG from one
draw of the normal stream. Fixed items and the chest's type, flags, trap and
map state remain. Taking or depositing an item removes the hidden markers and
ends rerolls for that chest, including after save/load.

The snapshot round trip and focused headless chest test passed with the toggle
on, including a rebuild after the final source edit (2/2). A combined run with
All Artifacts + Storage passed its focused game tests. The Atlas page and
Swift environment checks passed. The default full MM7 headless suite passed
733/733; the native unit target and `check_style` passed. The installed Atlas
bundle passed macOS signature verification and retained the previous MM6
executable unchanged. Its previous version is kept in an ignored local rollback
folder under `build/Atlas-MM7-Chest-Reroll/`.
Visible installed-app interaction remains to be checked in normal graphical
play; headless checks do not establish the button's appearance or mouse click.

An older save's already-generated chest lacks marker provenance, so this mod
does not offer a reroll for it until that map normally respawns. Monster loot,
MM6 and MM8 are outside this implementation. Private data, saves and mod files
remain local and untracked.

See also: [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md),
[Agent Handoff](Agent-Handoff.md), [MM7 Chest Reroll Contract](../project/MM7_CHEST_REROLL_MOD.md).
