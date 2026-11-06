# MM6 F4 direct tier-seven chest artifact picker

Status: **detached safe subset; live MM6 chest generation remains blocked** (2026-11-06). This is the original direct tier-seven source called by the chest generator, separate from the tier-six ANY item's 5% trial and 13-generated-artifact cap. The accepted bounded PE trace was reexamined without reopening the executable or launching MM6. Exact instructions and private data locators remain ignored.

## Source-backed branch behavior

The picker zeroes a local candidate mask, then scans four memory windows. Each window reads 581 consecutive 28-byte records, and consecutive window bases are only 5,660 bytes apart, so the windows overlap and extend beyond four character strides. It checks a record's ID for 400–429. **The mark write uses the scan slot index, not the checked item ID.** Only slot indexes 400–430 address the local 31-byte candidate mask. An artifact-bearing record at another slot writes elsewhere relative to the local stack frame; that branch cannot be safely treated as ordinary party ownership exclusion.

It then builds a candidate list from IDs **400–430 inclusive**, testing that mask. The retained item table assigns ID 430 a non-artifact role, so this extra candidate is an original executable quirk, not evidence that MM6 has a 31st normal artifact. If the list is empty, the picker clears its 28-byte destination and returns failure without an RNG draw. Otherwise it clears the destination, draws once from the original 15-bit RNG modulo candidate count, writes the chosen ID and returns success. The function neither reads nor writes the tier-six 30-byte generated-artifact flags and has no 13-artifact cap. It also does not initialize regular item flags or enchantments after selection.

## Detached model and tests

`mm6::pickDirectChestArtifactDetached` accepts a synthetic contiguous byte image covering all four overlapping scan windows and an RNG seed. It returns a staged ID (zero for no candidate), candidate count, next RNG state and draw count. It rejects MM7 and any scan in which an artifact ID occurs outside slots 400–430, avoiding the original out-of-mask mark write. It never reads live party memory, writes item records or alters global artifact flags.

Seeded tests cover all 31 initial candidates, the original slot-index rather than ID-based mask, absence of the tier-six 13 cap, ID 430 as the sole remaining candidate, empty-list/no-draw behavior, MM7 isolation and fail-closed unsafe writes. A native standalone harness executes the same numerical vectors; the focused unit test source syntax is checked separately when disk capacity prevents a fresh full unit build. These tests establish the bounded branch, not the safety or intent of the original ownership scan.

### Explicit safe compatibility choice

`mm6::pickDirectChestArtifactSafePolicy` is a second detached selector for a possible future runtime policy. It takes **caller-validated party-owned flags** and title-specific generated flags for IDs 400–429. It excludes an ID present in either set, caps the **union** of owned/generated IDs at 13, draws once over the remaining 30-ID band, and stages the selected generated flag. It rejects nonbinary flags and MM7 before changing any state. This is deterministic and prevents duplicate grants across those two sets, but it intentionally differs from the original direct picker in its ownership source, cap, exclusion of ID 430 and generated-state write. It is an option for review, not an established original MM6 rule or a live caller.

## Remaining gate

Trace the original scan's intended memory object and what is at every artifact-bearing slot in a real party, then review the explicit safe policy against desired gameplay. A title-specific runtime adapter needs bounded character inventory/equipment projections, a decision on the 13-union cap and ID 430, and persistence of the generated-state array, followed by seeded chest and save–reload tests. The live MM6 chest guard must remain closed. MM7's artifact picker and runtime are unchanged. See the broader [chest audit](MM6_F4_CHEST_GENERATION_AUDIT.md) and [wiki packet](../.llm-wiki/MM6-F4-Chest-Direct-Artifact.md).
