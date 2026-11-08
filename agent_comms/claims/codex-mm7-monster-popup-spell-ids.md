# Codex — MM7 monster popup spell IDs

- Owner: Codex MM7 save reference audit subagent
- Branch/worktree: `codex/mm7-audit-integration` in `mm7-save-atomic`
- Files: `src/GUI/UI/UIPopup.cpp`, `test/Bin/GameTest/GameTests_MM7Combat.cpp`
- Scope: bound the monster popup's two saved spell-ID display lookups and add
  focused MM7 and MM6 controls. Actor attack behavior and save validation are
  outside this claim.
- Coordination: the earlier Codex maintenance claim records integrated portrait
  work in `UIPopup.cpp` and asks for coordination; the Codex parent assigned
  this subsequent isolated popup slice. The shared GameTest lane was released
  by the native-root gate before this claim.
