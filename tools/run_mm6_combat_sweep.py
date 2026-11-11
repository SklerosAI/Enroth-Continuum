#!/usr/bin/env python3
"""Run MM6MapSweep.DISABLED_MonstersAttackParty over MM6 maps, one headless process per map.

Each kind of monster on a map gets the party to itself for a number of frames. A throw ends the process, so the map runs
again without the monster it happened with, until every kind has had its turn. Per-map records, logs and the summary go
to a Git-ignored folder, by default local-game-data/mm6/combat-sweep-YYYY-MM-DD. The summary lists every throw with the
monster that caused it and ranks throw categories by the main-quest maps they block, read from a private --main-quest
list. Set MM6_DATA_PATH and MM6_RULES_PATH as for run_mm6_firstmap_gate.py.
"""

import argparse
import concurrent.futures
import datetime
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from run_mm6_map_sweep import GAME_TEST, ROOT, category, process_death, run_game_test, throw_site


TEST = "MM6MapSweep.DISABLED_MonstersAttackParty"


def sweep_one(args, map_id, stem, skipped):
    suffix = f"-{len(skipped)}" if skipped else ""
    record_path = args.out / "records" / f"{stem}{suffix}.json"
    log_path = args.out / "logs" / f"{stem}{suffix}.log"
    record_path.unlink(missing_ok=True)
    env = {"ENROTH_MM6_COMBAT_SWEEP_MAP": str(map_id), "ENROTH_MM6_COMBAT_SWEEP_RESULT": str(record_path),
           "ENROTH_MM6_COMBAT_SWEEP_FRAMES": str(args.frames)}
    if skipped:
        env["ENROTH_MM6_COMBAT_SWEEP_SKIP_MONSTERS"] = ",".join(map(str, skipped))
    code, seconds = run_game_test(args.data, args.rules, env, log_path, args.timeout, TEST)
    log_text = log_path.read_text(errors="replace")
    record = json.loads(record_path.read_text()) if record_path.is_file() else {"phase": "no record"}
    record["exitCode"], record["seconds"] = code, round(seconds, 1)
    record["death"], record["crashSource"], record["crash"] = "", "", False
    if record.get("phase") != "done" and not record.get("loadError"):
        record["death"], record["crashSource"], record["crash"] = process_death(log_text, code)
    record_path.write_text(json.dumps(record, indent=1) + "\n")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / f"local-game-data/mm6/combat-sweep-{datetime.date.today().isoformat()}")
    parser.add_argument("--maps", nargs="*", help="map file stems or IDs to run, the --main-quest maps by default")
    parser.add_argument("--main-quest", type=Path, required=True, help="private file listing main-quest map stems, one per line")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--timeout", type=int, default=900, help="seconds per process")
    parser.add_argument("--frames", type=int, default=150, help="frames each kind of monster gets")
    parser.add_argument("--retries", type=int, default=12, help="reruns past monsters that end the process")
    args = parser.parse_args()

    for key in ("MM6_DATA_PATH", "MM6_RULES_PATH"):
        if not os.environ.get(key) or not Path(os.environ[key]).is_dir():
            print(f"Set {key} to an existing private directory.", file=sys.stderr)
            return 2
    args.data, args.rules = Path(os.environ["MM6_DATA_PATH"]), Path(os.environ["MM6_RULES_PATH"])
    if not GAME_TEST.is_file():
        print("Build OpenEnroth_GameTest first.", file=sys.stderr)
        return 2
    args.out = args.out.resolve()
    if subprocess.run(["git", "check-ignore", "-q", str(args.out / "summary.md")], cwd=ROOT).returncode != 0:
        print(f"{args.out} is not Git-ignored, refusing to write private results there.", file=sys.stderr)
        return 2
    (args.out / "records").mkdir(parents=True, exist_ok=True)
    (args.out / "logs").mkdir(exist_ok=True)

    main_quest = [line.strip() for line in args.main_quest.read_text().splitlines() if line.strip()]
    stems = args.maps or main_quest
    started = time.monotonic()
    list_path = args.out / "maps.txt"
    run_game_test(args.data, args.rules, {"ENROTH_MM6_MAP_SWEEP_MAP": "list", "ENROTH_MM6_MAP_SWEEP_RESULT": str(list_path)},
                  args.out / "logs" / "list.log", 120)
    ids = {line.split()[1].rsplit(".", 1)[0]: int(line.split()[0]) for line in list_path.read_text().splitlines() if line.strip()}

    def sweep(stem):
        # Again without each monster that ended a run, until a run finishes.
        skipped, throws, passes = [], [], []
        while True:
            record = sweep_one(args, ids[stem], stem, skipped)
            passes.append(record)
            monster = record.get("currentMonster", 0)
            if record.get("phase") != "combat" or not monster or len(skipped) >= args.retries:
                break
            throws.append({"monster": monster, "attacking": record.get("attackingMonsters", []),
                           "error": ("crash: " if record["crash"] else "") + record["death"], "crashSource": record["crashSource"]})
            skipped.append(monster)
        print(f"{stem}: {len(throws)} throws, " + ", ".join(str(record.get("phase")) for record in passes), flush=True)
        return stem, passes, throws

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(sweep, stems))
    total = time.monotonic() - started

    rows, categories = [], {}
    for stem, passes, throws in results:
        first, last = passes[0], passes[-1]
        monsters = {monster["id"]: monster for record in passes for monster in record.get("monsters", []) if monster.get("frames")}
        census = first.get("monsters", [])
        for value in throws:
            name = category(value["error"])
            entry = categories.setdefault(name, {"site": value["crashSource"] or throw_site(name), "maps": set(), "monsters": set()})
            entry["maps"].add(stem)
            entry["monsters"].add(value["monster"])
        rows.append({
            "map": stem, "mainQuest": stem in main_quest, "phase": last.get("phase"), "loadError": first.get("loadError", ""),
            "kinds": len(census), "placed": sum(monster.get("placed", False) for monster in monsters.values()),
            "attacked": sorted(monster["id"] for monster in monsters.values() if monster.get("attacked")),
            "unplaced": sorted(monster["id"] for monster in census if monster["id"] not in monsters),
            "projectiles": sorted({sprite for monster in monsters.values() for sprite in monster.get("projectiles", [])}),
            "throws": throws, "census": census, "passes": len(passes),
            "seconds": round(sum(record.get("seconds", 0) for record in passes), 1),
        })

    ranked = sorted(categories.items(), key=lambda item: (-len(item[1]["maps"]), item[0]))
    summary = {"maps": len(rows), "throwingMaps": sorted(row["map"] for row in rows if row["throws"]), "seconds": round(total, 1),
               "rows": rows, "categories": [{"category": name, "site": entry["site"], "maps": sorted(entry["maps"]),
                                             "monsters": sorted(entry["monsters"])} for name, entry in ranked]}
    (args.out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")

    lines = [f"# MM6 combat sweep, {datetime.date.today().isoformat()}", "",
             f"{summary['maps']} maps, {len(summary['throwingMaps'])} with a throw, {args.frames} frames per kind of monster. "
             f"Total {total / 60:.1f} minutes.", "",
             "| Map | Phase | Kinds | Placed | Attacked | Not placed | Projectiles | Throws |",
             "| --- | --- | ---: | ---: | --- | --- | --- | --- |"]
    for row in rows:
        throws = "; ".join(f"{value['monster']}: {value['error']}" for value in row["throws"])
        lines.append(f"| {row['map']} | {row['phase']} {row['loadError']} | {row['kinds']} | {row['placed']} "
                     f"| {','.join(map(str, row['attacked']))} | {','.join(map(str, row['unplaced']))} "
                     f"| {','.join(map(str, row['projectiles']))} | {throws} |")
    lines += ["", "## Throw categories", "", "| Category | Site | Maps | Monsters |", "| --- | --- | ---: | --- |"]
    for name, entry in ranked:
        lines.append(f"| {name} | {entry['site']} | {len(entry['maps'])} | {','.join(map(str, sorted(entry['monsters'])))} |")
    (args.out / "summary.md").write_text("\n".join(lines) + "\n")
    print(f"{summary['maps']} maps, {len(summary['throwingMaps'])} with a throw, {total / 60:.1f} minutes. "
          f"Summary in {args.out / 'summary.md'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
