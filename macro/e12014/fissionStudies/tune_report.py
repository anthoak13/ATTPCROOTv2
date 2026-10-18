#!/usr/bin/env python3
"""Physics against cost for fit settings: every [fit] section of a study fits the same events.

    ./tune_report.py hpc-tune:tune_fid --base base
    ./tune_report.py hpc-tune:tune_fid --base base --pair-ref base2 --by-z --csv out.csv

Each [fit] section is one row. For every row it prints the cost (CPU and wall seconds per fitted event,
from the jobs' .time files) and the light-fragment dZ = true - fitted Z against the simulated truth:
mean and width with their standard errors, and the fractions of events with |dZ| = 0, <= 1, <= 2 and >= 5.
Like speed_report.py, only events passing plot_fit's default cuts count (--all for every fitted event), and
"cut" is the fraction of fitted events that pass.

--base names the section the paired columns compare with, event by event, over the events both fits
fitted and both pass the cuts:
  d|dZ|   mean of |dZ_row| - |dZ_base| and its standard error; positive means the row fits worse
  same    fraction of events whose fitted Z is the same as in the base fit
  rms     RMS of Z_row - Z_base
A second fit of the base settings with another seed (--pair-ref, e.g. "base2") gives the floor of these
columns: how much two fits of the same events differ by chance alone.

--by-z adds a table of the dZ mean (and width) per true light-fragment Z for every row.

The events come from dump_fit.C through speed_report.py, so run it with the ATTPC environment loaded
(root on the PATH). Fetch the results and .time files first: fission.py STUDY --remote NAME --fetch.
"""
import argparse
import csv
import math
import os
import statistics
import sys
import tempfile

import speed_report as sr


def chunk_of(filename):
    """The chunk suffix (".c07" or "") of a fit or results file name, which identifies its input file."""
    base = sr.chunk_base(filename)
    tail = base.rsplit(".", 1)[-1]
    return tail if len(tail) == 3 and tail[0] == "c" and tail[1:].isdigit() else ""


class Row:
    """One [fit] section: its events (keyed by chunk and entry), cost and dZ statistics."""

    def __init__(self, spec, name, events, use_all):
        self.name = name
        args = argparse.Namespace(all=use_all)
        self.fit = sr.Fit(name, spec, events, args)  # Times and the plain dZ mean and width
        self.events = {}
        for e in events:
            if e["fitted"]:
                self.events[(chunk_of(e["file"]), int(e["entry"]))] = e
        self.used = [e for e in self.events.values() if use_all or e["cut"]]
        self.cut_frac = len([e for e in self.events.values() if e["cut"]]) / len(self.events) if self.events else math.nan
        dz = [abs(e["dz"]) for e in self.used]
        n = len(dz)
        self.frac = {k: (sum(1 for d in dz if f(d)) / n if n else math.nan)
                     for k, f in (("=0", lambda d: d == 0), ("<=1", lambda d: d <= 1), ("<=2", lambda d: d <= 2),
                                  (">=5", lambda d: d >= 5))}
        self.paired = None

    def pair(self, base, use_all):
        """Per-event comparison with the base row over the events both fitted and both pass the cuts."""
        keys = [k for k, e in self.events.items() if k in base.events and (use_all or (e["cut"] and base.events[k]["cut"]))]
        if not keys or base is self:
            self.paired = None
            return
        worse = [abs(self.events[k]["dz"]) - abs(base.events[k]["dz"]) for k in keys]
        diff = [self.events[k]["zfit"] - base.events[k]["zfit"] for k in keys]
        mean, err = sr.mean_err(worse)
        self.paired = {"n": len(keys), "worse": mean, "worse_err": err,
                       "same": sum(1 for d in diff if d == 0) / len(keys),
                       "rms": math.sqrt(sum(d * d for d in diff) / len(keys))}


def dump_events(spec, name, args, tmp):
    """dump_fit.C's events for one section, with entry, true and fitted Z (speed_report.dump drops those)."""
    path_args = argparse.Namespace(dump_dir=args.dump_dir, reuse=args.reuse, results=False)
    sr.dump(spec, name, path_args, tmp)  # Writes the CSV (or reuses it)
    remote, _, study = spec.rpartition(":")
    path = os.path.join(args.dump_dir or tmp, f"{remote or 'local'}-{study}-{name}.csv".replace(":", "_"))
    with open(path) as f:
        return [{"file": r["file"], "entry": r["entry"], "fitted": r["fitted"] == "1",
                 "zsim": int(r["zsim"]) if r["zsim"] else None, "zfit": int(r["zfit"]) if r["zfit"] else None,
                 "dz": float(r["dz"]) if r["dz"] else None, "obj": float(r["obj"]) if r["obj"] else None,
                 "cut": r["cut"] == "1"} for r in csv.DictReader(f)]


def fmt(x, spec):
    return "-" if x is None or (isinstance(x, float) and math.isnan(x)) else format(x, spec)


