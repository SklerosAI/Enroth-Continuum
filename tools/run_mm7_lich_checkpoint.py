#!/usr/bin/env python3
"""Run the physical Lich jars Save/Load checkpoint in two fresh processes."""

import argparse
from pathlib import Path
import subprocess
import tempfile


TESTS = ("DISABLED_WritePhysicalJars", "DISABLED_ReloadPhysicalJarsAndPromote")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--data-path", type=Path, required=True)
    parser.add_argument("--test-path", type=Path, required=True)
    args = parser.parse_args()
    for path in (args.binary, args.data_path, args.test_path):
        if not path.exists():
            parser.error(f"missing path: {path}")
    with tempfile.TemporaryDirectory(prefix="mm7-save-gate-lich-") as temporary:
        profile = Path(temporary)
        (profile / ".ec-mm7-disposable-save-gate").write_text("EnrothContinuum-MM7-save-gate-v1\n")
        for test in TESTS:
            command = (
                str(args.binary.resolve()), "--game=mm7", "--headless",
                f"--data-path={args.data_path.resolve()}", f"--test-path={args.test_path.resolve()}",
                f"--mm7-disposable-profile={profile}", f"--gtest_filter=MM7LichCheckpoint.{test}",
                "--gtest_also_run_disabled_tests", "--log-level=critical",
            )
            print(f"Running MM7LichCheckpoint.{test}", flush=True)
            subprocess.run(command, check=True)


if __name__ == "__main__":
    main()
