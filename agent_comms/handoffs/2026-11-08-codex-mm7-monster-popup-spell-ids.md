# MM7 monster popup spell-ID bounds handoff

- Owner: Codex MM7 save reference audit subagent.
- Source commit: `6b8c6f2e7` on `codex/mm7-audit-integration`, rebased onto
  integration `895839946`. Files: `src/GUI/UI/UIPopup.cpp` and
  `test/Bin/GameTest/GameTests_MM7Combat.cpp`.
- Behavior: the monster popup reads regular spell names only for IDs 1..99.
  Other nonzero IDs use the existing localized unknown-value label. This is a
  display guard; it does not change actor state, save admission, or spell
  attack dispatch.
- Red evidence: with only the synthetic GameTest added, the exact MM7
  inactive spell-ID 255/254 popup case exited 134 at the unchecked
  `IndexedArray<SpellInfo, 1, 99>` access. The actor was Standing and both
  spell-use chances were zero.
- Green evidence: after the source fix and again after rebase, a network-denied
  throttled GameTest build passed. The MM7 invalid-ID and regular-ID popup
  cases passed 2/2; the MM6 synthetic actor popup control passed 1/1 in a
  separate network-denied process. Scoped cpplint and `git diff --check`
  passed.
- Limit: the tests exercise the popup's measurement path and verify regular
  spell names are loaded; they do not assert pixels or captured rendered text.
  The synthetic actor state is saved-like, not a serialized save round trip.
  Active invalid spell IDs still need a separate save/runtime admission rule.
- Privacy: no game data, saves, raw logs, or private paths were committed.
