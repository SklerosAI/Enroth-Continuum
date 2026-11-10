# MM7 actor- and character-target scroll cancel handoff

- Owner: Claude Code lane subagent, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm7-actor-scroll-cancel`, commits `9ed8a074a` (fix and two tests) and `1fba84a61` (inventory-path test), stacked on `401d8e2a4`, which `master` already carries as `7b3b7b074`. Cherry-pick the two commits. Not pushed or merged.
- Files: `src/Application/Game.cpp`, `src/Engine/Objects/Character.cpp`, `src/Engine/Spells/CastSpellInfo.{h,cpp}`, `test/Bin/GameTest/GameTests_MM7Scrolls.cpp`.

## Behavior

A throwaway test first confirmed that Escape at the targeting cursor lost a Fire Bolt (actor target) or Cure Weakness (character target) scroll. Right-click does not cancel targeting. The scroll now stays on the cursor until the `UIMSG_SpellScrollUse` handler queues the spell. That message now carries the scroll's item ID instead of the spell ID, and the handler takes the scroll only if it is still on the cursor. `pushScrollSpell` keeps the scroll with any spell that opens a targeting window, and `Game::closeTargetedSpellWindow` gives it back when targeting closes before a target is picked. The Escape branch in `SCREEN_GAME` now calls `closeTargetedSpellWindow` instead of an inline copy, and the item-target refund from `401d8e2a4` moved there. `cancelSpellCastInProgress` still never refunds, because it also runs on Load. MM6 is unchanged.

Side effect, reasoned and not tested: reading a scroll during the turn-based movement stage, or losing the queued message, now leaves the scroll on the cursor instead of using it up.

## Checks

- `MM7Scrolls`: 4/4 (item target, actor target, character target, actor target read from the open inventory).
- `MM7Scrolls` plus Issue506, 2018a, 2018b, 2021, 2022, 2451a, 2451b, 2789, 2941, 2945, 2950: 15/15.
- Revert-and-fail: with the source change reverted, every cancel iteration fails with the scroll gone. Completed casts pass either way.
- Full headless MM7 GameTest binary on `9ed8a074a`: 530 passed, 2 skipped (MM7Vendor mod-flag tests), 0 failed. Unit tests 1087 passed, 45 skipped. The later commit only adds a test.
- cpplint clean on touched files. Full `check_style` on this branch only fails on the pre-existing errors that `claude/master-style-fix` removes.

## Lead review notes

- No other sender of `UIMSG_SpellScrollUse` exists. `pushScrollSpell` lost its default argument, and both callers pass a scroll.
- Closing the character screen does not move the held item, which the inventory-path test now pins.

## Remaining

Attacking, passing the turn, stealing, the caster becoming unable to act, a full spell queue, or turn-based wait still end targeting through `cancelSpellCastInProgress` and use the scroll up. Stage 3 audit queue.

Privacy: no game data, saves or logs in Git.
