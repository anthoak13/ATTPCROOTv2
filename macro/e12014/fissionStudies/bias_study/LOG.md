# Bias study log

Terse, dated, append-only. Decisions and events; results live in README.md.

- 2026-10-19 07:10 Fetched fit results hold all 200 sampled (Z, objective) points per event. Directions and
  vertex are fixed to the Y-pattern (spread 0), so Z is the only free parameter: per-Z mean objective is a clean
  landscape. Base fit: bias of best draw +0.401, of mean-objective minimum +0.399. Bias is in the model, not the
  search.
- 07:15 Pattern vertex is 19.7 mm upstream of the true one at every drift distance (sd 1-6 mm).
- 07:20 response.h indexes the TB-sampled measured response by time in us (AtPulse passes us), stretching the
  pseudo-data's response 3.1x against the respAvg the PSA deconvolves with. Predicted residual shift ~8 TB = 21 mm.
- 07:30 The ~5% "off by 3 or more" are mostly events whose every simulation has the max objective (159/3000):
  154 have the pattern vertex outside the drift volume (vz < 0). tune_report --all counts them as fits.
- 07:30 Added digi options respTB, respMeasured, simChargeHits, psaThreshold, beamThreshold, beamCharge (old
  behaviour by default). Local study tune_bias_det.ini (asis/resp/qhits) started on sim chunks c00-c03.
- 07:30 Checked: fit uses 931.5 MeV/u, sim 939: lab kinetic energies agree to first order, negligible. Shared
  inputs here match the cluster's (zap list same content, tables and respAvg identical).
- 07:40 Local re-digitization of sim c00-c03: pattern vertex offset -19.7 mm (as is, reproduced) -> +1.8 mm with
  the response read per time bucket, +1.7 mm with hits from the simulated charge. Bug confirmed.
- 07:45 Cluster checkout code-bias synced at d581913e. Submitted tune_bias_resp.ini (60 digi + 60 fit jobs,
  held, behind ~360 held jobs of the tuning study).
- 08:24 Local fits done (200 events, c00-c03). Best-draw bias: as made +0.43 (100 ev; cluster base on the same
  events +0.46), response per TB +0.026 +- 0.016, hits from simulated charge +0.030 +- 0.014. Landscape vertex
  still +0.12-0.14 in both. 4.5% / 1.5% of events have a flat objective (near-window vertices) and fail.
- 08:25 Matched-fidelity fit (tune_iter90 match, as-made data) on 461 events: paired best-draw change -0.12,
  parabola vertex -0.075 against today's fit. Coordinator's tune_report figure -0.076 includes events with no
  valid simulation.
- 08:26 Cluster tune_bias_resp digis done, 60 fits running. Submitted [fit match] on corrected data (10 chunks).
- 08:28 Added AtMCFission::SetNextEventMeans and fit.truePattern (true directions and vertex) to split the
  residual; local fit truepat on resp c00-c03 started.
- 08:47 truepat (true directions and vertex, resp data, 200 events): bias +0.000 +- 0.007, 95.5% exact, parabola
  +0.103 vs +0.137 with the pattern (paired -0.03 +- 0.02). Pattern errors are not the residual.
- 08:48 Cluster tune_bias_resp base (3000 events, response per TB): bias +0.019 +- 0.003, spread 0.18, 95.6%
  exact, 2.2% off by >=3 (31% of events with true vertex < 25 mm from the window). Parabola minimum +0.13 with
  the old Z trend at a third of the size.
- 08:50 Added sim.decayToBeam (decay angle from the beam direction, as the fit assumes). Local tune_bias_kin.ini
  (200 new events) and tune_bias_det nobeam (resp without beam deposits) started to split the +0.13.
- 09:40 decayToBeam (200 new events, response per TB): parabola +0.18 +- 0.03, not smaller. nobeam: paired
  +0.02 +- 0.03 on 100 events; its fits of chunks c00 and c02 crashed silently mid-run (no beam in the pattern),
  not followed up.
- 09:55 Cluster match on fixed data (500 events): fitted Z unchanged (paired -0.004 +- 0.009), parabola -0.055
  +- 0.007 to +0.095. About +0.1 of the sub-integer shift unexplained.
