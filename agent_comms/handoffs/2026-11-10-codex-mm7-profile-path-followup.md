# MM7 disposable profile path follow-up

- Owner: Codex lane B, `codex/mm7-candidate-validation`.
- Code commit: `b7a536f64`.
- Files: `launcher/AtlasApp.swift`, `launcher/MM7ProfilePath.swift`, `launcher/assemble.py`, `launcher/tests/main.swift`.
- This supersedes the path-isolation implementation described in the earlier candidate handoff at `bf2569dca`; the earlier visible W3 evidence remains valid for that staged build.

Atlas now resolves the profile override to either a validated disposable path or no path. Only an unset override selects the ordinary `Profiles/MM7-Atlas` profile. An invalid set override cannot list saves, open saves, back up saves, or launch MM7 through Atlas. A Play action retains one validated profile URL for directory creation, the engine `--user-path` argument, and its post-exit log lookup, so a later override/marker change cannot redirect those operations to the ordinary profile.

Focused validation passed:

```text
xcrun swiftc launcher/MM7ProfilePath.swift launcher/tests/main.swift -o /tmp/enroth-mm7-profile-path-tests
/tmp/enroth-mm7-profile-path-tests
  MM7 profile path checks passed
xcrun swiftc -typecheck -framework AppKit -framework WebKit launcher/AtlasApp.swift launcher/MM7ProfilePath.swift
xcrun swiftc -O -framework AppKit -framework WebKit launcher/AtlasApp.swift launcher/MM7ProfilePath.swift -o /tmp/enroth-atlas-profile-validation
git diff --check
```

No game or Atlas window was opened for this follow-up. The installed Atlas, normal profile, and saved games were not touched. Rebuild the staged Atlas from this commit before any further visible candidate check or installed-app cutover. The remaining visible W3 gaps and normal-profile side effect are recorded in `2026-11-10-codex-mm7-candidate-validation.md`.
