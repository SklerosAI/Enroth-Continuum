# MM6 `.ec6` map-delta fields — F4 offline progress

Status: **bounded `MAPS` v1 section codec; live SaveLoad remains disabled** (2026-11-06).

The `MAPS` section records a name and opaque byte payload for each MM6 `.dlv` or `.ddm` delta. Names are lowercase basenames with no path components; duplicates, empty payloads, more than 67 entries, oversized individual payloads and a section above 32 MiB reject. The title-tagged container checks the section CRC before `loadMapFieldsBeforeMutation` assigns a decoded candidate to its caller. The codec does not interpret proprietary map content or publish it.

This representation accepts either an original archive wrapper or decoded MM6 delta wire. The existing MM6 loaders call `decodeMM6ArchiveEntry`, which accepts both, but the eventual save transaction must validate **each** decoded delta against its base map before replacing `pMapDeltas`. The transaction must also require all 67 expected map names, capture the current map through the bounded indoor/outdoor changed-state encoders, and leave prior map bytes and live state untouched if any validation or write fails. These requirements are not met by the section codec alone.

Focused checks cover synthetic round-trip, duplicate/path/empty rejection, corrupt-container rejection before destination mutation, and a byte-exact 67-delta round-trip from ignored F3-matched decoded fixtures. No original installation or MM6 process was opened for this F4 work. See the [save specification](MM6_SAVE_SPEC.md), [container packet](MM6_EC6_CONTAINER_F4.md) and [F4 build graph](MM6_F4_BUILD_GRAPH.md).
