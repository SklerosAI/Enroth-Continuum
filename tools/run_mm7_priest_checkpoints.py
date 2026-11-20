#!/usr/bin/env python3
"""Run MM7 Priest promotion save checkpoints in fresh game processes."""

import argparse
from pathlib import Path
import subprocess
import tempfile


MARKER = ".ec-mm7-disposable-save-gate"
MARKER_TEXT = "EnrothContinuum-MM7-save-gate-v1\n"
PAIRS = (
    ("DISABLED_WriteEvenmornMap", "DISABLED_ReloadEvenmornMapAndPromote"),
    ("DISABLED_WriteMoonAltar", "DISABLED_ReloadMoonAltarAndPromote"),
    ("DISABLED_WriteSunAltar", "DISABLED_ReloadSunAltarAndPromote"),
)


def run_pair(binary: Path, data: Path, test_data: Path, writer: str, reader: str) -> None:
    with tempfile.TemporaryDirectory(prefix="mm7-save-gate-priest-") as temporary:
        profile = Path(temporary)
        (profile / MARKER).write_text(MARKER_TEXT)
        for test in (writer, reader):
            command = (
                str(binary), "--game=mm7", "--headless", f"--data-path={data}",
                f"--test-path={test_data}", f"--mm7-disposable-profile={profile}",
                f"--gtest_filter=MM7PriestCheckpoint.{test}", "--gtest_also_run_disabled_tests",
                "--log-level=critical",
            )
            print(f"Running MM7PriestCheckpoint.{test}", flush=True)
            subprocess.run(command, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--data-path", type=Path, required=True)
    parser.add_argument("--test-path", type=Path, required=True)
    args = parser.parse_args()
    for path in (args.binary, args.data_path, args.test_path):
        if not path.exists():
            parser.error(f"missing path: {path}")
    for writer, reader in PAIRS:
        run_pair(args.binary.resolve(), args.data_path.resolve(), args.test_path.resolve(), writer, reader)


if __name__ == "__main__":
    main()
