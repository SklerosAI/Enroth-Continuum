#!/usr/bin/env python3
"""Synthetic checks for tools/mm7_playtest_capture.sh.

Nothing here records the screen. A stub stands in for screencapture and
writes small chunk files, a sleeping process stands in for MM7, and every
session lives in a temporary directory.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools/mm7_playtest_capture.sh"

SCREENCAPTURE_STUB = r"""#!/bin/bash
# Stand-in for screencapture: one chunk per call, CHUNK_SLEEP seconds long.
out="${@: -1}"
printf '%s\n' "$*" >> "$STUB_DIR/screencapture.calls"
[[ "${STUB_MODE:-ok}" == ok ]] || exit 1
name=$(basename "$out")
trap 'exit 143' TERM
printf 'chunk %s pid %s start\n' "$name" "$$" > "$out"
head -c 4096 /dev/urandom >> "$out"
sleep "${CHUNK_SLEEP:-1}" &
wait $!
printf 'complete\n' >> "$out"
/bin/cp "$out" "$STUB_DIR/ledger/${name#.}"
"""

DF_STUB = r"""#!/bin/bash
# Reports DF_AVAIL_KB free when that file exists, otherwise the real value.
if [[ -f "$STUB_DIR/df-avail-kb" ]]; then
    printf 'Filesystem 1024-blocks Used Available Capacity Mounted on\n'
    printf 'stub 999999999 1 %s 1%% /\n' "$(cat "$STUB_DIR/df-avail-kb")"
else
    exec /bin/df "$@"
fi
"""

LN_STUB = r"""#!/bin/bash
# Fails like a volume without hard links when NO_HARD_LINKS exists.
[[ ! -f "$STUB_DIR/no-hard-links" ]] || exit 1
exec /bin/ln "$@"
"""

CP_STUB = r"""#!/bin/bash
# Copies slowly when SLOW_CP exists, so a stop can land mid-copy.
if [[ -f "$STUB_DIR/slow-cp" ]]; then
    : > "$STUB_DIR/cp-started"
    sleep 3
