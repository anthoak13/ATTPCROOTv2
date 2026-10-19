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
  d bias  mean of dZ_row - dZ_base (the paired shift of the signed bias) and its standard error
  p(Z)    p-value of the chi2 of the per-true-Z paired bias shifts against no shift in any bin
  rms     RMS of Z_row - Z_base
  d core bias  paired shift of dZ over the events in the core (|dZ| <= 2) of both fits
  d fail       paired change of the failure rate (|dZ| >= 3)
  p(Z) is computed from the per-true-Z core shifts.
"unfit" is the fraction of fitted events no simulation of the fit could compare with (every simulation returned
the maximum objective, so the fitted Z is arbitrary); every dZ statistic except the plain "dZ mean"/"dZ sigma"
leaves them out, and "d unfit" is the paired change of that fraction. "d =0" is the paired change of the
fraction fit exactly right, and "Z worst" the largest |per-Z bias change| + 2 s.e. over the true-Z bins.
The row's own "core bias"/"core sigma" are the mean and width of its |dZ| <= 2 events and "fail" its rate of
|dZ| >= 3: a fit's dZ is a tight core plus a wide tail of failures, which one mean and width mix up.
A second fit of the base settings with another seed (--pair-ref, e.g. "base2") gives the floor of these
columns: how much two fits of the same events differ by chance alone.

--truth (sim_truth.C's CSV of the fits' sim files) matches each fitted fragment to its true one by azimuth
and adds "swap", the fraction of events whose fit gave the true light fragment the heavier Z. The folded dZ
cannot see a swap, and near symmetric splits it is biased by construction (the fitted light Z cannot exceed
Zcn/2); --unfolded makes every dZ statistic use the matched error instead (true light Z - fitted Z of that
same fragment), dropping the few events whose match is ambiguous.

--by-z adds a table of the dZ mean (and width) per true light-fragment Z for every row, and --by-angle one
per bin of |90 deg - true decay angle in the CoM frame| (the fit assumes 90 deg).

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


