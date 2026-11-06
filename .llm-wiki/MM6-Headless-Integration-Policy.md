---
type: decision
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[Agent-Handoff]]", "[[Runtime-Isolation]]"]
decided_at: 2026-11-06
tags: [mm6, headless, testing, integration]
---

# MM6 Headless Integration Policy

MM6 integration tests may run while the owner uses the Mac only after the test path proves it creates no native window and consumes no physical keyboard or mouse input.

## Question

How can MM6 code be joined and checked before graphical play without taking over the owner's computer or waiting until every campaign rule is finished?

## Options considered

- Wait for a complete offline implementation before any engine process: keeps the original F4/F5 division, but postpones failures at the actual engine boundary.
- Repeatedly launch the graphical partial game: reveals integration failures, but interrupts the owner and gives poor reproducibility.
- Add an MM6-specific headless game-test path with synthetic events and in-memory user data: exercises the real engine while keeping native input and windows out of the session.

## Decision

Use the third option. The existing `GameStarter` headless branch selects `NullPlatform`, `RENDERER_NULL` and `ALSOFT_DRIVERS=null`; `NullPlatform` supplies a `NullWindow`, its event loop receives no OS messages, and its window's native handle is null. The game-test runner now has an MM6 selector while defaulting to MM7 and using in-memory user data. The first linked MM6 run proved no native window or focus change under a bounded host monitor, then stopped at a missing private rule manifest. The explicit in-memory rule-seeding follow-up awaits its linked rerun, so **MM6 headless game testing is not yet a passing capability**.

For each MM6 process run, require the title-specific selector, explicit private data and rules paths, and bounded checks that no native window appears, focus does not change, and physical keyboard and mouse remain untouched. Tests inject only synthetic events into the test controller. Do not use Finder launch, AppleScript, computer-use clicks, or a normal GUI engine process for background testing. The null renderer yields no graphical acceptance evidence.

Run a small first-map headless scenario after its chest and startup seams join, then add save/reload, cross-map travel, service, combat and campaign checkpoints as each vertical slice is complete. Each scenario asserts title isolation, RNG/state invariants, and a durable or in-memory state round-trip appropriate to that slice. Keep unresolved behavior guarded. A full campaign readiness pass precedes graphical F5 and Atlas installation. Graphical and audible checks occur only in a separate session when the owner is free to use the screen.

## Rejected alternatives

- An all-or-nothing F4 engine build is insufficient because detached unit tests do not exercise startup, global state, map transitions or UI event routing together.
- Normal graphical partial launches are unsuitable during the owner's active computer use.

## Revisit triggers

Revisit this choice if a headless test creates a native window, changes focus, accesses physical input, touches the installed MM7 profile, or cannot faithfully drive the required engine paths. Stop the background run and repair the harness before further MM6 process tests. Investigate a separate offscreen renderer only if visual checks are needed without a foreground session; `RENDERER_NULL` cannot validate graphics.

The [F4 headless harness packet](MM6-F4-Headless-Harness.md) records the first linked-process result and pending rule-seeded rerun.

Source: [local startup](../src/Application/Startup/GameStarter.cpp), [null platform](../src/Library/Platform/Null/NullPlatform.cpp), [null event loop](../src/Library/Platform/Null/NullEventLoop.cpp), [game-test options](../test/Bin/GameTest/GameTestOptions.cpp), and [upstream game-test guidance](https://github.com/OpenEnroth/OpenEnroth/blob/master/HACKING.md). See also: [F4 Offline Seams](MM6-F4-Offline-Seams.md), [Agent Handoff](Agent-Handoff.md), [Runtime Isolation](Runtime-Isolation.md).
