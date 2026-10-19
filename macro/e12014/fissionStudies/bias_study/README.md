# Why the MC fission fit's fragment Z is biased

The Monte Carlo fission fit (`run_fit.C`, `AtMCFission`) of the 90° pseudo-data gives the light fragment a Z
that is on average 0.40 too low, more so for asymmetric splits (+0.82 at Z = 31, +0.08 at Z = 42). This
report finds where that comes from.

## Result

**The bias comes from a bug in how the pseudo-data is digitized, not from the fit.** `response.h`, the
measured electronics response used by `run_digi_fiss.C`, is a trace sampled once per time bucket (320 ns),
but it is looked up with the time in microseconds. So every simulated pulse in the pseudo-data is 3.1 times
too long and starts late. The pulse-shape analysis then deconvolves the traces with the true-width response,
which leaves the charge smeared and shifted by about 8 time buckets (20 mm of drift). The fit makes its own
hits straight from the simulated charge, so its picture of an event never matches the pseudo-data's, and the
closest match is at a lower light-fragment Z.

Re-digitizing the same simulated events with the response read per time bucket, and fitting them with
today's fit, removes the bias:

| pseudo-data (same 200 events) | bias | spread | exact Z | off by 3 or more | unfittable |
|---|---|---|---|---|---|
| as made | +0.46 ± 0.04 | 0.54 | 53% | 0.5% | 5% |
| response read per time bucket | +0.03 ± 0.02 | 0.22 | 92% | 4.5% | 0 |
| hits made as the fit makes them | +0.03 ± 0.01 | 0.20 | 96% | 1.5% | 0 |

Bias is the mean of true − fitted light Z over fits within 2, spread their standard deviation. "Unfittable"
events are those where no simulation of the fit produced charge to compare (below); the other columns are
over the rest.

FULL_STATS_PLACEHOLDER

## Evidence

### The bias is in the objective, not in the search

The fit's only free parameter is the fragment Z: the fragment directions and the vertex are fixed to the
Y-pattern found in the event (their sampling width is zero). So the 200 simulations the fit runs per event
are repeated draws of the objective at a few Z values, and averaging them per Z gives the event's mean
objective as a function of Z. On today's fit of the 3000 events:

- the Z with the lowest single draw (what the fit returns) has a bias of +0.401 ± 0.010;
- the Z with the lowest mean objective has +0.399 ± 0.011;
- the minimum of a parabola through the mean objective at the three Z around it, +0.401 ± 0.008.

They agree for every true Z as well (figure `search_vs_model.png`). The mean objective, relative to its value
at the true Z, rises by only 0.14 one unit below the true light Z and by 1.42 one unit above it
(`landscape.png`): its minimum sits 0.4 below the truth. The noise of single draws (0.6–0.8) is similar on
both sides. So the search finds the minimum it is given; the minimum itself is in the wrong place. This also
explains the search-length table: shorter searches sample mostly near the starting Z (42), which pulls the
answer toward symmetric splits and partly hides the bias (+0.17 at 40 × 1); once the search reaches the
minimum the bias settles at +0.40.

The same holds for every other fit setting looked at (step 0.5 mm, 10 diffusion points, no gain
fluctuations, fixed diffusion patterns of 10 and 300 points): the lowest-draw and the mean-objective biases
agree within 0.06. The extra bias with 10 diffusion points (+0.65) is also a shift of the mean objective, not
noise.

### The pseudo-data's vertex is 20 mm off

The Y-pattern vertex found in the pseudo-data is 19.7 mm farther from the pad plane than the simulated vertex,
at every drift distance (median offset −19.6 to −19.8 mm in each 100 mm slice of vertex position, spread
1–6 mm). A shift that does not depend on drift distance is a timing offset in the pulses, not a drift
velocity error. Re-digitized with the response read per time bucket, the offset is +1.8 mm (spread 2.4 mm);
with hits made from the simulated charge, +1.7 mm (`vertex.png`).

### The response is stretched 3.1 times

`AtPulse` passes the response function the time since the charge arrived, in microseconds, as the framework's
`AtVectorResponse` expects (it divides by the bucket width). `Response::GetResponse` instead uses the time
directly as an index into the per-bucket trace, so bucket *i* of the measured response is used for a time of
*i* µs, i.e. 3.1 buckets. The measured response peaks 2 buckets after the charge; stretched, 6–9 buckets
after. One pad of one event (`trace.png`): the pulse made as before peaks 10 buckets after the charge
arriving on the pad and is wider; read per time bucket it peaks 4 buckets after, as the measured response
does. The expected leftover shift after deconvolving with the unstretched response is the difference of the
two responses' centroids, about 8 buckets, or 21 mm: what the vertex shows.

