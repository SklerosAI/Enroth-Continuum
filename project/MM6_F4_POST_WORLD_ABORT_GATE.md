# MM6 disposable post-world abort gate

The one-shot MM6 test hook now has an explicit `AbortToMainMenu` result. On an
armed load, `Game::gameLoop` receives that result immediately after
`DoPrepareWorld(true)` and returns with `GAME_STATE_GAME_QUITTING_TO_MAIN_MENU`
before timer unpause, `ENGINE_SKIP_NEXT_USER_INPUT`, and `MessageLoopWithWait`.
An unarmed MM6 load returns `Unarmed`; MM7 does not call the hook. A callback
exception still terminates the GameTest control routine, so expected refusal
must use the abort result. The scope can be installed only for MM6, once per
game thread, and is consumed before invoking the callback.

`MM6PostWorldAbortHeadless.DISABLED_V1ControlQuickLoadReturnsBeforeInput` is
an isolated control for this edge. It requires an independently generated v1
`autosave.ec6` under an owner-only `mm6-restart-*` directory immediately below
the system temporary directory, then selects the real Main Menu QuickLoad
route through the existing test-only v1 stage. The hook checks the first map,
paused timer, actors, chests, cleared transition and empty action queue, asks
for abort, and the test verifies the slot bytes remain unchanged. Run it alone
in a fresh null-renderer child and end that process after its assertions.

The integrated control now also samples the complete pending-action observer
at that hook and requires the held-cursor restore plan to see idle spell, turn,
rest, GUI-message, movement and attack state. Separate disposable writer and
abort processes passed **1/1** each. This is a v1 control, not a v2 owner join.

This control does **not** publish a v2 session. The v2 callback test remains
read-only and refuses before live assignment. The detached v2 admission still
reports unresolved mouse image, NPC static resources, active world resources,
chest collection, rebuilt identity, post-world persistent events, beacon and
indoor face owners, event timers/triggers, and transient UI/action owners.
Only after a complete pre-world owner join can a v2 child be published and
rechecked at this hook. A passing v1 abort control is not evidence of a
playable MM6 QuickLoad or failure-atomic v2 commit.
