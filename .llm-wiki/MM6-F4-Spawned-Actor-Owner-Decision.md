---
type: decision
up: "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"
related: ["[[MM6-F4-Raw-V2-Bootstrap-Respawn]]", "[[MM6-F4-Outdoor-Actor-Append]]"]
tags: [mm6, f4, save, actor, decision]
decided_at: 2026-11-07
---

# MM6 F4 Spawned Actor Owner Decision

This decision gives newly spawned MM6 outdoor actors an exact-slot,
versioned EC6 runtime owner candidate. See the
[source-backed design packet](../project/MM6_F4_SPAWNED_ACTOR_OWNER_DECISION.md).
The later [baseline actor audit](MM6-F4-Baseline-Actor-Save-Audit.md) narrows
this decision: spawned-only coverage cannot support durable gameplay saves.
Revisit the actor scope before a live manifest or loader is admitted.

## Question

How should a populated MM6 first-map save retain actors spawned after the
default native DDM was loaded?

## Options considered

- Encode full native 548-byte records for each new actor and use the
  [bounded append codec](MM6-F4-Outdoor-Actor-Append.md). The current runtime
  projection does not provide complete native fields or record provenance.
- Give new actors a versioned, title-specific EC6 runtime section, retaining
  original DDM records and opaque bytes. This requires a full runtime-field
  schema, explicit refusal for unsupported states, and a new exact manifest.

## Decision

Develop the runtime section as a detached candidate. Store actors at exact
PID slots, suppress repeated initial spawn with a nonzero DDM respawn day,
reconstruct before pre-event readers, and retain the strict final-owner
check. Do not admit a partial section into production.

## Rejected alternatives

Do not fabricate zero-filled or MM7-cast native records, or relax PID equality
to make the current bootstrap slot pass. Both would conceal missing actor
state. The native append codec remains available for a later complete encoder.

## Revisit triggers

Reconsider native-record encoding if a complete MM6 field map, provenance
policy and replay equality tests are proved. Reconsider the runtime section
if its Actor field audit finds irreducible state loss.

See also: [Bootstrap Respawn](MM6-F4-Raw-V2-Bootstrap-Respawn.md), [Outdoor Actor Append](MM6-F4-Outdoor-Actor-Append.md), [Gameplay RNG Save](MM6-F4-Gameplay-RNG-Save.md).
