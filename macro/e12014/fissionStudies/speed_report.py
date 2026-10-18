#!/usr/bin/env python3
"""Compare the fits of code variants for the fitter speed study: cost per event, and a physics gate.

    ./speed_report.py --ref speed-ref:speed_fit_ref speed-v1:speed_fit_v1 speed-v2:speed_fit_v2

Each argument is a study, as plot_fit.C takes it: "<remote>:<study>" for results files fetched from a
cluster ($FISSION_DATA/remote/<remote>/, see fission.py --fetch), or "<study>" for the fit files made
here. Every [fit] section of the study's file in studies/ is one row. --ref is the reference study, and
its first [fit] section the reference sample every row is compared with (its second is the same fit
again, so its row is the run-to-run spread).

The events come from dump_fit.C (run with root, so the ATTPC environment must be loaded). The CPU and
wall times come from the <job>.time files next to the fit files, which fission.py writes in every job:
for a cluster, fetch them with --fetch (or --fetch-logs). Rows without them have "-" for times.

Gate: a row passes if, against the reference sample, its mean dZ and dZ width agree within --sigma
standard errors, its fraction of fitted events does too, and the two-sample Kolmogorov-Smirnov p-values
of the dZ and objective distributions are above --ks. dZ is true minus fitted Z of the lighter
fragment, and, like plot_fit.C's zHistDiff, only events passing plot_fit's default cuts count (--all
to count every fitted event). dZ takes few integer values, so the KS p-values are only approximate.

Python 3.8 or newer, standard library only.
"""
import argparse
import csv
import math
import os
import re
import statistics
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.abspath(os.environ.get("FISSION_DATA", os.path.join(HERE, "data")))
SIGMA_CUT = 2.0  # Largest difference from the reference, in standard errors, for the mean, width and fraction
KS_CUT = 0.05  # Smallest Kolmogorov-Smirnov p-value of the dZ and objective distributions
TIME_LINE = re.compile(r"fission-time: user=(\S+) sys=(\S+) wall=(\S+) maxrss_kb=(\S+)")
NO_OBJECTIVE = 1e300  # An unusable fit has objective DBL_MAX: its dZ counts, its objective is left out of the objective stats
MC_LINE = re.compile(r"Simulation of (\d+) events took (\d+) ms")  # fit.timeEvent = 1, once per round


def fit_names(studies_dir, study):
    """The [fit <name>] sections of studies/<study>.ini, in file order."""
    path = os.path.join(studies_dir, study + ".ini")
    if not os.path.isfile(path):
        sys.exit(f"No study file {path} (--studies DIR if it is elsewhere)")
    with open(path) as f:
        names = re.findall(r"^\[\s*fit\s+([A-Za-z0-9_-]+)\s*\]", f.read(), re.M)
    if not names:
        sys.exit(f"{path} has no [fit <name>] sections")
    return names


def dump(spec, name, args, tmp):
    """Run dump_fit.C for a fit and return its events: a list of dicts (file, fitted, dz, obj, cut)."""
    remote, _, study = spec.rpartition(":")
    path = os.path.join(args.dump_dir or tmp, f"{remote or 'local'}-{study}-{name}.csv".replace(":", "_"))
    if not (args.reuse and os.path.isfile(path)):
        macro = f'dump_fit.C("{spec}", "{name}", "{path}", {"true" if args.results else "false"})'
        try:
            done = subprocess.run(["root", "-l", "-b", "-q", macro], cwd=HERE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True)
        except FileNotFoundError:
            sys.exit("root not found: source build/config.sh first")
        if done.returncode != 0 or not os.path.isfile(path):
            sys.exit(f"{macro} failed:\n{done.stdout[-2000:]}")
        errors = [line for line in done.stdout.splitlines() if "Error in <FitFiles>" in line]
        if errors:
            sys.exit(f"{spec} {name}: {errors[0]}")
    with open(path) as f:
        return [{"file": r["file"], "fitted": r["fitted"] == "1",
                 "dz": float(r["dz"]) if r["dz"] else None, "obj": float(r["obj"]) if r["obj"] else None,
                 "cut": r["cut"] == "1"} for r in csv.DictReader(f)]


