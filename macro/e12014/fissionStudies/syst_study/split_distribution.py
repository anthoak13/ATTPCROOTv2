#!/usr/bin/env python3
"""The measured fragment-Z distribution of two fits of the same events, for the systematics study.

    ./split_distribution.py BASE_DUMP VARIATION_DUMP [--dead DEAD_CSV]

The dumps are dump_fit.C's per-event CSVs (syst_report.py --dump-dir); zfit is the lighter fitted Z, so the
split is measured as it would be on experimental data, with no matching of fitted to true fragments. Prints,
for the whole sample, the events within 4 deg of 90 deg and (with --dead, dead_cross.C's output) the events with
10% or more of their fragments' pads dead: the mean and standard deviation of the fitted light Z in both fits,
the fraction below the lightest true Z (31), and the change of the fitted light Z event by event; then the
histogram of the true and both fitted light Z. Run from fissionStudies/.
"""
import argparse
import csv
import math
import statistics
import sys

sys.path.insert(0, ".")
import tune_report as tr  # noqa: E402


def load(path):
    out = {}
    for r in csv.DictReader(open(path)):
        if r["fitted"] == "1" and r["zfit"]:
            out[(tr.chunk_of(r["file"]), int(r["entry"]))] = (int(r["zsim"]), int(r["zfit"]), float(r["thcm"]))
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("base")
    p.add_argument("variation")
    p.add_argument("--dead", help="dead_cross.C's CSV for the variation's dead pads")
    a = p.parse_args()
    base, var = load(a.base), load(a.variation)
    dead = {}
    if a.dead:
        for r in csv.DictReader(open(a.dead)):
            n = int(r["pads0"]) + int(r["pads1"])
            dead[(r["chunk"], int(r["entry"]))] = (int(r["dead0"]) + int(r["dead1"])) / n if n else 0
    keys = sorted(set(base) & set(var))
    cells = [("whole sample", lambda k: True), ("within 4 deg of 90", lambda k: abs(base[k][2] - 90) < 4)]
    if a.dead:
        cells.append(("10% or more of the fragments' pads dead", lambda k: dead.get(k, 0) >= 0.1))
    for label, sel in cells:
        ks = [k for k in keys if sel(k)]
        true = [base[k][0] for k in ks]
        print(f"{label}: {len(ks)} events; true light Z mean {statistics.fmean(true):.3f}, sd {statistics.stdev(true):.3f}")
        for name, ev in (("base", base), ("variation", var)):
            z = [ev[k][1] for k in ks]
            print(f"  {name:9s} fitted light Z mean {statistics.fmean(z):.3f} ± {statistics.stdev(z) / math.sqrt(len(z)):.3f},"
                  f" sd {statistics.stdev(z):.3f}, below 31: {100 * sum(x < 31 for x in z) / len(z):.1f}%")
        d = [var[k][1] - base[k][1] for k in ks]
        print(f"  change event by event: {statistics.fmean(d):+.3f} ± {statistics.stdev(d) / math.sqrt(len(d)):.3f}")
    print("light Z, true, base, variation")
    for z in range(min(v[1] for v in base.values()) - 1, 44):
        print(z, *(sum(1 for k in keys if s[k][i] == z) for s, i in ((base, 0), (base, 1), (var, 1))))


if __name__ == "__main__":
    main()