def chi2_sf(x, k):
    """P(chi2 with k degrees of freedom > x): the regularized upper incomplete gamma Q(k/2, x/2)."""
    a, z = k / 2, x / 2
    if z <= 0:
        return 1.0
    if z < a + 1:  # Series for P, then Q = 1 - P
        term = total = 1 / a
        n = a
        while abs(term) > 1e-15 * abs(total):
            n += 1
            term *= z / n
            total += term
        return max(0.0, 1 - total * math.exp(-z + a * math.log(z) - math.lgamma(a)))
    b, c, d = z + 1 - a, 1e300, 1 / (z + 1 - a)  # Continued fraction for Q (Lentz)
    h = d
    for i in range(1, 1000):
        an = -i * (i - a)
        b += 2
        d = an * d + b
        d = 1 / d if abs(d) > 1e-300 else 1e300
        c = b + an / c if abs(c) > 1e-300 else 1e300
        h *= d * c
        if abs(d * c - 1) < 1e-15:
            break
    return h * math.exp(-z + a * math.log(z) - math.lgamma(a))


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
        self.unfit = sum(1 for e in self.events.values() if unfittable(e)) / len(self.events) if self.events else math.nan
        self.used = [e for e in self.events.values() if (use_all or e["cut"]) and not unfittable(e)]
        self.cut_frac = len([e for e in self.events.values() if e["cut"]]) / len(self.events) if self.events else math.nan
        dz = [abs(e["dz"]) for e in self.used]
        n = len(dz)
        self.frac = {k: (sum(1 for d in dz if f(d)) / n if n else math.nan)
                     for k, f in (("=0", lambda d: d == 0), ("<=1", lambda d: d <= 1), ("<=2", lambda d: d <= 2),
                                  (">=5", lambda d: d >= 5))}
        core = [e["dz"] for e in self.used if abs(e["dz"]) <= CORE]
        self.core_n = len(core)
        self.core_mean, self.core_err = sr.mean_err(core) if core else (math.nan, math.nan)
        self.core_sigma = statistics.stdev(core) if len(core) > 1 else math.nan
        self.fail = sum(1 for e in self.used if abs(e["dz"]) >= FAIL) / n if n else math.nan
        matched = [e for e in self.used if e.get("swap") is not None]
        self.swap = sum(1 for e in matched if e["swap"]) / len(matched) if matched else math.nan
        self.paired = None

    def pair(self, base, use_all):
        """Per-event comparison with the base row over the events both fitted, both could fit and both pass the
        cuts; the change in the unfittable rate is over all the events both fitted."""
        common = [k for k in self.events if k in base.events]
        keys = [k for k in common if (use_all or (self.events[k]["cut"] and base.events[k]["cut"]))
                and not unfittable(self.events[k]) and not unfittable(base.events[k])]
        if not keys or base is self:
            self.paired = None
            return
        worse = [abs(self.events[k]["dz"]) - abs(base.events[k]["dz"]) for k in keys]
        diff = [self.events[k]["zfit"] - base.events[k]["zfit"] for k in keys]
        mean, err = sr.mean_err(worse)
        shift, shift_err = sr.mean_err([-d for d in diff])  # Paired change of the signed dZ (true - fit)
        in_core = [k for k in keys if abs(self.events[k]["dz"]) <= CORE and abs(base.events[k]["dz"]) <= CORE]
        core_shift, core_err = sr.mean_err([self.events[k]["dz"] - base.events[k]["dz"] for k in in_core]) \
            if in_core else (math.nan, math.nan)
        fail_d, fail_err = sr.mean_err([(abs(self.events[k]["dz"]) >= FAIL) - (abs(base.events[k]["dz"]) >= FAIL)
                                        for k in keys])
        exact_d, exact_err = sr.mean_err([(self.events[k]["dz"] == 0) - (base.events[k]["dz"] == 0) for k in keys])
        # Per true Z: paired bias shifts, combined into one chi2 against "no shift in any bin"; z_worst is the
        # largest |shift| + 2 s.e. of any bin, the far edge of its 95% interval
        chi2, bins, z_worst = 0.0, 0, 0.0
        for z in sorted({self.events[k]["zsim"] for k in in_core}):
            dz = [self.events[k]["dz"] - base.events[k]["dz"] for k in in_core if self.events[k]["zsim"] == z]
            m, e = sr.mean_err(dz)
            if len(dz) > 1:
                z_worst = max(z_worst, abs(m) + 2 * e)
                if e > 0:
                    chi2 += (m / e) ** 2
                    bins += 1
                elif m != 0:
                    chi2, bins = math.inf, bins + 1
        unfit_d, unfit_err = sr.mean_err([unfittable(self.events[k]) - unfittable(base.events[k]) for k in common])
        self.paired = {"n": len(keys), "worse": mean, "worse_err": err, "unfit_d": unfit_d, "unfit_d_err": unfit_err,
                       "exact_d": exact_d, "exact_d_err": exact_err, "z_worst": z_worst,
                       "same": sum(1 for d in diff if d == 0) / len(keys),
                       "rms": math.sqrt(sum(d * d for d in diff) / len(keys)),
                       "shift": shift, "shift_err": shift_err, "chi2": chi2, "bins": bins,
                       "core_shift": core_shift, "core_shift_err": core_err, "core_pairs": len(in_core),
                       "fail_d": fail_d, "fail_d_err": fail_err,
                       "p_z": chi2_sf(chi2, bins) if bins and chi2 < math.inf else (0.0 if bins else math.nan)}


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
                 "cut": r["cut"] == "1", "thcm": float(r["thcm"]) if r.get("thcm") else None,
                 "z0fit": int(r["z0fit"]) if r.get("z0fit") else None,
                 "ph0": float(r["ph0"]) if r.get("ph0") else None, "ph1": float(r["ph1"]) if r.get("ph1") else None}
                for r in csv.DictReader(f)]


