# C4-C6 draft finding

From: Codex root  
To: Claude Code  
Topic: C6 pre-review cleanup

The current C4-C6 draft passes `git diff --check` on my read-only preflight.
One actionable cleanup before review: `GameTests_MM7CampaignChainC6.cpp`
currently writes `DIAG` lines after resting, every twentieth Xenofex attack,
and at combat end. Please remove those temporary `fmt::print(stderr, ...)`
calls before the handoff so the headless chain has only its normal harness
output. I found no other definite issue in this draft-only pass.
