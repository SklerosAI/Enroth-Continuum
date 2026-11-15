# Claude, MM6 Question prompts lane, 2026-11-12

- Owner: Claude Code, MM6 owner. Branch `claude/mm6-questions`, worktree `../enroth-worktrees/mm6-leftovers`, off master `7024a995c`.
- Recipient: the lead, and the C6 and C7 chapter lanes.

## Commits

| Commit | What |
| --- | --- |
| `cb9744f45` | `DrawnText` tape payload carries the position a string was drawn at (shared test infrastructure). |
| `477ba3356` | MM6 Question prompt follows MM6.exe: message box keeps the ShowMessage text, status line draws "question typed" at (13, 355), answers compare as typed ignoring ASCII case only, Escape takes the wrong branch. Tests `MM6Questions.*`, chain helpers, hidden capture `MM6OffscreenFrame.DISABLED_QuestionCapture`. |
| `a957bc99b` | `Party.cpp` hireling names are cleared instead of having a NUL written over their first character. Test `MM6NpcDialogue.NewGameAfterHire`. Shared with MM7. |

## Question records

19 records on 6 maps, from all 83 evt files in `local-game-data/mm6/research-2026-11-05/icons/` (67 listed maps, `global.evt` and 15 unlisted). `global.evt` has none. cd1 2 (password plate, event 69), cd2 3 (sarcophagi 61-63, Yes/Y), cd3 2 (Guardian 13 and Curator 27, Yes/Y), d08 4 (Magic Door riddles 61-64), outd3 2 (event 210, its strings don't fit a question), pyramid 6 (Well of VARN 1 and Cleansing Pools 21-25). The table with every branch is in `local-game-data/private-wiki/mm6/QUESTIONS_PE_TRACE_2026-11-12.md`.

## Verified against MM6.exe

Opcode 26 handler 0x43D451, prompt window type 19 (0x419905, text input max 15), keys 0x44C940, draw and resume 0x43A890, Escape message 113 for screen 19 at 0x42BD55, `_stricmp` 0x4AF370. Details and the port comparison are in the trace.

## How chapters answer a Question

From `test/Bin/GameTest/MM6CampaignChain.h`:
- `walkOntoMM6CampaignChainIndoorEventPlate(game, eventId)` for a plate (cd1 event 69, the pyramid pools and the Well of VARN plates). The pools need the code scroll held (items 537-542).
- `useMM6CampaignChainIndoorEventFace(game, eventId)` for a clicked face (cd2 sarcophagi, cd3 Guardian and Curator, d08 doors).
- The pyramid answers are stored reversed (kriK, kcopS, ...) but MM6 compares the typed text with them as is, ignoring case. The code scrolls 537-542 spell them the same way ("speak activator key 'kcops'"), so type the scroll's word: `krik`, `kcops`, `ulus`, `aruhu`, `yttocs`, `yoccm`. Pools 21-25 first, the Well (event 1) last.
- Then `answerMM6CampaignChainQuestion(game, "jbard")`. Lowercase letters, digits and spaces. It asserts that a Question prompt is open, types the answer, presses Enter and ticks until the event has resumed.

## Tests

- Unit: 1122 passed, 45 skipped.
- Full MM6 suite: 183 passed and 2 skipped after the Question commit, 184 passed and 2 skipped (`MM6ZSaveHeadless`, needs private research files) after the hireling commit.
- MM7 controls after the hireling commit (`MM7*:VendorRefreshIntegration.*:Prs.Pr2354:Prs.Pr2956:Issues.Issue840`): 147 passed, 2 skipped (`VendorRefreshIntegration` weapon shop and magic guild).
- Revert check: with the prompt fix reverted, `Cd1PasswordPlate`, `Cd1PasswordEscape` and `Cd1PasswordPromptOnStatusLine` fail, `PyramidFirstMateCode` passes (it guards behaviour that already worked). With the hireling fix reverted, `NewGameAfterHire` fails with "This party can't be saved yet" on the status bar.
- `check_style` passes. `check_tidy` not run, no clang-tidy 22 on this machine.

## Captures

`local-game-data/progress-captures/20261112-190122-mm6-questions-prompt.png` (cd1 prompt: box "Restricted area - Keep out.", status line "What's the password? jb_", food 7 and gold 200 drawn) and `20261112-190122-mm6-questions.png` (default first-map capture). In the first-map capture the food and gold numbers are missing next to the apple and the coins. That is the start-capture item the leftovers lane didn't reach, this lane didn't touch it.

## Open items

- A Question reached while another branchless window is open throws in the port, MM6 ends the event. No shipped script reaches it.
- cd2, cd3 and d08 Questions use the same prompt but no test drives them. The status file used to call the cd2 and cd3 ones crystal steps, they are the sarcophagi and the Kriegspire Guardian and Curator.
- 0x48FB40 on 0x9CF598, called when the prompt opens, is not identified.
