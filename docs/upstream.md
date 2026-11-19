# Upstream relationship

This page covers how Enroth Continuum relates to OpenEnroth, who gets credit for what, and how upstream changes come in.

## Relationship

Enroth Continuum is a source fork of [OpenEnroth/OpenEnroth](https://github.com/OpenEnroth/OpenEnroth), the
open-source reimplementation of the Might & Magic VI-VIII engine. OpenEnroth is the historical upstream and an
optional source of reviewed changes, not a required runtime or final MM7 build source. The SklerosAI repository,
[SklerosAI/Enroth-Continuum](https://github.com/SklerosAI/Enroth-Continuum), is the authoritative home for this
fork's MM7, MM6 and eventual MM8 implementations.

OpenEnroth is the technical foundation of this project. The engine, its Git history, its license and its authorship
come from OpenEnroth. Enroth Continuum adds the Atlas Mac launcher and title-specific work for MM6, MM7 and MM8.
SklerosAI does not claim to be the original author of the engine or of the Might & Magic games.

## Working agreement

1. Preserve the upstream `LICENSE`, Git history, notices, and contributor credit.
2. Do not rewrite or obscure upstream commits.
3. Keep SklerosAI changes small, documented, and separated from ordinary upstream updates where practical.
4. Offer broadly useful, game-agnostic fixes back to OpenEnroth when appropriate.
5. Keep all proprietary Might & Magic assets out of Git and out of public releases.

## Attribution

| Party | What they own or are credited for |
| --- | --- |
| OpenEnroth authors and contributors | The engine. They retain credit for their work. The fork keeps upstream Git history so authorship stays visible and traceable. |
| OpenEnroth license | LGPL-3.0-or-later. The unmodified upstream `LICENSE` file stays at the repository root. |
| Original game rights holders | Might & Magic VI, VII and VIII and their original game data. None of it is part of this repository. Players supply their own lawfully obtained installations. |
| SklerosAI | Fork-specific changes. They identify themselves in commit messages, release notes, and relevant source headers or documentation. |
| University of Notre Dame CRC | The [llm-wiki-memory-template](https://github.com/crcresearch/llm-wiki-memory-template) behind the project's earlier knowledge base. |

When a change is broadly useful to OpenEnroth, prefer an upstream contribution, or keep the fork delta small
enough to make that contribution practical.

## Remotes

The local checkout uses these names:

```text
origin    https://github.com/SklerosAI/Enroth-Continuum.git
upstream  https://github.com/OpenEnroth/OpenEnroth.git
```

To inspect incoming work without changing local history:

```bash
git fetch upstream
git log --oneline master..upstream/master
```

Upstream synchronization is a reviewed, optional operation. Do not force-push the shared branch, and do not mix
synchronization commits with SklerosAI work.

## Building without upstream artifacts

The Mac MM7 self-contained-source build removes the remaining build-time dependencies on OpenEnroth-hosted binaries
and dependency archives while keeping all required attribution. Its command is `tools/build_mm7_macos.py`. The
[build and dev setup page](build-and-dev-setup.md) describes it.

## Notices in the app bundle

The Atlas assembler copies `LICENSE`, `CREDITS.md` and this page into the app's `Contents/Resources/Notices/`
folder. When the MM7 media libraries are built from source, their dependency license notices go into
`Contents/Resources/Notices/MM7-Media/`, and the assembler refuses a notice set that differs from the build report.

The Atlas can also carry an unmodified upstream `OpenEnroth.app` as a separate helper, so the owner can playtest
current upstream with their own MM7 data. `tools/openenroth_upstream_refresh.sh` rebuilds it from `upstream/master`
on its own branch, with no fork changes mixed in.

## Source

- [LICENSE](../LICENSE)
- [CREDITS.md](../CREDITS.md)
- [launcher/assemble.py](../launcher/assemble.py)
- [tools/build_mm7_macos.py](../tools/build_mm7_macos.py)
- [tools/openenroth_upstream_refresh.sh](../tools/openenroth_upstream_refresh.sh)
- [tools/OPENENROTH_UPSTREAM_REFRESH.md](../tools/OPENENROTH_UPSTREAM_REFRESH.md)

See also: [Project overview](project-overview.md), [Source and privacy](source-and-privacy.md), [Build and dev setup](build-and-dev-setup.md), [Releases](releases.md)