UNFIT_OBJ = 1e30  # A best objective this large: no simulation of the event gave charge to compare (unfittable)


def unfittable(e):
    return e["obj"] is not None and e["obj"] >= UNFIT_OBJ


AMBIGUOUS = 0.5  # rad: matches whose two pairings differ by less than this in summed azimuth error are dropped


def unfold(events, truth, zcn, use_unfolded):
    """Match each fitted fragment to its true fragment by azimuth (sim_truth.C's CSV) and store the unfolded
    light-fragment error (true light Z - fitted Z of the same fragment) as "dzu" and whether the fit gave the
    true light fragment the heavier Z as "swap". With use_unfolded, "dz" becomes "dzu" and ambiguous
    matches count as not fitted."""

    def gap(a, b):
        return abs((a - b + math.pi) % (2 * math.pi) - math.pi)

    for e in events:
        e["dzu"] = e["swap"] = None
        if not e["fitted"] or e["z0fit"] is None:
            continue
        t = truth.get((chunk_of(e["file"]), int(e["entry"])))
        if t is None:
            sys.exit(f"no truth for {e['file']} entry {e['entry']} (--truth from the same sim?)")
        direct = gap(t["phi0"], e["ph0"]) + gap(t["phi1"], e["ph1"])
        swapped = gap(t["phi0"], e["ph1"]) + gap(t["phi1"], e["ph0"])
        if abs(direct - swapped) >= AMBIGUOUS:
            zfit0 = e["z0fit"] if direct < swapped else zcn - e["z0fit"]  # Fitted Z of true fragment 0
            light0 = t["z0"] < zcn - t["z0"]
            zlight, zfit_light = (t["z0"], zfit0) if light0 else (zcn - t["z0"], zcn - zfit0)
            e["dzu"] = zlight - zfit_light
            e["swap"] = zfit_light > zcn / 2
        if use_unfolded:
            if e["dzu"] is None:
                e["fitted"] = False
            else:
                e["dz"] = float(e["dzu"])


def read_truth(path):
    with open(path) as f:
        return {(r["chunk"], int(r["entry"])): {"z0": int(float(r["z0"])), "phi0": float(r["phi0"]),
                                                  "phi1": float(r["phi1"])} for r in csv.DictReader(f)}


CORE = 2  # |dZ| <= CORE: the core of a fit's dZ distribution, whose mean and width are the bias and resolution
FAIL = 3  # |dZ| >= FAIL: a failed fit (swaps included)
ANGLE_EDGES = [0, 15, 30, 45, 60, 90]  # Bins of |90 deg - true decay angle in the CoM frame|; the fit assumes 90


def angle_bin(e):
    """Label of the event's |90 - thCM| bin, or None if the dump has no angle."""
    if e["thcm"] is None:
        return None
    off = abs(90 - e["thcm"])
    for lo, hi in zip(ANGLE_EDGES, ANGLE_EDGES[1:]):
        if off < hi or hi == ANGLE_EDGES[-1]:
            return f"{lo}-{hi}"


def binned(rows, key, order):
    """{row name: {bin: (n, core mean, its error, core sigma, failure rate)}} of dZ, with the event's bin
    given by key(event). Core: |dZ| <= CORE; failure: |dZ| >= FAIL."""
    out = {}
    for r in rows:
        cells = {}
        for b in order:
            dz = [e["dz"] for e in r.used if key(e) == b]
            core = [d for d in dz if abs(d) <= CORE]
            mean, err = sr.mean_err(core) if core else (math.nan, math.nan)
            cells[b] = (len(dz), mean, err, statistics.stdev(core) if len(core) > 1 else math.nan,
                        sum(1 for d in dz if abs(d) >= FAIL) / len(dz) if dz else math.nan)
        out[r.name] = cells
    return out


