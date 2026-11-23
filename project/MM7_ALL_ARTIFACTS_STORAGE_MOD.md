# MM7 All Artifacts + Storage

## Purpose and scope

**All Artifacts + Storage** is an optional, native MM7 gameplay modification
for completionist playthroughs. It removes the original thirteen-artifact cap
and changes random-artifact uniqueness from generated to collected, without
changing treasure levels, the tier-six five-percent roll, monster/chest loot
families, item statistics, or the random-artifact catalog.

The implementation is MM7-only. MM6 remains owned by Claude and is preserved
as a future plan request in
[`agent_comms/messages/2026-11-16T00-05-04Z--from-codex--to-claude--mm6-all-artifacts-storage-plan.md`](../agent_comms/messages/2026-11-16T00-05-04Z--from-codex--to-claude--mm6-all-artifacts-storage-plan.md).

## Activation

The source default is off. In The Atlas:

1. Open **The Atelier**.
2. Turn on **All Artifacts + Storage - MM7**.
3. Select **Use private mods**.
4. Start MM7 again.

Atlas stores `mods.mm7.allArtifactsStorage` in its local preferences and passes
the exact launch flag `ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1` only to its forked
MM7 helper while Private mods is active. Vanilla MM7, MM6 and upstream
OpenEnroth do not receive an active flag. Direct diagnostic launches may set
that exact environment value; absent, `0`, or any other value leaves the mod
off.

The profile's `artifact_limit` and `chest_try_place_items` values are not
rewritten. Turning the option off therefore restores the configured vanilla
rules on the next launch.

## Artifact generation and collection

`allSpawnableArtifacts()` remains authoritative: fourteen artifacts and
fifteen relics, 29 IDs total. Fixed-location artifacts, quest artifacts and
other special items are not added to random loot.

The native `Party::isArtifactGenerated` ledger is retained and continues to be
updated. When the mod is on, generation instead excludes items in a separate
monotonic `isArtifactCollected` ledger:

- generating an artifact in a chest, corpse or world object does not collect it;
- placing it on the party cursor, adding it to a party inventory, or taking it
  directly through the chest grab action collects it;
- selling, dropping, losing or storing it later does not clear collection;
- generated but never acquired artifacts remain eligible after their world
  instances disappear;
- after all 29 eligible IDs are collected, guaranteed generation fails through
  the existing safe fallback and tier-six rolls produce ordinary loot.

Before collection, two world sources may contain the same artifact at once.
Preventing that would require a persisted outstanding-instance ledger and
lifecycle hooks for every map reset and despawn; this focused implementation
does not delete or rewrite already-generated world items.

## Save compatibility and migration

The collected ledger is save-specific and versioned inside the 39 unused bytes
already present in `Party_MM7::field_7d7`. `Party_MM7` remains exactly the same
size and `party.bin` retains its native layout. Once initialized, the ledger is
preserved even when the mod is disabled.

On first opt-in for an older save, migration records spawnable artifacts found
in party backpacks, equipped item records and the cursor. It deliberately does
not reinterpret every historic generated flag as proof of collection. An
artifact already left in a Castle Harmondale chest or elsewhere in the world is
recorded when the party next withdraws or picks it up. Exact pickup history for
sold or lost items cannot be reconstructed from an old native save.

Migration never deletes, moves or duplicates an item.

## Castle Harmondale storage

MM7 already supplies permanent storage in Castle Harmondale (`d29.blv`, saved
as `d29.dlv`). Its twenty authored chest events 176-195 address chest IDs 0-19
exactly once. The engine serializes every chest's full item records and placement
grid in the indoor map delta. Castle Harmondale is excluded from timed indoor
respawn, and the castle rebuild transaction changes quests, awards and NPC homes
without replacing that delta.

The mod therefore adds no parallel vault or storage UI. Deposit collected
artifacts in the existing castle chests; their collection bits remain set even
while the items are outside party inventory.

## Focused validation

The representative automated packet covers:

- exact MM7-only launch-flag parsing and the disabled vanilla control;
- collecting all 29 IDs with the configured cap set to one;
- generated-but-uncollected re-eligibility and acquired exclusion;
- tier-six exhaustion and safe ordinary-item fallback;
- cursor, party insertion and direct chest-grab acquisition hooks;
- conservative old-save migration and new-ledger snapshot roundtrip;
- one Castle Harmondale boundary chest with exact item fields, placement grid,
  elapsed reset time and collection state across a save/delta roundtrip;
- the exact 176-195 to chest 0-19 event permutation.

Run the focused engine cases with:

```sh
ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1 \
  build/test/Bin/GameTest/OpenEnroth_GameTest \
  --game mm7 --headless --data-path "$MM7_DATA_PATH" --test-path test/Data \
  --gtest_filter='MM7AllArtifactsEngine.*:MM7HarmondaleStorage.*'
```

The flag-parsing checks are unit tests in the unit-test binary:

```sh
build/test/Bin/UnitTest/OpenEnroth_UnitTest --gtest_filter='MM7PrivateMods.*'
```

Run the disabled control without the flag:

```sh
env -u ENROTH_MM7_ALL_ARTIFACTS_STORAGE \
  build/test/Bin/GameTest/OpenEnroth_GameTest \
  --game mm7 --headless --data-path "$MM7_DATA_PATH" --test-path test/Data \
  --gtest_filter='MM7AllArtifactsEngine.DisabledModeLeavesCollectionStateUntouched:Issues.Issue1988_ArtifactLimitOnLevelSeven'
```

For the next visible playtest, load an existing save, acquire one new random
artifact, put it in a Castle Harmondale chest, save, quit, relaunch and load.
Confirm the full item remains in the chest. A long farming session is still the
manual acceptance check for natural rarity and pacing.

