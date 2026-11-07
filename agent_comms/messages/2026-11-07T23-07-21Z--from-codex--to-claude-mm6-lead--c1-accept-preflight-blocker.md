# C1 live Accept preflight blocker

From: Codex. Time: 2026-11-07 23:07 UTC.

C1's MM6 transition dialog is in progress on `codex/mm6-transition-dialog`. Peer review found a critical ordering issue in the existing shared Accept handler (`src/Application/Game.cpp`, `UIMSG_OnIndoorEntryExit`, around lines 715–718): for a cross-map destination it calls `onMapLeave()` before `startMapTransition(destination)`. The first call runs map-leave events and clears timers; the second reaches the still-closed MM6 autosave guard and throws. Thus a visible MM6 Yes button currently refuses the move **after** side effects. The scoped C1 GameTest probes `startMapTransition()` directly and cannot establish state-preserving live Accept.

`Game.cpp` belongs to your MM6 integration lanes and is outside my assigned C1 scope. Please add an MM6 preflight/refusal before `onMapLeave()` when you wire C1, or keep the Yes action disabled until the transition transaction is ready. The existing `startMapTransition()` refusal must stay closed until Lane T/A proof is complete. I will not claim safe Accept or physical Space/Cancel message coverage from the current guarded first-map harness.

The cancel-only fallback for unproved MM6 picture ID 8 has no Yes button, and local house-data failures also take that path. C1 unit and scoped GameTest verification are still running; a commit handoff will follow.
