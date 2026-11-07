# Upstream relationship

Enroth Continuum is a source fork of [OpenEnroth/OpenEnroth](https://github.com/OpenEnroth/OpenEnroth).
OpenEnroth is the historical upstream and an optional source of reviewed changes,
not a required runtime or final MM7 build source. The SklerosAI repository is the
authoritative home for this fork's MM7, MM6 and eventual MM8 implementations.

## Working agreement

1. Preserve the upstream `LICENSE`, Git history, notices, and contributor credit.
2. Do not rewrite or obscure upstream commits.
3. Keep SklerosAI changes small, documented, and separated from ordinary upstream
   updates where practical.
4. Offer broadly useful, game-agnostic fixes back to OpenEnroth when appropriate.
5. Keep all proprietary Might & Magic assets out of Git and out of public releases.

## Remotes

The local checkout should use these names:

```text
origin    https://github.com/SklerosAI/Enroth-Continuum.git
upstream  https://github.com/OpenEnroth/OpenEnroth.git
```

To inspect incoming work without changing local history:

```bash
git fetch upstream
git log --oneline master..upstream/master
```

Upstream synchronization is intentionally a reviewed, optional operation. Do not
force-push the shared branch, and do not mix synchronization commits with
SklerosAI work. The Mac MM7 self-contained-source gate in
[MM7_SELF_CONTAINED_SOURCE.md](MM7_SELF_CONTAINED_SOURCE.md) removes remaining
build-time dependencies on OpenEnroth-hosted artifacts while retaining all
required attribution.