def data_dir(spec):
    remote, _, _ = spec.rpartition(":")
    return os.path.join(DATA, "remote", remote) if remote else DATA


def chunk_base(filename):
    """<study>-<name>.fit[.cNN] of a fit file or results file name."""
    return re.sub(r"(\.results)?\.root$", "", filename)


def read_time(path):
    """(user + sys CPU seconds, wall seconds, max RSS in MB or None) of a .time file, or None if missing."""
    try:
        with open(path) as f:
            match = TIME_LINE.search(f.read())
    except OSError:
        return None
    if not match:
        return None
    user, sys_, wall, rss = match.groups()
    return float(user) + float(sys_), float(wall), (float(rss) / 1024 if float(rss) > 0 else None)


def read_mc_seconds(path):
    """Seconds the fit's simulation rounds took, summed from a .log (fit.timeEvent = 1), or None."""
    try:
        with open(path, errors="replace") as f:  # Line by line: a log is about 1 MB per event
            ms = [int(m[2]) for m in map(MC_LINE.search, f) if m]
    except OSError:
        return None
    return sum(ms) / 1000 if ms else None


def mean_err(values):
    n = len(values)
    if n < 2:
        return (values[0] if n else float("nan")), float("nan")
    return statistics.fmean(values), statistics.stdev(values) / math.sqrt(n)


def sigma_err(values):
    """Sample standard deviation and its standard error (sigma / sqrt(2 (n - 1)), for a normal distribution)."""
    n = len(values)
    if n < 2:
        return float("nan"), float("nan")
    s = statistics.stdev(values)
    return s, s / math.sqrt(2 * (n - 1))


def ks_test(a, b):
    """Two-sample Kolmogorov-Smirnov test: (D, p). p is the asymptotic probability with Stephens'
    finite-size correction, so it is approximate for small samples and conservative for ties."""
    if not a or not b:
        return float("nan"), float("nan")
    a, b = sorted(a), sorted(b)
    na, nb = len(a), len(b)
    i = j = 0
    d = 0.0
    while i < na and j < nb:
        x = min(a[i], b[j])
        while i < na and a[i] == x:  # Step past ties in both samples before comparing the CDFs
            i += 1
        while j < nb and b[j] == x:
            j += 1
        d = max(d, abs(i / na - j / nb))
    ne = na * nb / (na + nb)
    lam = (math.sqrt(ne) + 0.12 + 0.11 / math.sqrt(ne)) * d
    if lam < 0.2:
        return d, 1.0
    p = 2 * sum((-1) ** (k - 1) * math.exp(-2 * k * k * lam * lam) for k in range(1, 101))
    return d, min(1.0, max(0.0, p))


