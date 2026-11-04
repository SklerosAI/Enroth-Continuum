# Local game-data vault

This directory is intentionally retained in Git only as a signpost. Everything else
inside `local-game-data/` is ignored by Git.

Place lawfully obtained, local-only installations or extracted game data here:

```text
local-game-data/
├── README.md                 # tracked; this file
├── mm6/                      # ignored
├── mm7/                      # ignored
└── mm8/                      # ignored
```

Do not put any proprietary game data, installers, archives, music, art, video,
saves, screenshots, or generated asset dumps into a commit. The ignore rule protects
normal Git use, but `git add -f` can override it—do not use that override here.

These folders are only local references for future tooling. They are not a promise
that all three games are currently supported by OpenEnroth.
