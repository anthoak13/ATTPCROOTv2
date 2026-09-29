# e12014 Fission Fit Validation: Handoff Notes

Work by Skyler Gangestad (2024–2026), with the pipeline set up by Adam Anthony. Written 2026-09 for the next student or agent picking this up.

## Goal

Check how well the Monte Carlo fitter (`MCFitter::AtMCFission`) recovers fission-fragment charge Z in e12014 (²⁰⁰Bi beam at ~2.7 GeV on ⁴He gas, fissioning via ²⁰⁴At, Z = 85). Simulate fission with known truth, run the full detector response and reconstruction, fit, and compare the fitted Z with the true Z.

## Pipeline

1. `run_simp_fiss.C` + `eventSim.h` simulate with `AtSimpleSimulation`:
   - Beam spot, direction and energy are sampled from fits to data.
   - Fragment masses are Gaussian (`massFrac`, `massDev`). Z comes from the allowed-ion list (26–59). Total kinetic energy follows Viola systematics.
   - The decay angle is fixed or sampled. A trigger cut requires both fragments to be more than 50 mm from the beam axis.
   - Energy loss uses LISE or SRIM tables.
2. `run_digi_fiss.C`: clusterize, pulse with the measured response (`respAvg.root`), smart-zap low-gain pads, PSA (`AtPSAComposite`), Y-RANSAC, then `AtFissionTask`.
3. `run_fit.C`: `AtMCFitterTask` with `AtMCFission`, 100 iterations × 2 rounds.
4. `plot_fit.C`: fitted Z/A/mass ratio, objective functions, and truth comparison (`zHistSim`, `zHistDiff`). `group_fit_plots.C` groups results by a scanned parameter.

`hpc/` was never used.

## Results So Far (`plots_pdf/`)

Run conditions are inferred from file names and commit diffs, so treat them as approximate. The value is the width σ of true Z minus fitted Z.

| Study | σ | Note |
|---|---|---|
| Symmetric split, single Z | 0.41 | Unbiased |
| Fixed decay angle (90°, 120°) | 0.17 | Best case |
| `massDev` = 2 | 0.68 | Centered |
| Asymmetric split | 0.69 | Centered |
| SRIM in simulation vs LISE in fit | 1.14 | Two peaks at ±1: the energy-loss table mismatch biases Z by about ±1 |

## Known Issues

- Paths are hardcoded to specific machines (`/home/skyler/fission/tpcSharedInfo/`, `/mnt/tpc-data/`).
- **`tpcSharedInfo/` lives in Skyler's home directory.** It holds the energy-loss tables, `respAvg.root` and `e12014_zap.csv`. Move it to a group location before her account is removed.
- Run settings are set by editing the macros, and no record ties a run number to its settings. Overwriting runs is intended; the problem is the missing record.
- The random seed is time-based (`SetSeed(0)`) and not recorded.
- Truth is carried in a labeled-bin `TH1D` and read through a friend tree. See [docs/development/mc-truth.md](../../../docs/development/mc-truth.md) for why and what should replace it.
- `eventSim.h` uses 939.0 MeV per nucleon, not 931.5. See [docs/development/units-and-conventions.md](../../../docs/development/units-and-conventions.md).
- All studies use a single Z per run. No fit to a realistic Z distribution yet.

## Infrastructure Ideas (discussed, not decided)

- Store the settings that produced each output file (stage parameters, seed, git hash, parent file) inside the file itself, so results describe themselves.
- Pass truth into the fit output so analysis reads a single file (see `mc-truth.md`).
- Take paths from one environment variable or include file.
- Keep a short study log (question, settings, runs, result).
- Rejected as overkill at this scale: content-hashed run IDs, nested run directories, a run index database, a custom submit/status CLI.