def print_binned(title, rows, table, order, label):
    print(f"\n{title}\n")
    lines = [["events"] + [str(table[rows[0].name][b][0]) for b in order]]
    for r in rows:
        lines.append([r.name] + [f"{c[1]:+.2f} ({100 * c[4]:.0f}%)" if c[0] > 1 else "-"
                                 for c in (table[r.name][b] for b in order)])
    print_table(["fit"] + [label(b) for b in order], lines)


def write_binned(path, rows, table, order, column):
    with open(path, "w", newline="") as out:
        w = csv.writer(out)
        w.writerow(["fit", column, "n", "dz_mean", "dz_mean_err", "dz_sigma", "fail"])  # dZ: core (|dZ| <= 2)
        for r in rows:
            for b in order:
                n, mean, err, sigma, fail = table[r.name][b]
                if n:
                    w.writerow([r.name, b, n, mean, err, "" if math.isnan(sigma) else sigma, fail])


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
    parser.add_argument("--truth", metavar="CSV", help="sim_truth.C's CSV for the fits' input: match fragments, print swaps")
    parser.add_argument("--unfolded", action="store_true", help="with --truth: every dZ is the matched, unfolded error")
    parser.add_argument("--zcn", type=int, default=85, help="Z of the compound nucleus (default 85)")
    parser.add_argument("--all", action="store_true", help="every fitted event, not only those passing plot_fit's cuts")
    parser.add_argument("--by-z", action="store_true", help="also print the dZ mean per true light-fragment Z")
    parser.add_argument("--csv", metavar="FILE", help="also write the main table as CSV")
    parser.add_argument("--by-angle", action="store_true", help="also print the dZ mean per |90 - true decay angle|")
    parser.add_argument("--z-csv", metavar="FILE", help="also write the dZ mean and width per true Z as CSV")
    parser.add_argument("--angle-csv", metavar="FILE", help="also write the dZ mean and width per angle bin as CSV")
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
        truth = read_truth(args.truth) if args.truth else None
        rows = []
        for n in names:
            events = dump_events(args.study, n, args, tmp)
            if truth:
                unfold(events, truth, args.zcn, args.unfolded)
            rows.append(Row(args.study, n, events, args.all))
    base = next(r for r in rows if r.name == base_name)
    for r in rows:
        r.pair(base, args.all)
    if args.pair_ref:  # Floor first, right after the base
        rows.sort(key=lambda r: (r.name != base_name, r.name != args.pair_ref))

    header = ["fit", "events", "unfit", "cut", "cpu-s/ev", "x base", "dZ n", "core bias", "core sigma", "fail", "swap",
              "=0", "<=1", "dZ mean", "dZ sigma", "pairs", "d core bias", "d fail", "d unfit", "d =0", "Z worst", "p(Z)", "d|dZ|", "same",
              "rms"]
    table = []
    for r in rows:
        f, p = r.fit, r.paired
        table.append([r.name, str(len(r.events)), fmt(r.unfit, ".3f"), fmt(r.cut_frac, ".3f"),
                      "-" if math.isnan(f.cpu_mean) else f"{f.cpu_mean:.1f}",
                      fmt(base.fit.cpu_mean / f.cpu_mean if f.timed and base.fit.timed else math.nan, ".2f"),
                      str(f.dz_n), f"{r.core_mean:+.3f} +- {r.core_err:.3f}", fmt(r.core_sigma, ".3f"),
                      fmt(r.fail, ".3f"), fmt(r.swap, ".3f"), fmt(r.frac["=0"], ".3f"), fmt(r.frac["<=1"], ".3f"),
                      f"{f.dz_mean:+.2f} +- {f.dz_mean_err:.2f}", f"{f.dz_sigma:.2f}",
                      str(p["n"]) if p else "-",
                      f"{p['core_shift']:+.3f} +- {p['core_shift_err']:.3f}" if p else "-",
                      f"{p['fail_d']:+.4f} +- {p['fail_d_err']:.4f}" if p else "-",
                      f"{p['unfit_d']:+.4f} +- {p['unfit_d_err']:.4f}" if p else "-",
                      f"{p['exact_d']:+.4f} +- {p['exact_d_err']:.4f}" if p else "-",
                      fmt(p["z_worst"], ".3f") if p else "-",
                      fmt(p["p_z"], ".3f") if p else "-",
                      f"{p['worse']:+.3f} +- {p['worse_err']:.3f}" if p else "-",
                      fmt(p["same"], ".3f") if p else "-", fmt(p["rms"], ".2f") if p else "-"])
    print_table(header, table)
    print(f"\nbase: {base_name}. cpu-s/ev is user+sys per fitted event; 'x base' is the base's CPU over the row's.")
    print("dZ = true - fitted light-fragment Z" + (" of the same fragment (unfolded, fragments matched by azimuth; "
          "ambiguous matches dropped), " if args.unfolded else " (folded: the lighter of the fitted pair), ")
          + ("all fitted events." if args.all else "events passing plot_fit's cuts."))
    if args.truth:
        print("swap: fraction of matched events whose fit gave the true light fragment the heavier Z.")

    zs = sorted({e["zsim"] for r in rows for e in r.used})
    ztable = binned(rows, lambda e: e["zsim"], zs)
    angles = [f"{lo}-{hi}" for lo, hi in zip(ANGLE_EDGES, ANGLE_EDGES[1:])]
    atable = binned(rows, angle_bin, angles)
    if args.by_z:
        print_binned("core dZ mean (failure rate) per true light-fragment Z", rows, ztable, zs, lambda z: f"Z={z}")
    if args.by_angle:
        print_binned("core dZ mean (failure rate) per |90 - true thCM| (deg)", rows, atable, angles, str)
    if args.z_csv:
        write_binned(args.z_csv, rows, ztable, zs, "zsim")
    if args.angle_csv:
        write_binned(args.angle_csv, rows, atable, angles, "angle_bin")

    if args.csv:
        with open(args.csv, "w", newline="") as out:
            w = csv.writer(out)
            w.writerow(["fit", "fitted", "unfit", "cut_frac", "cpu_s_per_event", "wall_s_per_event", "dz_n", "dz_mean",
                        "dz_mean_err", "dz_sigma", "dz_sigma_err", "f0", "f1", "f2", "f5", "core_mean", "core_err",
                        "core_sigma", "fail", "swap", "pairs", "core_shift", "core_shift_err", "fail_d", "fail_d_err", "worse",
                        "worse_err", "shift", "shift_err", "chi2_z", "bins_z", "p_z", "same", "rms", "unfit_d", "unfit_d_err",
                        "exact_d", "exact_d_err", "z_worst"])
            for r in rows:
                f, p = r.fit, r.paired or {}
                w.writerow([r.name, len(r.events), r.unfit, r.cut_frac, f.cpu_mean, f.wall_mean, f.dz_n, f.dz_mean, f.dz_mean_err,
                            f.dz_sigma, f.dz_sigma_err, r.frac["=0"], r.frac["<=1"], r.frac["<=2"], r.frac[">=5"],
                            r.core_mean, r.core_err, r.core_sigma, r.fail, r.swap,
                            p.get("n", ""), p.get("core_shift", ""), p.get("core_shift_err", ""), p.get("fail_d", ""),
                            p.get("fail_d_err", ""), p.get("worse", ""), p.get("worse_err", ""), p.get("shift", ""),
                            p.get("shift_err", ""), p.get("chi2", ""), p.get("bins", ""), p.get("p_z", ""),
                            p.get("same", ""), p.get("rms", ""), p.get("unfit_d", ""), p.get("unfit_d_err", ""),
                            p.get("exact_d", ""), p.get("exact_d_err", ""), p.get("z_worst", "")])


if __name__ == "__main__":
    main()
