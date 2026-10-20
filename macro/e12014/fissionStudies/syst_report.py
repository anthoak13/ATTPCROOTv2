#!/usr/bin/env python3
"""Accuracy of the fitted light-fragment Z per true Z, decay angle and vertex depth, for the systematics study.

    ./syst_report.py hpc-syst:syst_base:rec@truth_base.csv hpc-syst:syst_base:ref@truth_base.csv --base rec
    ./syst_report.py hpc-syst:syst_c4:mu931@c4_931.csv hpc-syst:syst_c4:mu939@c4_939.csv --base mu931 --by angle

Each sample is one [fit] section, [REMOTE:]STUDY:FIT, with @CSV naming sim_truth.C's output for the sim it
fits. Its label is FIT (STUDY:FIT if two samples share a name). Every event of the fit is used: dZ is the
unfolded error, true light-fragment Z minus the fitted Z of the same fragment (fragments matched by azimuth, as
tune_report.py --unfolded). Events are counted, per cell, as:

  no Y-pattern   the fitter found no Y-pattern and did not fit the event
  ambiguous      the two fragments are too close in azimuth to tell which fitted fragment is which
  unfittable     no simulation of the fit put charge on the pads with a data hit
  used           the rest; every dZ statistic is over these
  correct        dZ = 0;  within 1: |dZ| <= 1;  off by >= 3: |dZ| >= 3
  mean dZ within 2, spread   mean and standard deviation of dZ over the events with |dZ| <= 2
  mean dZ all    mean of dZ over every used event (the mean within 2 is pulled toward 0 when dZ widens)
  fold           mean folding angle of the Y-pattern (deg), from the fit's fragment directions

--base LABEL adds the change of each other sample against it, per cell. Two fits of the same sim (the same
@CSV) fit the same events, so their changes are paired, event by event, over the events used in both. Samples
from different sims are compared as independent samples (difference of the cell statistics), unless --paired:
then entries with the same chunk and number are paired too, for sims run with the same seeds whose events are
still in step (sim_truth.C's CSVs show whether they are). Errors are one standard error; for fractions,
binomial.

--by chooses the cells (repeatable): all (the whole sample, default), z (true light-fragment Z), angle
(|90 - true decay angle in the CoM frame|, --angle-edges), side (true angle - 90, signed: below 0 the heavy
fragment goes forward in the CoM frame), depth (true vertex depth from the window, --depth-edges), z-angle
(true Z within each angle band), dead (fraction of the fragments' pads that are dead, with --dead), z-near (true Z, events within --near degrees of 90 only). --csv DIR writes one
CSV per sample and binning; --changes-csv FILE appends the changes against --base, one row per sample and cell.

The events come from dump_fit.C through speed_report.py: run with the ATTPC environment loaded, after
fission.py STUDY --remote NAME --fetch.
"""
import argparse
import csv
import math
import os
import statistics
import sys
import tempfile

import speed_report as sr
import tune_report as tr

ZCN = 85
CORE = 2  # |dZ| <= CORE: the events the bias and spread are taken over
FAIL = 3  # |dZ| >= FAIL: off by 3 or more


def load(spec, args, tmp):
    """The events of sample spec ([REMOTE:]STUDY:FIT@CSV), unfolded against its truth, and its cost."""
    fitspec, _, truth_path = spec.partition("@")
    if not truth_path:
        sys.exit(f"{spec}: give the sim's truth as @CSV (sim_truth.C)")
    study, _, name = fitspec.rpartition(":")
    sr.dump(study, name, argparse.Namespace(dump_dir=args.dump_dir, reuse=args.reuse, results=False), tmp)
    remote, _, study_name = study.rpartition(":")
    path = os.path.join(args.dump_dir or tmp, f"{remote or 'local'}-{study_name}-{name}.csv".replace(":", "_"))
    num = lambda r, k, f=float: f(r[k]) if r.get(k) else None
    with open(path) as f:
        events = [{"file": r["file"], "entry": r["entry"], "fitted": r["fitted"] == "1", "zsim": num(r, "zsim", int),
                   "zfit": num(r, "zfit", int), "dz": num(r, "dz"), "obj": num(r, "obj"), "cut": r["cut"] == "1",
                   "thcm": num(r, "thcm"), "z0fit": num(r, "z0fit", int), "ph0": num(r, "ph0"), "ph1": num(r, "ph1"),
                   "vz": num(r, "vz"), "fold": num(r, "fold"), "vzfit": num(r, "vzfit"), "ebeam": num(r, "ebeam")}
                  for r in csv.DictReader(f)]
    cost = sr.Fit(name, study, events, argparse.Namespace(all=True))
    truth = tr.read_truth(truth_path)
    tr.unfold(events, truth, ZCN, False)
    for e in events:
        t = truth[(tr.chunk_of(e["file"]), int(e["entry"]))]
        e["zlight"] = min(t["z0"], ZCN - t["z0"])
        e["state"] = ("no Y-pattern" if not e["fitted"] else "unfittable" if tr.unfittable(e)
                      else "ambiguous" if e["dzu"] is None else "used")
        e["d"] = e["dzu"]
        e["chunk"], e["entry_n"] = tr.chunk_of(e["file"]), int(e["entry"])
    return {"label": name, "study": study_name, "truth": os.path.abspath(truth_path), "cost": cost,
            "events": {(tr.chunk_of(e["file"]), int(e["entry"])): e for e in events}}


