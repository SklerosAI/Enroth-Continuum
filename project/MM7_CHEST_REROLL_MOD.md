# MM7 chest reroll mod

This private, opt-in MM7 mod adds a small **Reroll** button to a chest that
contains authored random-loot markers. Enable **Use private mods** and
**Fortune’s Turn · MM7** in The Atlas, then launch MM7 again. Vanilla MM7,
MM6 and upstream OpenEnroth do not receive the launch flag.
The in-game X controls non-chest rewards and does not disable this button.
The brass and dark-wood button sits in the chest's lower wooden border.

Each click draws a fresh seed from the gameplay RNG and rolls the chest's
unclaimed random slots through the existing MM7 chest generator. The authored
random marker and current map treasure level determine the same possible
treasure-level range as the first generation. The chest type, flags, fixed
items, traps and map state are retained. If an artifact from the discarded
roll was never claimed, its vanilla generated flag is released before the new
roll. The All Artifacts + Storage mod continues to use its collected ledger.

The source markers are kept as hidden chest records until the player takes or
deposits an item. At that point the markers are removed, and the button is no
longer available for that chest. This prevents rerolling after collecting loot.
The hidden markers and their generated-item links survive the native MM7 chest
save format without changing its record size. A save that already contains
generated chest loot from before this option was enabled has no source-marker
provenance, so that chest cannot be rerolled until its normal map respawn.

The button is absent when the toggle is off, no source marker remains, or the
chest belongs to MM6. MM6 has its own chest implementation. This chest route
changes no monster loot; the rest of Fortune’s Turn follows its own reward paths. The mod
changes only local source and preferences; it does not edit GOG archives.

Focused validation: exact MM7-only flag parsing, Atlas environment isolation,
the Atlas toggle, a chest snapshot round trip, retained fixed item and
source treasure marker, repeated rerolls, and reroll cutoff after an item is
taken. The inset button's click was checked headlessly, and its placement was
reviewed in an offscreen MM7 capture. Visible installed-app interaction remains
for the next play session.
