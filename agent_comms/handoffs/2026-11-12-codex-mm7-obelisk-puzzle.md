# MM7 Obelisk puzzle handoff — 2026-11-12

- Owner/lane: Codex, MM7 delivery lane. Integrated into neutral `master` at `66b7a1a8f`.
- `MM7SideQuests.ObeliskPuzzleRequiresAllRegionsAndMidnight` loads every regional map, discovers and executes each of the fourteen authored local Obelisk bindings, and verifies its corresponding quest bit.
- The route proves the Evenmorn treasure remains hidden when Stone City is missing and after all discoveries at 1 AM. At midnight it exposes the authored decoration, opens the real chest through `DecorationInteraction`, verifies the treasure bit and 100,000-gold script reward, then takes all chest gold and items through `Chest::GrabItem`.
- `MM7SideQuestCheckpoint.DISABLED_WriteObeliskPuzzle` writes a structurally valid normal MM7 `save000.mm7`; a separate `DISABLED_ReloadObeliskPuzzle` process uses the normal Load menu and verifies every discovery bit, treasure bit, gold and emptied chest while leaving the slot byte-identical.
- Verification: focused route 1/1; writer 1/1; fresh-process reader 1/1; regional `MM7SideQuests.*:MM7Promotions.*` selection 14/14; repository `check_style` 157/157; missing-profile and no-save admissions rejected. Disposable profiles were moved to Trash.
- Boundary: this is the eighth independent durable regional route, not continuous regional progression. All checks were headless; no graphical capture or installed-app acceptance ran.
- Resume: select the next uncovered high-value row from `.llm-wiki/MM7-Quest-Behavior-Catalog.md`, keep private data and evidence outside Git, and retain the separate-process Save/Load and 14-test regional gates.

