# MM7 Lincoln face-pick control handoff

- Branch: `codex/mm7-lincoln-pick-control` in reused `mm7-load-admission`.
- Code commit: `87647d8bd`. No runtime source changed.
- Scope: the Lincoln interaction helper now returns both whether an input picked the requested face and whether the expected effect occurred. It checks the same keyboard selection filters used by `Game::onPressSpace` and checks the mouse pick after aiming at a face.
- The unpowered pedestal and unsuited exit negatives now require a confirmed pick with no effect. Console activation, powered retrieval and suited exit require both a pick and the expected effect. This closes W4 review finding F2's false-pass route.
- Evidence: the focused `MM7OverthrusterRoute.LincolnRetrievalAndExit` passed 1/1 headlessly. Changed-file cpplint and `git diff --check` passed.
- Integration: cherry-pick `87647d8bd`, rerun the focused Lincoln route and the exact disposable Overthruster writer/reader checkpoint on integration source. The helper's route search still uses the same positions and input attempts.

No private data, saves, logs or graphical windows were used.