class Fit:
    """One [fit] section's events and times."""

    def __init__(self, label, spec, events, args):
        self.label = label
        self.events = len(events)
        fitted = [e for e in events if e["fitted"]]
        self.fitted = len(fitted)
        used = fitted if args.all else [e for e in fitted if e["cut"]]
        self.dz = [e["dz"] for e in used]
        self.obj = [e["obj"] for e in used if e["obj"] < NO_OBJECTIVE]
        self.dz_n = len(self.dz)
        self.dz_mean, self.dz_mean_err = mean_err(self.dz)
        self.dz_sigma, self.dz_sigma_err = sigma_err(self.dz)
        self.obj_median = statistics.median(self.obj) if self.obj else float("nan")
        self.obj_mean = statistics.fmean(self.obj) if self.obj else float("nan")
        self.fraction = self.fitted / self.events if self.events else float("nan")

        # Times, per chunk: fission.py's .time file next to the fit file, fitted events from the dump
        directory = data_dir(spec)
        per_chunk = {}
        for e in events:
            per_chunk[chunk_base(e["file"])] = per_chunk.get(chunk_base(e["file"]), 0) + e["fitted"]
        self.chunks = len(per_chunk)
        times = {base: read_time(os.path.join(directory, base + ".time")) for base in per_chunk}
        complete = [base for base in per_chunk if times[base] and per_chunk[base]]
        self.timed = len(complete) == len(per_chunk) and bool(complete)
        self.cpu_total = sum(times[b][0] for b in complete)
        self.wall_total = sum(times[b][1] for b in complete)
        self.events_timed = sum(per_chunk[b] for b in complete)
        self.cpu_chunks = [times[b][0] / per_chunk[b] for b in complete]
        self.rss_mb = max([times[b][2] for b in complete if times[b][2]], default=None)
        mc = [read_mc_seconds(os.path.join(directory, base + ".log")) for base in per_chunk]
        self.mc_total = sum(mc) if mc and all(m is not None for m in mc) and self.fitted else None
        self.cpu_mean = self.cpu_total / self.events_timed if self.timed and self.events_timed else float("nan")
        self.cpu_spread = statistics.stdev(self.cpu_chunks) if len(self.cpu_chunks) > 1 else float("nan")
        self.wall_mean = self.wall_total / self.events_timed if self.timed and self.events_timed else float("nan")
        self.mc_mean = self.mc_total / self.fitted if self.mc_total is not None else float("nan")
        self.speedup = float("nan")
        self.ks_dz = self.ks_obj = float("nan")
        self.verdict = "ref"
        self.reasons = []

    def compare(self, ref, args):
        """Fill the KS p-values and the gate against the reference sample ref."""
        self.ks_dz = ks_test(self.dz, ref.dz)[1]
        self.ks_obj = ks_test(self.obj, ref.obj)[1]
        problems = []

        def apart(name, x, ex, y, ey):
            sigma = math.hypot(ex, ey)
            if math.isnan(sigma) or sigma == 0:
                if x != y:
                    problems.append(f"{name} {x:.3g} vs {y:.3g} (no error)")
            elif abs(x - y) > args.sigma * sigma:
                problems.append(f"{name} {x:.3g} vs {y:.3g} ({abs(x - y) / sigma:.1f} sigma)")

        apart("dZ mean", self.dz_mean, self.dz_mean_err, ref.dz_mean, ref.dz_mean_err)
        apart("dZ sigma", self.dz_sigma, self.dz_sigma_err, ref.dz_sigma, ref.dz_sigma_err)
        err = lambda f: math.sqrt(f.fraction * (1 - f.fraction) / f.events) if f.events else float("nan")
        apart("fitted fraction", self.fraction, err(self), ref.fraction, err(ref))
        for name, p in (("dZ KS", self.ks_dz), ("objective KS", self.ks_obj)):
            if not p > args.ks:  # Also true for nan
                problems.append(f"{name} p={p:.3f}")
        self.reasons = problems
        self.verdict = "FAIL" if problems else "PASS"


COLUMNS = [  # (header, key, formatter)
    ("fit", "label", str),
    ("events", "events", "{:d}"),
    ("fitted", "fitted", "{:d}"),
    ("frac", "fraction", "{:.3f}"),
    ("cpu-s/ev", "cpu", None),
    ("wall-s/ev", "wall_mean", "{:.2f}"),
    ("mc-s/ev", "mc_mean", "{:.2f}"),
    ("speedup", "speedup", "{:.2f}"),
    ("dZ n", "dz_n", "{:d}"),
    ("dZ mean", "dzm", None),
    ("dZ sigma", "dzs", None),
    ("obj med", "obj_median", "{:.2f}"),
    ("obj mean", "obj_mean", "{:.2f}"),
    ("KS dZ", "ks_dz", "{:.3f}"),
    ("KS obj", "ks_obj", "{:.3f}"),
    ("gate", "verdict", str),
]


