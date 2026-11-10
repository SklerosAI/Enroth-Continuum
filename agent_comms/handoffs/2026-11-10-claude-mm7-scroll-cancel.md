# MM7 cancelled item-target scroll handoff

- Owner: Claude Code lane subagent, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm7-scroll-cancel` from `master` `ff78a422a`, in the `mm7-save-durability` checkout. `codex/mm7-multitarget` keeps its ref.
- Commit: `401d8e2a4` Keep an item-target scroll when its item pick is cancelled. Not pushed or merged.
- Files: `src/Application/Game.cpp` (casting-screen escape path), `src/Engine/Objects/Character.cpp` (`useItem` scroll branch), `src/Engine/Spells/CastSpellInfo.{h,cpp}` (new `CastSpellInfo::scroll`, `pushScrollSpell` takes the scroll), `test/Bin/GameTest/GameTests_MM7Scrolls.cpp` (new), `test/Bin/GameTest/CMakeLists.txt`.

## Behavior

A throwaway test confirmed the defect before the fix. Reading a Fire Aura scroll onto a portrait and then pressing Escape or the Cancel button left zero scrolls on the cursor and in every inventory. The queued spell now carries the scroll while the item pick is pending, and escaping from the casting inventory puts it back on the cursor. A completed pick still uses exactly one scroll. The refund is not in `cancelSpellCastInProgress`, because that also runs on Load and would duplicate the scroll into the loaded party. MM6 scrolls stop at their guard before this path. The original game's behavior was not observed.

## Checks

- `MM7Scrolls.CancelItemTargetScroll`, a cancel and complete loop with scroll count, held item, screen and sword enchantment tapes: 1/1.
- Revert-and-fail: with the source change reverted, the cancel iteration fails with the scroll gone. The complete iteration passes either way.
- Existing scroll and enchant tests (Issue506, 2018a, 2018b, 2021, 2022, 2451a, 2451b, 2789, 2941, 2945, 2950): 11/11.
- `Run_UnitTest`: 1087 passed, 45 skipped, 0 failed.
- Full headless MM7 GameTest binary, one process: 528 passed, 2 skipped, 0 failed, 7 disabled not run.
- Style targets for the touched directories pass. Full `check_style` fails on files this commit does not touch, already on `master`: `src/Engine/Mods/PrivateMods.cpp` and `src/Engine/Objects/Chest.cpp` missing includes, and a using-directive in `src/Engine/Mods/PrivateMods_ut.cpp`. `check_tidy` was not run because clang-tidy is not installed here.

## Lead review notes

- `Party::setHoldingItem` moves any held item into inventory first, so the refund cannot overwrite another item.
- Only Escape is tested. The casting inventory's Exit button goes through `OnCancel2`, which the throwaway run showed loses the scroll on old code. Its fixed path is reasoned, not tested.
- The test clicks the paperdoll sword by raw coordinates. Acceptable, no paperdoll helper exists.

## Remaining

- Actor- and character-target scrolls (Fire Bolt and similar) still lose the scroll when targeting is cancelled. The TODO now sits on that branch. Added to the stage 1 defect queue as non-blocking.
- A cast refused during turn-based wait or movement stages still loses an item-target scroll. Existing behavior, untested.
- The `master` style failures above need their own small fix.

Privacy: no game data, saves or logs in Git. Test logs stayed in Claude's temporary scratch directory.
