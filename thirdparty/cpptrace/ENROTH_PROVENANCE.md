# Repo-owned cpptrace source snapshot

This is the exact tracked source tree of [jeremy-rifkin/cpptrace](https://github.com/jeremy-rifkin/cpptrace) at commit `3db8da80111171c219ab5839905771386bee06b3`, the gitlink previously recorded by Enroth Continuum. On 2026-11-07, all 209 original tracked blobs were copied from that commit and verified against their Git blob IDs without source edits.

The original full MIT `LICENSE` is retained. This provenance note is Enroth Continuum metadata, not part of the original snapshot.

This snapshot does **not** make the Mac build offline. The current cpptrace CMake route fetches `jeremy-rifkin/libdwarf-lite` at pinned commit `5dfb2cd2aacf2bf473e5bfea79e41289f88b3a5f` when `CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF` is enabled, as in the existing Mac build. That dependency has its own LGPL terms and remains unowned. The `EC_LOCAL_ONLY_CONFIGURE` gate must continue to reject a clean offline build until that dependency and the other missing inputs and recipes are handled.
