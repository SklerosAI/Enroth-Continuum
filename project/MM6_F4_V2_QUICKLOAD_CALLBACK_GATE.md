# MM6 v2 fresh-child QuickLoad callback gate

The real Main Menu QuickLoad edge already has test-only switches:
`setMainMenuAutosaveQuickLoadOverride(true)` selects `autosave.ec6`, and
`setLoadGameOverride()` replaces only the MM6 load body. `Game::gameLoop()`
calls the void load override, then immediately loads portraits and prepares
the world. Returning from a callback that has only staged detached owners
would therefore enter gameplay without publishing them. Throwing from the
real edge aborts the process, so this gate does not queue QuickLoad yet.

The disabled `MM6ZCreationItemSeedHeadless.V2QuickLoadCallbackRejectsBeforePublish`
test invokes the **same** test-only `loadGame()` callback inside a caught game
routine. It requires an existing native v2 `autosave.ec6` under a 0700
`mm6-restart-*` directory directly below the system temp root. The callback
uses fresh MM6 NPC tables and the loaded map source to stage the exact
primary bytes through `preflightStagedRawSessionFields()`, checks the first
outdoor plan, then calls the typed child-publication inspection. It refuses
to return to `gameLoop()` while that admission reports unresolved owners.
The test checks that the selected slot bytes and sampled Party, raw
cursor/shop, chest and map-cache owners remain unchanged. Clearing the test
override restores the ordinary MM6 `loadGame()` guard.

Run this test alone in a fresh null-renderer process after the existing
disabled real-starter v2 writer has populated the same disposable profile.
The earlier read-only v2 stage test may run as another separate process.
The runner must pass explicit MM6 data and private rules paths, monitor that
the game creates no on-screen window or focus change, and remove the
temporary profile after all child processes finish. The test performs no
publication and does not touch the installed MM7 or MM6 user profile.

With `MM6_GAMETEST`, `MM6_PRIVATE_DATA`, `MM6_PRIVATE_RULES` and
`MM6_TESTDATA` set to local paths, this runner creates and removes the only
disk destination. Run it under the headless host window/focus monitor:

```sh
python3 - <<'PY'
import os
import subprocess
import tempfile

tests = (
    'MM6ZCreationItemSeedHeadless.DISABLED_WriteRealStarterV2RestartSlot',
    'MM6ZSaveHeadless.DISABLED_RawV2RestartSlotStagesDetachedSession',
    'MM6ZCreationItemSeedHeadless.DISABLED_V2QuickLoadCallbackRejectsBeforePublish',
)
with tempfile.TemporaryDirectory(prefix='mm6-restart-') as profile:
    env = os.environ.copy()
    env['ENROTH_MM6_RESTART_PROFILE'] = profile
    for name in tests:
        subprocess.run((
            env['MM6_GAMETEST'], '--game', 'mm6', '--headless',
            '--data-path', env['MM6_PRIVATE_DATA'],
            '--rules-path', env['MM6_PRIVATE_RULES'],
            '--test-path', env['MM6_TESTDATA'],
            '--gtest_also_run_disabled_tests', '--gtest_filter=' + name,
        ), env=env, check=True)
PY
```

In the isolated linked check, all three children passed **1/1**. The host
monitor sampled 26, 11 and 12 times respectively, reporting zero on-screen
game windows and no focus change. The callback re-encoded the selected v2
primary to the exact source bytes, received unresolved resource/world
blockers, rejected before assignment, restored the normal MM6 load guard and
left the slot bytes unchanged. This is a callback-stage proof, not a
Main Menu QuickLoad or live reload proof.

The actual QuickLoad event and `Game::loop()` transition may be enabled in
this disposable test only after a complete test-only publication coordinator
can assign Party/HELD/SHOP/SCLK, NPC/timer/overlays, persistent events,
map/chest/world objects and UI/resource identity as one validated handoff.
Until then production SaveLoad, the menu and Atlas remain guarded.
