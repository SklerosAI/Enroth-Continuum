# Claude MM6 outdoor actor alpha lane, 2026-11-18

Branch `claude/mm6-outdoor-actor-alpha`, off master `23f6eb0c9`. Reported in the 2026-11-17 owner playtest and tour
(`local-game-data/mm6/tour-2026-11-17/`): every actor on MM6 outdoor maps drew washed out and see-through, while
indoor actors drew fine.

## Commits

- `25dc4dc6a` Draw MM6 outdoor actors opaque. MM6 `dmonlist.bin` records have no tint field, and their reconstruct
  filled one with opaque white. Outdoors `BaseRenderer::TransformBillboard` takes a tint with a non-zero alpha byte as
  the original's additive blend mode (`Opaque_3`, `GL_ONE, GL_ONE`), so every MM6 outdoor actor was added onto the
  scene instead of alpha blended. Indoors the same check reads the dimming level, which is why dungeons looked right.
  MM6 monsters now get no tint, the value MM7 uses for its untinted monsters. `MM6StreetNpcs.VillagersDrawOpaque`
  tapes the blend modes of the actor and decoration billboards drawn down the street from Rosemary the Guard.

## Verified against the data

- MM6 `icons.lod/dmonlist.bin`: 173 records of 148 bytes, no tint field (decoded with a throwaway Python LOD reader).
- MM7 `Events.lod/dmonlist.bin`: 277 records, 211 with tint 0, the rest an RGB tint whose alpha byte is 0 in every
  record, so MM7 never took the additive path through monster tint. MM8 was not scanned, its path is untouched.
- Runtime before the fix, from a throwaway offscreen probe on oute3: all 15 drawn actor billboards had opacity
  `Opaque_3` and tint `ffffffff`, all drawn decoration billboards had `Transparent` and tint 0. After the fix all
  actor billboards are `Transparent` with tint 0. Decorations and items never went through the monster tint, so they
  were never affected.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/outdoor-actor-alpha/before.png` and `after.png`, New Sorpigal street
looking north at Rosemary the Guard. Pixel diff: 2799 pixels differ, 2522 inside the guard's box and the rest on the
two distant villagers, nothing else in the frame moved. Mean RGB sum in the guard's box went from 313 to 249. The MM7
Emerald Island first-outdoor capture is pixel-identical before and after (0 differing pixels).

## Test results

- `MM6StreetNpcs.VillagersDrawOpaque` failed on master (actor tape held `Opaque_3`) with the same test code, and
  passes with the fix.
- All MM6 game tests (`MM6*`, headless): 258 ran, 256 passed, 2 skipped (private-fixture tests), 0 failed. 126
  disabled tests are the offscreen capture tests.
- `OpenEnroth_UnitTest`: 1170 ran, 1125 passed, 45 skipped (optional fixtures not supplied), 0 failed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840`: 3/3 passed.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 is not installed locally.

The shared file `src/Engine/Snapshots/EntitySnapshots.cpp` changed, but only the MM6 `MonsterDesc_MM6` reconstruct,
which MM7 and MM8 never call.
