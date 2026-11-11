"""Analysis sanity checks with synthetic data, isolated from study results."""
import csv
import importlib.util
import math
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "x64/ecology/analysis-tests"
FIELDS = ("seed scenario shift tick bots apples organics mean_energy mean_age low_energy_pct "
          "brain_richness brain_effective founder_richness founder_effective ps_pct predator_pct "
          "organic_pct mixed_pct unknown_pct births deaths rebalances crisis_removed min_bots "
          "extinction_tick pre_crisis_bots recovery_ticks variant left_pct initial_sha256").split()


def main():
    spec = importlib.util.spec_from_file_location("analysis", ROOT / "tests/Analyze-CB3Ecology.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    assert module.bootstrap([0]*20) == (0,0,0)
    positive = module.bootstrap(list(range(1,21)))
    negative = module.bootstrap(list(range(-1,-21,-1)))
    assert positive[0] == 10.5 and positive[1] < positive[0] < positive[2]
    assert math.isclose(positive[1], -negative[2]) and math.isclose(positive[2], -negative[1])
    lo, hi = module.wilson(0,20)
    assert lo == 0 and .16 < hi < .17
    assert math.isclose(module.wilson(20,20)[0], 1-hi)
    OUT.mkdir(parents=True, exist_ok=True)
    for delta in (0,40):
        rows = []
        for seed in (1009+7919*i for i in range(20)):
            for case, shift in (("normal",0),("cluster",0),("scarce",0),("crisis",0),
                                ("normal",359),("neutral",0),("neutral",359)):
                for variant in ("before","after"):
                    for tick in range(0,601,50):
                        r = dict.fromkeys(FIELDS, "0")
                        r.update(seed=seed, scenario=case, shift=shift, variant=variant, tick=tick, bots=100+tick,
                                 mean_energy=200+(delta if variant=="after" else 0), mean_age=50, low_energy_pct=10,
                                 brain_richness=1 if case=="neutral" else 2, brain_effective=1 if case=="neutral" else 2,
                                 founder_richness=2, founder_effective=2, ps_pct=95, unknown_pct=5, births=tick,
                                 min_bots=100, extinction_tick=-1, pre_crisis_bots=350 if case=="crisis" and tick>250 else 0,
                                 recovery_ticks=20 if case=="crisis" and tick>=400 else -1,
                                 left_pct=50 if case=="neutral" else "nan",
                                 initial_sha256=f"synthetic-{'normal' if case=='crisis' else case}-{seed}")
                        rows.append(r)
        source = OUT / f"synthetic-{delta}.csv"
        with source.open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=FIELDS)
            writer.writeheader(); writer.writerows(rows)
        output = OUT / f"result-{delta}"
        run = subprocess.run([sys.executable,str(ROOT/"tests/Analyze-CB3Ecology.py"),"--csv",str(source),
                              "--output-dir",str(output)], cwd=ROOT, capture_output=True, text=True)
        assert run.returncode == 0, run.stderr
        summary = list(csv.DictReader((output/"CB3_ECOLOGY_SUMMARY.csv").open()))
        if delta == 0:
            assert all(r["status"] in ("equivalent", "not_observed") for r in summary)
            assert any(r["metric"] == "organic_pct" and r["status"] == "not_observed" for r in summary)
        else:
            energy = [r for r in summary if r["metric"] == "mean_energy"]
            assert len(energy) == 7 and all(r["status"] == "outside" for r in energy)
        print("PASS synthetic full pipeline:", delta, "energy shift,",len(summary),"comparisons")
    # Repeated snapshots must not silently inflate the independent sample size.
    with source.open("a",newline="") as stream:
        csv.DictWriter(stream,fieldnames=FIELDS).writerow(rows[-1])
    run = subprocess.run([sys.executable,str(ROOT/"tests/Analyze-CB3Ecology.py"),"--csv",str(source),
                          "--output-dir",str(OUT/"duplicate")],cwd=ROOT,capture_output=True,text=True)
    assert run.returncode != 0
    print("PASS duplicate-snapshot rejection and Wilson uncertainty for zero events")


if __name__ == "__main__":
    main()
