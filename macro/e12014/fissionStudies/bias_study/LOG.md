# Bias study log

Terse, dated, append-only. Decisions and events; results live in README.md.

- 2026-10-19 07:20 Fetched fit results hold all 200 sampled (Z, objective) points per event. Directions and
  vertex are fixed to the Y-pattern (spread 0), so Z is the only free parameter: per-Z mean objective is a clean
  landscape. Base fit: bias of best draw +0.401, of mean-objective minimum +0.399. Bias is in the model, not the
  search.
- 07:25 Pattern vertex is 19.7 mm upstream of the true one at every drift distance (sd 1-6 mm).
- 07:30 response.h indexes the TB-sampled measured response by time in us (AtPulse passes us), stretching the
  pseudo-data's response 3.1x against the respAvg the PSA deconvolves with. Predicted residual shift ~8 TB = 21 mm.
- 07:35 The ~5% "off by 3 or more" are mostly events whose every simulation has the max objective (159/3000):
  154 have the pattern vertex outside the drift volume (vz < 0). tune_report --all counts them as fits.
- 07:40 Added digi options respTB, respMeasured, simChargeHits, psaThreshold, beamThreshold, beamCharge (old
  behaviour by default). Local study tune_bias_det.ini (asis/resp/qhits) started on sim chunks c00-c03.
- 07:45 Checked: fit uses 931.5 MeV/u, sim 939: lab kinetic energies agree to first order, negligible. Shared
  inputs here match the cluster's (zap list same content, tables and respAvg identical).