def frac_err(k, n):
    if not n:
        return math.nan, math.nan
    p = k / n
    return p, math.sqrt(p * (1 - p) / n) if n > 1 else math.nan


def stats(events):
    """Cell statistics of a list of events."""
    used = [e for e in events if e["state"] == "used"]
    d = [e["d"] for e in used]
    n = len(d)
    core = [x for x in d if abs(x) <= CORE]
    s = {"n": len(events), "used": n}
    for state in ("no Y-pattern", "ambiguous", "unfittable"):
        s[state] = sum(1 for e in events if e["state"] == state)
    s["correct"] = frac_err(sum(1 for x in d if x == 0), n)
    s["within1"] = frac_err(sum(1 for x in d if abs(x) <= 1), n)
    s["off3"] = frac_err(sum(1 for x in d if abs(x) >= FAIL), n)
    s["bias"] = sr.mean_err(core) if core else (math.nan, math.nan)
    s["spread"] = statistics.stdev(core) if len(core) > 1 else math.nan
    s["meanall"] = sr.mean_err(d) if d else (math.nan, math.nan)
    folds = [e["fold"] for e in used if e["fold"] is not None]
    s["fold"] = sr.mean_err(folds) if folds else (math.nan, math.nan)
    return s


MEASURES = (  # (name, function of an event that is used, or None)
    ("correct", lambda e: float(e["d"] == 0)),
    ("off3", lambda e: float(abs(e["d"]) >= FAIL)),
    ("fold", lambda e: e["fold"]),
    ("meanall", lambda e: e["d"]),
)


def change(sample, base, keys_in_cell, paired):
    """Change of sample against base in one cell: {measure: (change, error)}, and the bias change."""
    if paired:
        both = [k for k in keys_in_cell if k in sample["events"] and sample["events"][k]["state"] == "used"
                and base["events"][k]["state"] == "used"]
        out = {}
        for name, f in MEASURES:
            out[name] = sr.mean_err([f(sample["events"][k]) - f(base["events"][k]) for k in both]) if both \
                else (math.nan, math.nan)
        core = [k for k in both if abs(sample["events"][k]["d"]) <= CORE and abs(base["events"][k]["d"]) <= CORE]
        out["bias"] = sr.mean_err([sample["events"][k]["d"] - base["events"][k]["d"] for k in core]) if core \
            else (math.nan, math.nan)
        out["pairs"] = len(both)
        return out
    return None  # Unpaired changes come from the two cells' statistics


def unpaired(s, b):
    out = {}
    for name in ("correct", "off3", "fold", "bias", "meanall"):
        (x, ex), (y, ey) = s[name], b[name]
        out[name] = (x - y, math.hypot(ex, ey))
    out["pairs"] = 0
    return out


