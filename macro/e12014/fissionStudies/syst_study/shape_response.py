#!/usr/bin/env python3
"""How a variation changes a measured fragment-Z distribution of a given shape, for the systematics study.

    ./shape_response.py DUMP_DIR [--boot N]

The baseline sample has about 250 events at each true light Z from 31 to 42. Weighting its events by
shape(Z) / (events at that Z) turns it into a sample with that true distribution; the same weights on another fit
of the same events (a variation that digitizes the baseline's simulation again) give that fit's measured
distribution. The measured light Z is the lighter fitted Z (the folded split). Prints, for each shape (two
Gaussians for asymmetric fission, one at 42.5 for symmetric fission) and variation: the mean and standard
deviation of the measured light Z, the height at Z = 42 (next to the symmetric split, the valley of an
asymmetric distribution) relative to the highest bin, and the fraction below 31; then the changes against the
baseline's fit with bootstrap errors (events resampled with both fits together). DUMP_DIR is syst_report.py's
--dump-dir. Run from fissionStudies/.
"""
import argparse
import csv
import math
import os
import random
import statistics
import sys

sys.path.insert(0, ".")
import tune_report as tr  # noqa: E402

FITS = {"every pad working": "hpc-syst-syst_base-rec.csv", "dead AsAd": "hpc-syst3-syst_pads-asad.csv",
        "dead AsAd + 10% in patches": "hpc-syst3-syst_pads-asad_p10.csv",
        "gain per AsAd, sigma 10%": "hpc-syst3-syst_pads-asad10.csv", "noise 5 ADC": "hpc-syst3-syst_pads-noise5.csv"}
gauss = lambda z, m, s: math.exp(-0.5 * ((z - m) / s) ** 2)
SHAPES = {"asymmetric: peaks at Z 36 and 49, sigma 2": lambda z: gauss(z, 36, 2) + gauss(85 - z, 36, 2),
          "asymmetric: peaks at Z 34 and 51, sigma 1.5": lambda z: gauss(z, 34, 1.5) + gauss(85 - z, 34, 1.5),
          "symmetric: one peak at 42.5, sigma 3": lambda z: gauss(z, 42.5, 3)}


def load(path):
    out = {}
    for r in csv.DictReader(open(path)):
        if r["fitted"] == "1" and r["zfit"]:
            out[(tr.chunk_of(r["file"]), int(r["entry"]))] = (int(r["zsim"]), int(r["zfit"]))
    return out


def measures(keys, fit, true, w):
    """Mean and sd of the measured light Z, height at 42 over the highest bin, fraction below 31."""
    W = sum(w[true[k][0]] for k in keys)
    m = sum(fit[k][1] * w[true[k][0]] for k in keys) / W
    sd = math.sqrt(sum(w[true[k][0]] * (fit[k][1] - m) ** 2 for k in keys) / W)
    h = {}
    for k in keys:
        h[fit[k][1]] = h.get(fit[k][1], 0) + w[true[k][0]]
    return m, sd, h.get(42, 0) / max(h.values()), sum(x for z, x in h.items() if z < 31) / W


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("dumps")
    p.add_argument("--boot", type=int, default=200, help="bootstrap resamples")
    a = p.parse_args()
    fits = {name: load(os.path.join(a.dumps, f)) for name, f in FITS.items()}
    base = fits["every pad working"]
    keys = sorted(set.intersection(*(set(f) for f in fits.values())))
    count = {z: sum(1 for k in keys if base[k][0] == z) for z in range(31, 43)}
    random.seed(1)
    for name, shape in SHAPES.items():
        w = {z: shape(z) / count[z] for z in range(31, 43)}
        true = {k: (base[k][0], base[k][0]) for k in keys}
        m, sd, valley, tail = measures(keys, true, base, w)
        print(f"== {name}\n  true: mean {m:.2f}, sd {sd:.2f}, Z42/peak {valley:.2f}")
        for fname, fit in fits.items():
            m, sd, valley, tail = measures(keys, fit, base, w)
            print(f"  {fname:28s} mean {m:.2f}, sd {sd:.2f}, Z42/peak {valley:.2f}, below 31 {100 * tail:.1f}%")
        for fname, fit in fits.items():
            if fit is base:
                continue
            d = []
            for _ in range(a.boot):
                ks = [random.choice(keys) for _ in keys]
                x, y = measures(ks, base, base, w), measures(ks, fit, base, w)
                d.append([y[i] - x[i] for i in range(4)])
            mu = [statistics.fmean(r[i] for r in d) for i in range(4)]
            se = [statistics.stdev(r[i] for r in d) for i in range(4)]
            print(f"  change, {fname:28s} mean {mu[0]:+.3f} ± {se[0]:.3f}, sd {mu[1]:+.3f} ± {se[1]:.3f}, "
                  f"Z42/peak {mu[2]:+.3f} ± {se[2]:.3f}, below 31 {100 * mu[3]:+.2f} ± {100 * se[3]:.2f}%")


if __name__ == "__main__":
    main()
