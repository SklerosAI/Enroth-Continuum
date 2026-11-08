# Codex — MM7 chest reconstruction guard

- Owner: Codex
- Branch/worktree: `codex/mm6-transition-dialog` at `/Users/tburton2/.codex/worktrees/mm7-save-atomic/Enroth-Continuum`
- Narrow file scope: `src/Engine/Snapshots/EntitySnapshots.cpp`, only `reconstruct(const Chest_MM7&, ChestInventory*, ...)` and a focused test seam if needed
- Purpose: skip stale, invalid raw chest item IDs before they become live inventory items; bound the invalid-grid warning index before reading `items[index]`
- Started: 2026-11-08
- Coordination: all registered worktrees were checked for working or committed changes to this file and none had any. Claude was informed of the shared-core issue in the 23:58 UTC message. MM6 chest reconstruction uses this path, so run MM6 chest and MM7 save controls before handoff. Do not touch SaveLoad/UI or other snapshot reconstruction functions.
- Privacy: use only synthetic test records and high-level disposable-save results; no private save bytes or item inventories in Git.
