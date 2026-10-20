#!/usr/bin/env python3
"""Markdown tables of the systematics study's changes, from syst_report.py --changes-csv.

    ./summary.py changes.csv --base rec --samples s095 s098 s102 s105 [--names "0.95" ...]

Prints, for the given samples against --base: the changes over the whole sample (all), the change in bias per
2 deg band of the signed decay angle (side), and per true light-fragment Z within 4 deg of 90 (z-near). With
--slope X1 X2 ...: the input value of each sample, and the slope of the whole-sample bias change against it,
weighted by the errors (the base is at --slope-base).
"""
import argparse
import csv
import math


def load(path, base):
    rows = {}
    for r in csv.DictReader(open(path)):
        if r["base"] == base:
            rows[(r["sample"], r["binning"], r["cell"])] = r
    return rows


def pm(r, key, scale=1, spec="+.3f"):
    x, e = float(r[key]) * scale, float(r[key + "_err"]) * scale
    if math.isnan(x):
        return "-"
    return f"{format(x, spec)} ± {format(e, spec.lstrip('+'))}"


def table(header, rows):
    print("| " + " | ".join(header) + " |")
    print("|" + "|".join("---" for _ in header) + "|")
    for r in rows:
        print("| " + " | ".join(r) + " |")
    print()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("csv")
    p.add_argument("--base", required=True)
    p.add_argument("--samples", nargs="+", required=True)
    p.add_argument("--names", nargs="+", help="column names for the samples")
    p.add_argument("--slope", type=float, nargs="+", help="input value of each sample")
    p.add_argument("--slope-base", type=float, default=0, help="input value of the base")
    a = p.parse_args()
    rows = load(a.csv, a.base)
    names = a.names or a.samples

    table(["sample", "Δ correct (pp)", "Δ bias (Z)", "Δ off ≥ 3 (pp)", "Δ folding angle (deg)"],
          [[n, pm(rows[(s, "all", "all")], "correct", 100, "+.1f"), pm(rows[(s, "all", "all")], "bias"),
            pm(rows[(s, "all", "all")], "off3", 100, "+.1f"), pm(rows[(s, "all", "all")], "fold")]
           for s, n in zip(a.samples, names) if (s, "all", "all") in rows])

    for binning, title in (("side", "θ_CM − 90°"), ("z-near", "true Z (within 4° of 90°)")):
        cells = []
        for (s, b, c) in rows:
            if b == binning and s in a.samples and c not in cells:
                cells.append(c)
        print(f"Δ bias (Z) by {title}:\n")
        table([title] + names, [[c.replace(" near", "").replace("..", " to ")] +
                                [pm(rows[(s, binning, c)], "bias", 1, "+.2f") if (s, binning, c) in rows else "-"
                                 for s in a.samples] for c in cells])

    if a.slope:
        xs, ys, ws = [], [], []
        for s, x in zip(a.samples, a.slope):
            r = rows[(s, "all", "all")]
            xs.append(x - a.slope_base)
            ys.append(float(r["bias"]))
            ws.append(1 / float(r["bias_err"]) ** 2)
        # Line through the base (Δ = 0 at the base value)
        slope = sum(w * x * y for x, y, w in zip(xs, ys, ws)) / sum(w * x * x for x, w in zip(xs, ws))
        err = 1 / math.sqrt(sum(w * x * x for x, w in zip(xs, ws)))
        print(f"Slope of Δ bias: {slope:+.4f} ± {err:.4f} Z per unit of the input\n")


if __name__ == "__main__":
    main()
