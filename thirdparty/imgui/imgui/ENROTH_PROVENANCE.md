# Repo-owned Dear ImGui source snapshot

This is the pinned [ocornut/imgui](https://github.com/ocornut/imgui) source at commit `b48d1afbe8ee8b238e2961dc363a949dd7304e23`, previously a Git submodule of Enroth Continuum. The original 281 tracked files are retained without source edits. The full upstream `LICENSE.txt`, source notices and bundled font credits/licenses in `docs/FONTS.md` remain here. This file is Enroth Continuum provenance metadata, not part of the upstream snapshot.

The current Mac MM7 target uses this source through the tracked ImGui integration wrapper. Owning it removes its submodule fetch, but does not resolve the other MM7 build inputs or make a clean offline build possible.
