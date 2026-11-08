# Codex MM7 native profile-root context

- Owner: Codex MM7 native-root lane
- Branch/worktree: `codex/mm7-combat-fixes` in the reused combat worktree, based on integration `1af5f586d`
- Owned files: `src/Engine/MM7/MM7AtomicSaveFile.h`, `.cpp`, `src/Engine/MM7/Tests/MM7AtomicSaveFile_ut.cpp`, `src/Application/Startup/GameStarter.cpp`, and a focused existing startup GameTest file if needed
- Scope: MM7-only startup handoff of the resolved native user path, a read-only optional root for future atomic SaveLoad wiring, RAM/non-MM7 clearing, and root trust checks. No SaveLoad/UI wiring or production atomic-save claim.
- Test coordination: unit tests first; GameTests serial with the MM7 hired-profession validator lane.