def binning(kind, args):
    """[(label, test(event))] for a kind of cells."""
    if kind == "all":
        return [("all", lambda e: True)]
    if kind == "z":
        return [(f"Z={z}", lambda e, z=z: e["zlight"] == z) for z in range(31, 43)]
    if kind == "angle":
        edges = args.angle_edges
        return [(f"{lo:g}-{hi:g}", lambda e, lo=lo, hi=hi: lo <= abs(90 - e["thcm"]) < hi)
                for lo, hi in zip(edges, edges[1:])]
    if kind == "side":
        edges = sorted({-x for x in args.angle_edges} | set(args.angle_edges))
        return [(f"{lo:+g}..{hi:+g}", lambda e, lo=lo, hi=hi: lo <= e["thcm"] - 90 < hi)
                for lo, hi in zip(edges, edges[1:])]
    if kind == "depth":
        edges = args.depth_edges
        return [(f"{lo:g}-{hi:g}", lambda e, lo=lo, hi=hi: lo <= e["vz"] < hi) for lo, hi in zip(edges, edges[1:])]
    if kind == "dead":
        if not args.dead:
            sys.exit("--by dead needs --dead CSV (dead_cross.C)")
        return [(label, lambda e, lo=lo, hi=hi: (k := (e["chunk"], e["entry_n"])) in args.dead_frac
                 and lo <= args.dead_frac[k] < hi)
                for label, lo, hi in (("no dead pad", 0, 1e-9), ("dead < 10%", 1e-9, 0.1), ("dead >= 10%", 0.1, 2))]
    if kind == "z-near":
        return [(f"Z={z} near", lambda e, z=z: e["zlight"] == z and abs(90 - e["thcm"]) < args.near)
                for z in range(31, 43)]
    if kind == "z-angle":
        edges = args.angle_edges
        return [(f"{lo:g}-{hi:g} Z={z}", lambda e, lo=lo, hi=hi, z=z: lo <= abs(90 - e["thcm"]) < hi and e["zlight"] == z)
                for lo, hi in zip(edges, edges[1:]) for z in range(31, 43)]
    sys.exit(f"unknown --by {kind}")


def fmt(x, spec):
    return "-" if x is None or (isinstance(x, float) and math.isnan(x)) else format(x, spec)


def pm(v, spec, scale=1):
    x, e = v
    return f"{fmt(x * scale, spec)} ± {fmt(e * scale, spec.lstrip('+'))}"