### Fixing it removes the bias

The table above: the first 200 events of the data set, re-digitized locally at the data's fidelity (0.25 mm
steps, 4000 diffusion points) and fit with today's fit. The local re-digitization as made reproduces the
cluster's result on the same events (paired change −0.01 ± 0.04). With the response fixed the bias falls by
0.44 ± 0.04 (paired, same events). Making the pseudo-data's hits exactly as the fit makes its own (nominal
response, hits fit to the simulated charge, the fit's threshold) gives the same result as only fixing the
response (paired change +0.005 ± 0.022): the deconvolution, its filter and the PSA threshold add no
measurable bias once the response is right. (The beam's deposits and its separate pulse-shape analysis are
the same in both.)

### The simulation's fidelity is a smaller effect

Fitting the as-made data with the fit's simulation at the data's own fidelity (0.25 mm steps, 4000 diffusion
points; 461 events) lowers the bias by 0.12 ± 0.02 (paired, lowest draw; the parabola minimum moves by
0.075 ± 0.010). That is a quarter of the bias, from the fit's coarser simulation, measured on the buggy data.

## What remains

- **A residual shift below one unit of Z.** With the response fixed, the integer fit is right for 94% of
  events, but the parabola through the mean objective still has its minimum 0.12–0.14 below the true light Z
  (both fixed data sets). TRUEPAT_PLACEHOLDER
- **Unfittable events.** About 5% of events in today's fits had no simulation that produced any charge to
  compare (every objective at its maximum): 154 of 159 because the pattern vertex, shifted by the bug, lay
  upstream of the window, outside the drift volume. `tune_report.py --all` counts them as fits with an
  arbitrary Z, and they are most of the "5% off by 3 or more". With the response fixed these events are fit,
  but those with vertices within about 20 mm of the window have an almost flat objective (too little charge
  compared) and fail.

## Reproducing

Tools in this directory:

- `dump_landscape.C`: every sampled (Z, objective) point of every event of a set of fit or results files,
  with the true vertex, the pattern's directions and vertex, and the fit's beam energy.
- `landscape.py`: per-event mean objective by Z; bias of the lowest draw, of the mean-objective minimum and
  of the parabola minimum, per true Z, and the mean objective by distance from the true Z.
- `paired.py`: paired change of the bias between two fits of the same events.
- `vertex_check.C`: Y-pattern vertex and angles against the simulated vertex, per event.
- `truth_dirs.C`: true fragment directions and energies from sim files (extends `sim_truth.C`).
- `make_figures.py`: the figures.

Settings added (all default to the old behaviour):

| setting | meaning |
|---|---|
| `digi.respTB` | 1: read the measured response per time bucket (the fix) |
| `digi.respMeasured` | 0: nominal GET response, as the fit's pulse uses |
| `digi.simChargeHits` | 1: off-beam hits fit to the simulated charge, as the fit makes them |
| `digi.psaThreshold`, `digi.beamThreshold` | PSA thresholds (10 and 45) |
| `digi.beamCharge` | 0: drop the beam's energy deposits before digitizing |
| `fit.truePattern` | 1: the fit uses the true fragment directions and vertex instead of the Y-pattern's |

## Appendix

### Fits and data sets

| name in files | what it is |
|---|---|
| `tune_data90-fine` | the 3000-event 90° pseudo-data (0.25 mm steps, 4000 points), digitized as made |
| `tune_fid90-base`, `-base2` | today's fit of it, and a reseeded repeat |
| `tune_iter90-match` | the fit at the data's fidelity, first 10 chunks, as-made data |
| `tune_bias_det-asis`, `-resp`, `-qhits` | local re-digitizations of chunks 0–3 (0–1 for asis): as made, response per time bucket, hits as the fit makes them; each fit with today's fit |
| `tune_bias_det-truepat` | today's fit of `-resp` with the true directions and vertex |
| `tune_bias_resp-fix`, `-base`, `-match` | cluster: all 3000 events re-digitized with the response per time bucket; today's fit of them; the fit at the data's fidelity on the first 10 chunks |

### Paths

- Fetched cluster fits of the tuning study: `.claude/worktrees/fit-tune/macro/e12014/fissionStudies/data/remote/hpc-tune/`
- Local re-digitizations and fits: `.claude/worktrees/fit-bias/macro/e12014/fissionStudies/data/`
- Cluster data directory: `/home/faculty/aanthony/fission/adam/tune/data`
- Shared inputs used locally: `/home/adam/fair_install/tpcSharedInfo` (same content as the cluster's)
