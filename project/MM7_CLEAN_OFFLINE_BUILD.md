# Mac MM7 build from owned source

Status (2026-11-07): this local publication preview builds a staged MM7 Atlas
from a fresh extraction of its tracked files with network access denied. It
has not been pushed or installed and has not passed visible save/quit/reload.

Run `python3 tools/build_mm7_macos.py /absolute/new/output-directory` on
Apple silicon. The wrapper requires local Python, CMake, Ninja, and Apple's
SDK/toolchain. It creates a new output directory, builds five pinned media
source families into nine arm64 static archives, configures the local-only
CMake lane with the scoped Apple iconv adapter, links `MM7.app`, and stages
`Atlas/Enroth Continuum.app`. It validates source pins, archive hashes,
library links and notices. Original game files and saves remain external.

For this preview, `git archive HEAD` produced a clean source directory with
no Git metadata, submodule checkout, dependency ZIP or private game data. The
wrapper ran in a macOS sandbox denying network access and completed the
dependency build, MM7 compile and Atlas staging. The helper is arm64 and its
dynamic links are Apple system libraries/frameworks only. The staged helper
has bundle ID `org.sklerosai.enrothcontinuum.mm7` and version `0.1.0`; both
helper and Atlas passed strict code-signature verification. The app includes
the retained OpenEnroth license, credits and upstream notes, plus 23 source
notices for the owned media libraries. A bounded headless MM7 launch using an
explicit private data path remained running for eight seconds. A foreign
`--game mm6` request is rejected before initialization.

This is a source and bounded runtime check, not full campaign acceptance. A
visible launch from this staged app, sound, existing-save load, new save,
quit and reload are still required before replacing the known-good installed
Atlas. The ordinary shared CMake path retains historical upstream fetch
routes; the dedicated Mac MM7 wrapper selects the verified local-only lane.
The Apple iconv adapter covers MM7 production encodings and is not a full
legacy/CJK replacement. Preserve OpenEnroth Git history, license, notices
and authorship when publishing this branch.