def cell(fit, key, fmt):
    if key == "cpu":
        return "-" if math.isnan(fit.cpu_mean) else f"{fit.cpu_mean:.2f} +- {fit.cpu_spread:.2f}"
    if key == "dzm":
        return f"{fit.dz_mean:+.3f} +- {fit.dz_mean_err:.3f}"
    if key == "dzs":
        return f"{fit.dz_sigma:.3f} +- {fit.dz_sigma_err:.3f}"
    value = getattr(fit, key)
    if isinstance(value, float) and math.isnan(value):
        return "-"
    return fmt(value) if fmt is str else fmt.format(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("variants", nargs="*", metavar="[REMOTE:]STUDY", help="studies to compare with --ref")
    parser.add_argument("--ref", required=True, metavar="[REMOTE:]STUDY", help="the reference study")
    parser.add_argument("--ref-fit", metavar="NAME", help="the reference sample (default: --ref's first [fit])")
    parser.add_argument("--sigma", type=float, default=SIGMA_CUT,
                        help=f"gate: largest difference in standard errors (default {SIGMA_CUT})")
    parser.add_argument("--ks", type=float, default=KS_CUT, help=f"gate: smallest KS p-value (default {KS_CUT})")
    parser.add_argument("--all", action="store_true",
                        help="dZ and objective of every fitted event, not only those passing plot_fit's cuts")
    parser.add_argument("--results", action="store_true", help="read a local study's results files, not its fit files")
    parser.add_argument("--csv", metavar="FILE", help="also write the table as CSV")
    parser.add_argument("--studies", default=os.path.join(HERE, "studies"), metavar="DIR",
                        help="where the studies' .ini files are (default: studies/ next to this script)")
    parser.add_argument("--dump-dir", metavar="DIR", help="keep dump_fit.C's per-event CSVs here")
    parser.add_argument("--reuse", action="store_true", help="with --dump-dir: use dumps already there")
    args = parser.parse_args()

    fits, ref = [], None
    with tempfile.TemporaryDirectory() as tmp:
        for spec in [args.ref] + args.variants:
            names = fit_names(args.studies, spec.rpartition(":")[2])
            for name in names:
                label = f"{spec}.{name}"
                fits.append(Fit(label, spec, dump(spec, name, args, tmp), args))
                if spec == args.ref and name == (args.ref_fit or names[0]):
                    ref = fits[-1]
    if ref is None:
        sys.exit(f"--ref-fit {args.ref_fit} is not a [fit] section of {args.ref}")

    ref_fits = [f for f in fits if f.label.startswith(args.ref + ".") and f.timed]
    ref_events = sum(f.events_timed for f in ref_fits)
    ref_cpu = sum(f.cpu_total for f in ref_fits) / ref_events if ref_events else float("nan")
    for fit in fits:
        fit.speedup = ref_cpu / fit.cpu_mean if fit.timed else float("nan")
        if fit is not ref:
            fit.compare(ref, args)

    header = [c[0] for c in COLUMNS]
    rows = [[cell(f, k, fmt) for _, k, fmt in COLUMNS] for f in fits]
    widths = [max(len(r[i]) for r in rows + [header]) for i in range(len(header))]
    for row in [header] + rows:
        print("  ".join(f"{c:<{w}}" if i == 0 else f"{c:>{w}}" for i, (c, w) in enumerate(zip(row, widths))))
    print(f"\nreference sample: {ref.label}; speedup is the CPU-s/event of all of {args.ref}'s fits over the row's")
    for fit in fits:
        if fit.reasons:
            print(f"{fit.label}: " + "; ".join(fit.reasons))
    untimed = [f.label for f in fits if not f.timed]
    if untimed:
        print("no complete .time files for: " + ", ".join(untimed) + " (they are next to the fit files: fetch them with"
              " fission.py STUDY --remote NAME --fetch)")

    if args.csv:
        with open(args.csv, "w", newline="") as f:
            out = csv.writer(f)
            out.writerow(["fit", "events", "fitted", "fraction", "cpu_s_per_event", "cpu_spread", "wall_s_per_event",
                          "mc_s_per_event", "speedup", "dz_n", "dz_mean", "dz_mean_err", "dz_sigma", "dz_sigma_err",
                          "obj_median", "obj_mean", "ks_p_dz", "ks_p_obj", "max_rss_mb", "gate", "reasons"])
            for fit in fits:
                out.writerow([fit.label, fit.events, fit.fitted, fit.fraction, fit.cpu_mean, fit.cpu_spread,
                              fit.wall_mean, fit.mc_mean, fit.speedup, fit.dz_n, fit.dz_mean, fit.dz_mean_err, fit.dz_sigma,
                              fit.dz_sigma_err, fit.obj_median, fit.obj_mean, fit.ks_dz, fit.ks_obj,
                              fit.rss_mb if fit.rss_mb is not None else "", fit.verdict, "; ".join(fit.reasons)])


if __name__ == "__main__":
    main()
