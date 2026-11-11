"""Paired biological experiments; runtime is not a benchmark result."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
CASES = [("normal", 0), ("cluster", 0), ("scarce", 0), ("crisis", 0),
         ("normal", 359), ("neutral", 0), ("neutral", 359)]


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--seeds", type=int, default=20)
    p.add_argument("--ticks", type=int, default=600)
    p.add_argument("--output-dir", default="x64/ecology/study")
    p.add_argument("--csv", default="CB3_ECOLOGY_RESULTS.csv")
    p.add_argument("--case", choices=sorted({c[0] for c in CASES}))
    args = p.parse_args()
    out = ROOT / args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    destination = ROOT / args.csv
    executables = {v: ROOT / "x64/ecology" / v / "ecology.exe" for v in ("before", "after")}
    cases = [c for c in CASES if not args.case or c[0] == args.case]
    manifest = {"seeds": [1009 + 7919*i for i in range(args.seeds)], "ticks": args.ticks,
                "cases": cases, "threads": 4, "executables": {v: sha(f) for v, f in executables.items()},
                "protocol_sha256": sha(ROOT / "CB3_ECOLOGY_PROTOCOL.md"),
                "measurement_sha256": sha(ROOT / "tests/CB3Ecology.cpp"),
                "runner_sha256": sha(Path(__file__))}
    manifest = json.loads(json.dumps(manifest))
    manifest_path = out / "manifest.json"
    if manifest_path.exists() and json.loads(manifest_path.read_text()) != manifest:
        raise RuntimeError("Study inputs changed; choose a different output directory and CSV.")
    manifest_path.write_text(json.dumps(manifest, indent=2))
    rows = list(csv.DictReader(destination.open(encoding="utf-8-sig", newline=""))) if destination.exists() else []
    done = {(r["variant"], int(r["seed"]), r["scenario"], int(r["shift"]))
            for r in rows if int(r["tick"]) == args.ticks}
    initial_hashes = {}
    for r in rows:
        key = (r["scenario"], r["seed"])
        if key in initial_hashes and initial_hashes[key] != r["initial_sha256"]:
            raise RuntimeError("Existing initial states disagree.")
        initial_hashes[key] = r["initial_sha256"]
    total = args.seeds * len(cases) * 2
    for i, seed in enumerate(manifest["seeds"]):
        for j in range(len(cases)):
            scenario, shift = cases[(i+j) % len(cases)]
            variants = ("before", "after") if (i+j) % 2 == 0 else ("after", "before")
            for variant in variants:
                key = (variant, seed, scenario, shift)
                if key in done:
                    continue
                name = f"{scenario}-s{seed}-x{shift}-{variant}"
                initial = out / "initial.world"
                command = [str(executables[variant]), "--seed", str(seed), "--scenario", scenario,
                           "--shift", str(shift), "--ticks", str(args.ticks), "--threads", "4",
                           "--initial-out", str(initial)]
                start = time.monotonic()
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=300)
                (out / f"{name}.log").write_text(result.stdout + result.stderr, encoding="utf-8")
                if result.returncode:
                    raise RuntimeError(f"Experiment failed: {name}: {result.stderr}")
                lines = result.stdout.splitlines()
                samples = [line for line in lines if not line.startswith("LABEL,")]
                parsed = list(csv.DictReader(samples))
                labels = {int(line.split(",")[4]): line.split(",")[5] for line in lines if line.startswith("LABEL,")}
                digest = sha(initial)
                initial_key = (scenario, str(seed))
                if initial_key in initial_hashes and initial_hashes[initial_key] != digest:
                    raise RuntimeError(f"Different starting worlds: {name}")
                initial_hashes[initial_key] = digest
                if [int(r["tick"]) for r in parsed] != list(range(0, args.ticks + 1, 50)):
                    raise RuntimeError(f"Incomplete samples: {name}")
                for r in parsed:
                    r["variant"] = variant
                    r["left_pct"] = labels[int(r["tick"])]
                    r["initial_sha256"] = digest
                rows.extend(parsed)
                temporary = destination.with_suffix(destination.suffix + ".tmp")
                with temporary.open("w", newline="", encoding="utf-8") as stream:
                    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
                    writer.writeheader()
                    writer.writerows(rows)
                temporary.replace(destination)
                done.add(key)
                end = parsed[-1]
                print(f"{len(done)}/{total} {name}: bots={end['bots']} "
                      f"founders_eff={end['founder_effective']} brain_eff={end['brain_effective']} "
                      f"left={end['left_pct']} elapsed={time.monotonic()-start:.1f}s", flush=True)
    print("PASS: all runs completed, initial-world hashes match across versions/shifts.", flush=True)


if __name__ == "__main__":
    main()