def print_table(header, rows):
    widths = [max(len(r[i]) for r in rows + [header]) for i in range(len(header))]
    for row in [header] + rows:
        print("  ".join(f"{c:<{w}}" if i == 0 else f"{c:>{w}}" for i, (c, w) in enumerate(zip(row, widths))))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("study", metavar="[REMOTE:]STUDY")
    parser.add_argument("--base", help="section the paired columns compare with (default: the first)")
    parser.add_argument("--pair-ref", metavar="NAME", help="a repeat of --base with another seed, listed first")
    parser.add_argument("--sections", nargs="+", metavar="NAME", help="only these sections, in this order")
    parser.add_argument("--all", action="store_true", help="every fitted event, not only those passing plot_fit's cuts")
    parser.add_argument("--by-z", action="store_true", help="also print the dZ mean per true light-fragment Z")
    parser.add_argument("--csv", metavar="FILE", help="also write the main table as CSV")
    parser.add_argument("--studies", default=os.path.join(sr.HERE, "studies"), metavar="DIR")
    parser.add_argument("--dump-dir", metavar="DIR", help="keep dump_fit.C's per-event CSVs here")
    parser.add_argument("--reuse", action="store_true", help="with --dump-dir: use dumps already there")
    args = parser.parse_args()
    if args.dump_dir:
        os.makedirs(args.dump_dir, exist_ok=True)

    names =sr.fit_names(args.studies, args.study.rpartition(":")[2])
    if args.sections:
        missing = [n for n in args.sections if n not in names]
        if missing:
            sys.exit(f"not [fit] sections of {args.study}: {' '.join(missing)}")
        names = args.sections
    base_name = args.base or names[0]
    if base_name not in names:
        names.insert(0, base_name)
    if args.pair_ref and args.pair_ref not in names:
        names.insert(1, args.pair_ref)

    with tempfile.TemporaryDirectory() as tmp:
        rows = [Row(args.study, n, dump_events(args.study, n, args, tmp), args.all) for n in names]
    base = next(r for r in rows if r.name == base_name)
    for r in rows:
        r.pair(base, args.all)
    if args.pair_ref:  # Floor first, right after the base
        rows.sort(key=lambda r: (r.name != base_name, r.name != args.pair_ref))

    header = ["fit", "events", "cut", "cpu-s/ev", "wall-s/ev", "x base", "dZ n", "dZ mean", "dZ sigma",
              "=0", "<=1", "<=2", ">=5", "pairs", "d|dZ|", "same", "rms"]
    table = []
    for r in rows:
        f, p = r.fit, r.paired
        table.append([r.name, str(len(r.events)), fmt(r.cut_frac, ".3f"),
                      "-" if math.isnan(f.cpu_mean) else f"{f.cpu_mean:.1f}", fmt(f.wall_mean, ".2f"),
                      fmt(base.fit.cpu_mean / f.cpu_mean if f.timed and base.fit.timed else math.nan, ".2f"),
                      str(f.dz_n), f"{f.dz_mean:+.2f} +- {f.dz_mean_err:.2f}", f"{f.dz_sigma:.2f} +- {f.dz_sigma_err:.2f}",
                      *[fmt(r.frac[k], ".3f") for k in ("=0", "<=1", "<=2", ">=5")],
                      str(p["n"]) if p else "-", f"{p['worse']:+.3f} +- {p['worse_err']:.3f}" if p else "-",
                      fmt(p["same"], ".3f") if p else "-", fmt(p["rms"], ".2f") if p else "-"])
    print_table(header, table)
    print(f"\nbase: {base_name}. cpu-s/ev is user+sys per fitted event; 'x base' is the base's CPU over the row's.")
    print("dZ = true - fitted light-fragment Z, " + ("all fitted events." if args.all else "events passing plot_fit's cuts."))

    if args.by_z:
        zs = sorted({e["zsim"] for r in rows for e in r.used})
        print("\ndZ mean (sigma) per true light-fragment Z\n")
        ztable = []
        for r in rows:
            cells = [r.name]
            for z in zs:
                dz = [e["dz"] for e in r.used if e["zsim"] == z]
                cells.append(f"{statistics.fmean(dz):+.2f} ({statistics.stdev(dz):.1f})" if len(dz) > 1 else "-")
            ztable.append(cells)
        counts = ["events"] + [str(sum(1 for e in base.used if e["zsim"] == z)) for z in zs]
        print_table(["fit"] + [f"Z={z}" for z in zs], [counts] + ztable)

    if args.csv:
        with open(args.csv, "w", newline="") as out:
            w = csv.writer(out)
            w.writerow(["fit", "fitted", "cut_frac", "cpu_s_per_event", "wall_s_per_event", "dz_n", "dz_mean",
                        "dz_mean_err", "dz_sigma", "dz_sigma_err", "f0", "f1", "f2", "f5", "pairs", "worse",
                        "worse_err", "same", "rms"])
            for r in rows:
                f, p = r.fit, r.paired or {}
                w.writerow([r.name, len(r.events), r.cut_frac, f.cpu_mean, f.wall_mean, f.dz_n, f.dz_mean, f.dz_mean_err,
                            f.dz_sigma, f.dz_sigma_err, r.frac["=0"], r.frac["<=1"], r.frac["<=2"], r.frac[">=5"],
                            p.get("n", ""), p.get("worse", ""), p.get("worse_err", ""), p.get("same", ""),
                            p.get("rms", "")])


if __name__ == "__main__":
    main()
