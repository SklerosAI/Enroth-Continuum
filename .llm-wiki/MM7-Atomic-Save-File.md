---
type: synthesis
up: "[[MM7-Disposable-Save-Reload]]"
related: ["[[MM7-Self-Contained-Source]]", "[[Agent-Handoff]]"]
tags: [mm7, save, durability, mac]
---

# MM7 Atomic Save File

An isolated MM7 native-slot helper stages complete bytes beside the destination and retains one valid previous primary as a backup.

The [source](../src/Engine/MM7/MM7AtomicSaveFile.cpp) and [focused tests](../src/Engine/MM7/Tests/MM7AtomicSaveFile_ut.cpp) are integrated in the main tree. The helper requires a caller-supplied full-content validator; its own checks reject unsafe slot names, symlinked save paths, oversized files, invalid existing primary or backup files, and backup-only overwrite. Read-only preflight can return a valid backup without repairing the slot. Writes sync temporary file contents, sync the newly created `saves` directory entry through the profile root, rotate the validated primary to `.bak`, replace the primary, and sync the saves directory. A failure after primary rename remains an uncertain result.

Seven synthetic, isolated test bodies passed under network denial, including overwrite rotation, read-only backup selection, corrupt candidates, unsafe paths, and faults before and after replacement. The full engine target and real MM7 save format were not exercised by those tests. The normal [MM7 SaveLoad](MM7-Disposable-Save-Reload.md) and menu paths still use the inherited writer; Claude's active MM6 save lane owns the shared files needed for production wiring. The helper does not yet make live saves atomic or prevent the existing corrupt-body load crash. Full semantic validation, the trusted native profile root, capture-side effects, visible save/reload, and a restore decision remain open.

A later Codex branch candidate adds [bounded structural preflight](../src/Engine/MM7/MM7SaveValidation.cpp) for native-save framing, decompression, map deltas and PCX payloads, plus selected inventory, sprite, actor and door references. It bounds Party alignment, arena state, character class, sex, portrait/voice, spellbook page and quick spell, active beacon map indices, and selected actor monster/name/hostility IDs. Thirteen focused tests, a disposable fresh-save writer and reader, and an earlier 25-test MM7 GameTest batch passed under network denial; four PCX decoder units passed. The candidate remains detached.

Fresh saves can contain stale item IDs in unreferenced chest records. The shared [chest reconstruction guard](../src/Engine/Snapshots/EntitySnapshots.cpp) now drops invalid raw IDs while preserving valid random-loot markers, and checks oversized active grid references before indexing an item record. A synthetic MM7 chest unit, the MM6 first-map chest route and a fresh disposable MM7 writer/reader passed. The detached preflight still omits other semantic owners, archive identity and legacy-save compatibility. It is not ready to serve as the atomic helper's production validator. The shared `SaveLoad.cpp` and `UISaveLoad.cpp` ownership remains with Claude's MM6 lane until its handoff.

See also: [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md), [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [MM6 Transition and Death Candidates](MM6-Transition-And-Death-Candidates.md), [Agent Handoff](Agent-Handoff.md).