fi
exec /bin/cp "$@"
"""


def write_executable(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(0o755)


def wait_for(condition, timeout: float, step: float = 0.1) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(step)
    return condition()


class CaptureCase(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = Path(tempfile.mkdtemp(prefix="mm7-capture-test-")).resolve()
        self.stub = self.temp / "stub"
        (self.stub / "ledger").mkdir(parents=True)
        (self.stub / "bin").mkdir()
        write_executable(self.stub / "screencapture", SCREENCAPTURE_STUB)
        write_executable(self.stub / "bin/df", DF_STUB)
        write_executable(self.stub / "bin/ln", LN_STUB)
        write_executable(self.stub / "bin/cp", CP_STUB)
        self.script = self.temp / "tools/mm7_playtest_capture.sh"
        self.script.parent.mkdir()
        self.script.write_text(SCRIPT.read_text().replace("/usr/sbin/screencapture", str(self.stub / "screencapture")))

        self.profile = self.temp / "profile"
        (self.profile / "logs").mkdir(parents=True)
        (self.profile / "saves").mkdir()
        self.app = self.temp / "Atlas.app"
        write_executable(self.app / "Contents/MacOS/EnrothContinuum", "#!/bin/sh\n")
        write_executable(self.app / "Contents/Helpers/MM7.app/Contents/MacOS/MM7", "#!/bin/sh\n")
        self.recordings = self.temp / "recordings"
        self.recordings.mkdir()
        self.session = self.recordings / "session"
        self.env = dict(os.environ, STUB_DIR=str(self.stub), CHUNK_SLEEP="1",
                        PATH=f"{self.stub / 'bin'}:{os.environ['PATH']}")
        self.game = subprocess.Popen(["/bin/sleep", "600"])
        self.recorder: subprocess.Popen | None = None

    def tearDown(self) -> None:
        for process in (self.recorder, self.game):
            if process and process.poll() is None:
                process.kill()
                process.wait()
        subprocess.run(["pkill", "-f", str(self.stub / "screencapture")], check=False)
        subprocess.run(["rm", "-rf", str(self.temp)], check=True)

    def cli(self, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(["/bin/bash", str(self.script), *args, "--session", str(self.session)],
                              env=self.env, text=True, capture_output=True, check=True)

    def start(self, *extra: str, video: bool = True) -> None:
        args = ["/bin/bash", str(self.script), "start", "--profile", str(self.profile),
                "--session", str(self.session), "--app", str(self.app),
                "--game-pid", str(self.game.pid), "--display", "1", "--video-chunk-seconds", "60",
                "--poll-seconds", "1", "--min-free-mib", "128", *extra]
        if video:
            args.append("--video")
        with (self.temp / "recorder.log").open("a") as log:
            self.recorder = subprocess.Popen(args, env=self.env, stdout=log, stderr=log)
        self.assertTrue(wait_for(lambda: self.status().get("state") == "running", 5))

    def finish(self, timeout: float = 30) -> None:
        assert self.recorder
        self.recorder.wait(timeout=timeout)

    def status(self) -> dict:
        try:
            return json.loads((self.session / "status.json").read_text())
        except (OSError, ValueError):
            return {}

    def completed(self) -> list[str]:
        return sorted(path.name for path in (self.session / "video").glob("chunk-*.mov"))

    def video_files(self) -> list[Path]:
        return [path for path in (self.session / "video").iterdir() if path.suffix == ".mov"]

    def clips(self) -> list[Path]:
        return sorted(path for path in (self.session / "clips").iterdir() if path.is_dir())

    def wait_for_completed(self, count: int, timeout: float = 20) -> None:
        self.assertTrue(wait_for(lambda: len(self.calls()) > count and len(self.completed()) >= min(count, 2), timeout))

    def calls(self) -> list[str]:
        path = self.stub / "screencapture.calls"
        return path.read_text().splitlines() if path.exists() else []

    def assert_clip_matches_ledger(self, clip: Path) -> list[str]:
        names = sorted(path.name for path in clip.iterdir())
        self.assertFalse([name for name in names if name.startswith(".")], "temporary files left in clip")
        for name in names:
            recorded = self.stub / "ledger" / name.replace(".mov", ".recording.mov")
            self.assertEqual((clip / name).read_bytes(), recorded.read_bytes(), name)
            self.assertTrue((clip / name).read_bytes().endswith(b"complete\n"), name)
        return names

    def assert_buffer_gone(self) -> None:
        self.assertEqual(self.video_files(), [])
        self.assertEqual(list((self.session / "video").glob(".*")), [])

    def assert_no_capture_running(self) -> None:
        result = subprocess.run(["pgrep", "-f", str(self.stub / "screencapture")],
                                capture_output=True, text=True)
        self.assertEqual(result.stdout.strip(), "")


class RollingReplayTest(CaptureCase):
    def test_roll_keeps_at_most_two_completed_chunks(self) -> None:
        # The buffer must never hold more than two completed chunks and the one being recorded.
        self.start()
        most_completed = most_files = 0
        deadline = time.monotonic() + 9
        while time.monotonic() < deadline:
            most_completed = max(most_completed, len(self.completed()))
            most_files = max(most_files, len(self.video_files()))
            time.sleep(0.05)
        self.assertGreaterEqual(len(self.calls()), 5, "the stub should have recorded several chunks")
        self.assertEqual(most_completed, 2)
        self.assertLessEqual(most_files, 3)
        self.cli("stop", "--game-exit", "normal")
        self.finish()

    def test_clip_request_copies_buffer_intact(self) -> None:
        # A clip holds the two completed chunks at the request and the chunk being recorded.
        self.env["CHUNK_SLEEP"] = "3"
        self.start()
        self.wait_for_completed(3)
        started = len(self.calls())
        self.assertTrue(wait_for(lambda: len(self.calls()) > started, 10))
        before = self.completed()
        self.cli("clip")
        self.assertTrue(wait_for(lambda: self.clips() and len(list(self.clips()[0].glob("*.mov"))) == 3, 10))
        names = self.assert_clip_matches_ledger(self.clips()[0])
        numbers = [int(name[6:12]) for name in names]
        self.assertEqual(names[:2], before[-2:])
        self.assertEqual(numbers, list(range(numbers[0], numbers[0] + 3)))
        time.sleep(7)  # The roll continues past the clip.
        self.assertEqual(len(self.assert_clip_matches_ledger(self.clips()[0])), 3)
        self.cli("stop", "--game-exit", "normal")
        self.finish()
        self.assertEqual(len(self.clips()), 1)
        self.assert_buffer_gone()
        self.assertEqual(self.status()["clips"], 1)

    def test_abnormal_exit_keeps_clip(self) -> None:
        # A crash keeps the buffer, including the chunk recording when MM7 died.
        self.start()
        self.wait_for_completed(3)
        self.game.kill()
        self.game.wait()
        self.cli("stop", "--game-exit", "abnormal")
        self.finish()
        status = self.status()
        self.assertEqual(status["video"], "kept")
        self.assertEqual(len(self.clips()), 1)
        names = self.assert_clip_matches_ledger(self.clips()[0])
        self.assertEqual(len(names), 3)
        self.assertEqual(int(names[-1][6:12]), len(self.calls()))
        self.assert_buffer_gone()
        self.assert_no_capture_running()
        self.assertIn("abnormal exit", (self.session / "timeline.tsv").read_text())

    def test_unreported_game_exit_keeps_clip(self) -> None:
        # With no stop request the exit status is unknown, so the buffer is kept.
        self.start()
        self.wait_for_completed(2)
        self.game.kill()
        self.game.wait()
        self.finish(timeout=40)
        self.assertEqual(self.status()["state"], "game-exited")
        self.assertEqual(self.status()["video"], "kept")
        self.assertEqual(len(self.clips()), 1)
        self.assert_clip_matches_ledger(self.clips()[0])
        self.assert_buffer_gone()

    def test_normal_exit_discards_video(self) -> None:
        # A normal exit deletes the buffer and keeps logs and saves.
        (self.profile / "saves/save000.mm7").write_bytes(b"save one")
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text("line one\n")
        self.start()
        self.wait_for_completed(3)
        self.game.terminate()
        self.game.wait()
        self.cli("stop", "--game-exit", "normal")
        self.finish()
        status = self.status()
        self.assertEqual(status["video"], "discarded")
        self.assertEqual(status["clips"], 0)
        self.assertEqual(self.clips(), [])
        self.assert_buffer_gone()
        self.assert_no_capture_running()
        self.assertEqual([path.read_bytes() for path in (self.session / "saves").glob("*.mm7")], [b"save one"])
        self.assertEqual([path.read_text() for path in (self.session / "logs").glob("*.log.part")], ["line one\n"])
        self.assertFalse((self.session / "warnings.txt").exists())

    def test_low_space_stops_recording(self) -> None:
        # Free space under the floor stops the recorder and its capture child.
        self.start()
        self.wait_for_completed(2)
        (self.stub / "df-avail-kb").write_text(str(100 * 1024))
        self.finish()
        status = self.status()
        self.assertEqual(status["state"], "low-space")
        self.assertIn("free space fell below 128 MiB", (self.session / "warnings.txt").read_text())
        self.assert_buffer_gone()
        self.assert_no_capture_running()
        self.assertTrue(self.game.poll() is None, "the recorder must never stop MM7")

    def test_stop_during_clip_copy_is_safe(self) -> None:
        # A stop and a signal that land during a slow clip copy leave whole files only.
        self.start()
        self.wait_for_completed(3)
        (self.stub / "no-hard-links").touch()
        (self.stub / "slow-cp").touch()
        self.cli("clip")
        self.assertTrue(wait_for(lambda: (self.stub / "cp-started").exists(), 10))
        self.cli("stop", "--game-exit", "normal")
        assert self.recorder
        self.recorder.send_signal(signal.SIGTERM)
        self.finish(timeout=60)
        (self.stub / "slow-cp").unlink()
        status = self.status()
        self.assertEqual(status["state"], "stopped")
        self.assertIsNot(status["ended_at"], None)
        self.assertEqual(len(self.clips()), 1)
        names = self.assert_clip_matches_ledger(self.clips()[0])
        self.assertEqual(len(names), 3, "the pending clip still receives the chunk being recorded")
        self.assert_buffer_gone()
        self.assert_no_capture_running()

    def test_failed_video_warns_about_permission(self) -> None:
        # A capture that writes nothing warns about Screen Recording permission and keeps logging.
        self.env["STUB_MODE"] = "fail"
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text("line one\n")
        self.start()
        self.assertTrue(wait_for(lambda: self.status().get("video") == "failed", 10))
        self.assertIn("Screen Recording permission", (self.session / "warnings.txt").read_text())
        self.assertEqual(len(self.calls()), 1, "a failed capture is not retried")
        self.cli("stop", "--game-exit", "abnormal")
        self.finish()
        self.assertEqual(self.status()["video"], "failed")
        self.assertEqual(self.clips(), [])
        self.assertEqual(len(list((self.session / "logs").glob("*.log.part"))), 1)

    def test_no_video_option_records_no_video(self) -> None:
        # Without --video the recorder never starts a capture.
        self.start(video=False)
        time.sleep(2)
        self.cli("clip")
        self.cli("stop", "--game-exit", "abnormal")
        self.finish()
        self.assertEqual(self.calls(), [])
        self.assertEqual(self.status()["video"], "off")
        self.assertEqual(self.clips(), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
