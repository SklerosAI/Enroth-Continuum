---
type: analysis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, conditions]
---

# MM6 F4 Condition Variables

The [condition packet](../project/MM6_F4_CONDITION_VARIABLES.md) records a bounded MM6-only executor for raw EVT IDs 87–104. The F3-matched private event ledger has 54 uses across 15 IDs: 18 have bounded execution and 36 active writes remain guarded. Exact script locations and values remain in ignored `local-game-data/`.

The seventeen ailment IDs map to character condition clocks. Good at ID 104 skips the inherited MM7-only Zombie slot and clears the selected character's conditions. The executor handles observed Cursed/Fear comparisons, Cursed/Fear/Drunk application, Fear clearing and Good clearing through the existing event character selector. Weak, Insane, Poison, Disease, Dead, Stoned and Eradicated writes remain guarded because current MM7 blocking or HP/SP effects are unproved for MM6. MM7 keeps its path.

The title-specific `.ec6` `EFFT` section already copy-stages four characters' condition clocks. Live SaveLoad remains closed. MM6-specific blocking, health effects, audiovisual cues and Good recovery still need original-game comparison and F5 play/reload checks.