def print_table(header, rows):
    widths = [max(len(r[i]) for r in rows + [header]) for i in range(len(header))]
    print("| " + " | ".join(h.ljust(w) for h, w in zip(header, widths)) + " |")
    print("|" + "|".join("-" * (w + 2) for w in widths) + "|")
    for r in rows:
        print("| " + " | ".join(c.ljust(w) for c, w in zip(r, widths)) + " |")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("samples", nargs="+", metavar="[REMOTE:]STUDY:FIT@TRUTH")
    parser.add_argument("--base", help="label of the sample the others are compared with")
    parser.add_argument("--paired", action="store_true", help="pair entries of different sims too (same seeds)")
    parser.add_argument("--by", action="append", default=[], help="all, z, angle, side, depth, z-angle (repeatable)")
    parser.add_argument("--angle-edges", type=float, nargs="+", default=[0, 2, 4, 6, 8, 10])
    parser.add_argument("--depth-edges", type=float, nargs="+", default=[0, 100, 200, 300, 400, 500, 600, 700])
    parser.add_argument("--dead", metavar="CSV", help="dead_cross.C's output: --by dead splits the events by the "
                        "fraction of their fragments' pads (nominal hits) that are dead")
    parser.add_argument("--near", type=float, default=4, help="z-near: events within this many degrees of 90")
    parser.add_argument("--csv", metavar="DIR", help="write one CSV per sample and binning here")
    parser.add_argument("--changes-csv", metavar="FILE", help="append the changes against --base to this CSV")
    parser.add_argument("--dump-dir", metavar="DIR", help="keep dump_fit.C's per-event CSVs here")
    parser.add_argument("--reuse", action="store_true", help="with --dump-dir: use dumps already there")
    args = parser.parse_args()
    args.dead_frac = {}
    if args.dead:
        with open(args.dead) as f:
            for r in csv.DictReader(f):
                pads = int(r["pads0"]) + int(r["pads1"])
                args.dead_frac[(r["chunk"], int(r["entry"]))] = (int(r["dead0"]) + int(r["dead1"])) / pads if pads else 0
    for d in (args.dump_dir, args.csv):
        if d:
            os.makedirs(d, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        samples = [load(s, args, tmp) for s in args.samples]
    labels = [s["label"] for s in samples]
    for s in samples:
        if labels.count(s["label"]) > 1:
            s["label"] = f"{s['study']}:{s['label']}"
    base = next((s for s in samples if s["label"] == args.base), None)
    if args.base and base is None:
        sys.exit(f"--base {args.base}: no such sample ({', '.join(s['label'] for s in samples)})")

    print("Cost (CPU-s per fitted event, fit jobs only):",
          ", ".join(f"{s['label']} {fmt(s['cost'].cpu_mean, '.1f')}" for s in samples))
    for kind in args.by or ["all"]:
        cells = binning(kind, args)
        print(f"\n## By {kind}\n")
        header = ["sample", "cell", "events", "no Y", "ambig", "unfit", "correct (%)", "within 1 (%)", "off ≥ 3 (%)",
                  "mean dZ within 2 (Z)", "spread (Z)", "mean dZ all (Z)", "fold (deg)"]
        rows, table = [], {}
        for s in samples:
            for label, test in cells:
                ev = [e for e in s["events"].values() if test(e)]
                st = stats(ev)
                table[(s["label"], label)] = (st, [k for k, e in s["events"].items() if test(e)])
                if not st["n"]:
                    continue
                rows.append([s["label"], label, str(st["n"]), str(st["no Y-pattern"]), str(st["ambiguous"]),
                             str(st["unfittable"]), pm(st["correct"], ".1f", 100), pm(st["within1"], ".1f", 100),
                             pm(st["off3"], ".1f", 100), pm(st["bias"], "+.3f"), fmt(st["spread"], ".2f"),
                             pm(st["meanall"], "+.3f"), pm(st["fold"], ".2f")])
        print_table(header, rows)
        if base:
            print(f"\nChange against {base['label']}\n")
            crows, cvals = [], []
            for s in samples:
                if s is base:
                    continue
                paired = s["truth"] == base["truth"] or args.paired
                for label, _ in cells:
                    st, keys = table[(s["label"], label)]
                    bst, bkeys = table[(base["label"], label)]
                    if not bst["n"]:
                        continue
                    c = change(s, base, bkeys, True) if paired else unpaired(st, bst)
                    cvals.append(((s["label"], s["study"], label), c))
                    crows.append([s["label"], label, "paired" if paired else "independent", str(c["pairs"] or st["used"]),
                                  pm(c["correct"], "+.1f", 100), pm(c["bias"], "+.3f"), pm(c["off3"], "+.1f", 100),
                                  pm(c["fold"], "+.3f"), pm(c["meanall"], "+.3f")])
            print_table(["sample", "cell", "comparison", "events", "Δ correct (pp)", "Δ mean dZ within 2 (Z)", "Δ off ≥ 3 (pp)",
                         "Δ fold (deg)", "Δ mean dZ all (Z)"], crows)
            if args.changes_csv:
                new = not os.path.exists(args.changes_csv)
                with open(args.changes_csv, "a", newline="") as out:
                    w = csv.writer(out)
                    if new:
                        w.writerow(["base", "sample", "study", "binning", "cell", "pairs", "correct", "correct_err",
                                    "bias", "bias_err", "off3", "off3_err", "fold", "fold_err", "meanall",
                                    "meanall_err"])
                    for row, c in cvals:
                        w.writerow([base["label"], row[0], row[1], kind, row[2], c["pairs"], *c["correct"], *c["bias"],
                                    *c["off3"], *c["fold"], *c["meanall"]])
        if args.csv:
            for s in samples:
                path = os.path.join(args.csv, f"{s['study']}-{s['label'].split(':')[-1]}-{kind}.csv")
                with open(path, "w", newline="") as out:
                    w = csv.writer(out)
                    w.writerow(["cell", "events", "no_y", "ambiguous", "unfittable", "used", "correct", "correct_err",
                                "within1", "within1_err", "off3", "off3_err", "bias", "bias_err", "spread", "fold",
                                "fold_err"])
                    for label, _ in cells:
                        st = table[(s["label"], label)][0]
                        if st["n"]:
                            w.writerow([label, st["n"], st["no Y-pattern"], st["ambiguous"], st["unfittable"], st["used"],
                                        *st["correct"], *st["within1"], *st["off3"], *st["bias"], st["spread"],
                                        *st["fold"]])


if __name__ == "__main__":
    main()
