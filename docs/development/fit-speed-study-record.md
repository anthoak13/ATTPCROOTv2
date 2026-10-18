# MC Fitter Speed Study: Full Record

Result of the 2026-10-17 study of the CPU cost and thread scaling of the e12014 fission fit (`macro/e12014/fissionStudies/run_fit.C`), without changing its physics. The study was made as one commit per change on `fit-speed-tools` (`skyler-fission` plus benchmark tooling): the first six (branch `fit-speed`, "phase 1") cut the CPU per event, the rest (`fit-speed-v4`, "phase 2") remove the limit on threads. That code reached develop as #270 and those branches were removed; the commits cited below survive in the cluster checkouts (see the appendix). [fit-speed-study.md](fit-speed-study.md) is the summary on develop. Code is described here as it was in the study. Numbers are from the cluster unless marked local; its `cs` nodes have 64 cores with two hardware threads each (128 CPUs).

## Result

- The production fit (4 threads) costs 4.9 times less CPU per event than the reference, with the same ΔZ and objective distributions.
- Wall time per event now keeps falling up to 50–64 threads: 3.69 s/event at 50 threads, against 10.7 s at the best thread count (16) before phase 2.
- For throughput, two threads per allocated core is still the most efficient setting. See [Recommendation](#recommendation).

All rows fit the same 300-event digitization (`studies/speed_ref.ini`: `massDev 2`, `decayAngle 0`, LISE) with `iter 100`, `rounds 2` and `threads 4`. Each fit runs twice (fits `a` and `b`) to show the run-to-run spread.

| Code | What it adds | CPU-s/event | Speedup | ΔZ mean | ΔZ σ | KS p (ΔZ, obj) | Gate |
|---|---|---|---|---|---|---|---|
| reference (`fit-speed-tools`) | benchmark tooling only | 533 / 534 | 1.00 | −0.96 ± 0.28 / −0.89 ± 0.28 | 3.71 / 3.71 | 1.00, 1.00 | ref |
| `fit-speed-base` | seedable, per-thread RNG | 527 / 518 | 1.02 | −0.91 / −0.86 | 3.66 / 3.75 | 1.00, 1.00 | PASS |
| `fit-speed-v1` | skip the unused ADC convolution | 227 / 224 | 2.37 | −0.94 / −0.90 | 3.65 / 3.74 | 1.00, 1.00 | PASS |
| `fit-speed-v2`\* | exact pulse hot-path optimizations | 405 / 402 | 1.32 | −0.88 / −0.88 | 3.72 / 3.69 | 1.00, 1.00 | PASS |
| `fit-speed-v3` | one Gamma deviate for the low-electron gain | 509 / 508 | 1.05 | −0.79 / −0.92 | 3.71 / 3.69 | 1.00, 0.98 | PASS |
| `fit-speed` (all)\* | base + v1 + v2 + v3 | 97 / 93 | 5.6 | −0.92 / −0.88 | 3.63 / 3.73 | 1.00, 1.00 | PASS |
| `fit-speed-v4` (final) | phase 1 + phase 2 | 108 / 107 | 4.9 | −0.93 ± 0.28 / −0.98 ± 0.28 | 3.69 / 3.69 | 1.00, 1.00 | PASS |

\* Measured with a flat-vector integration map in `AtPulseLine` that was dropped afterwards. It was worth about 12 CPU-s/event (see [the phase-1 table](#phase-1-every-change-tried-kept-or-dropped)). The `fit-speed` code without it was not gated on its own. The `fit-speed-v4` row is the final code. At 4 threads phase 2 changes nothing measurable (see [Thread scaling](#thread-scaling)), so 108/107 against 97/93 is consistent with the dropped map. That row was measured at commit 47ae41d02; the later commits leave a seeded fit bit-identical.

CPU-s/event is the fit jobs' user+sys per event, averaged over 10 chunks, for fits `a` / `b`. The chunk-to-chunk spread is about 9%, so `fit-speed-base` (1.02) and `fit-speed-v3` (1.05) are within noise on CPU. Each variant was measured on top of `fit-speed-base` alone; their savings add (v1 about 310 CPU-s/event, v2 130, v3 25, out of 533), hence 5.6x combined. Wall time per event at 4 threads went from 136 s to 28 s. ΔZ errors are about ±0.28 on the mean and ±0.20 on σ in every row. `speed_report.py` prints the full table (see [Reproducing](#reproducing)).

The gate: a variant passes if its ΔZ mean and σ agree with the reference within 2σ, its fraction of fitted events agrees, and the two-sample Kolmogorov–Smirnov p-values of the ΔZ and objective distributions are above 0.05. Every fitted event in every row passed the fit; the ΔZ rows use the ~178 events that pass `plot_fit.C`'s default cuts.

## Phase 1: what changed

Each commit of phase 1, oldest first.

1. **Benchmark tooling** (`macro/e12014/fissionStudies`, branch `fit-speed-tools`). The study files `speed_ref.ini` (shared digitization), `speed_fit_ref.ini` and `speed_fit_TEMPLATE.ini` (copied per variant); a `<job>.time` file with every job's CPU and wall time, since the cluster keeps no SLURM accounting; and `speed_report.py`, which builds the table above. See "Benchmarking fitter changes" in the manual.
2. **Seedable, per-thread RNG** (`AtTools/AtRandom`, `AtMCFitter`, parameter distributions, `AtPulse`, `AtPulseLine`, `AtClusterize`). The fit could not be seeded and its threads shared `gRandom`. `AtTools::Random::Get()` returns the calling thread's generator, or `*gRandom` when none is installed, so sim and digi draw as before. Each fitter worker installs a `TRandom3` seeded from `fit.seed`, entry number, round and thread, so a fit with the same seed and thread count reproduces bit for bit. Worth 2% on the cluster, 23% at 4 threads on the desktop.
3. **Skip the ADC convolution** (`run_fit.C`: `pulse->SetDoConvolution(false)`). With `SetUseSimCharge(true)` the PSA reads only the "Q" augment, and nothing in the objective or the study tooling reads the simulated traces. The convolution was 38% of every iteration. Fit results are bit-identical. The 10 saved simulated events per fit now carry charge but zero traces, and fit files shrink from about 23 to 3.5 MB per event.
4. **Exact pulse hot-path optimizations** (`AtMap/AtPadLookup`, `AtPulse::ThrowGain`, `AtPulseLine`, `AtPSADeconvFit`, `AtE12014`, `AtELossTable`). A grid of pad numbers stands in for `TH2Poly::FindBin` (rewritten in phase 2; `AtMap::SetUsePadLookup(false)` switches it off). Pad inhibition is looked up once per pad instead of once per time bucket, per-hit `TF1`s were reused per thread (replaced in phase 2), and the per-step "Energy converged" log line is debug level. Fit output was bit-identical to the base, change by change; about 27% less single-thread CPU locally. A flat-vector integration map that was part of the measured commit was dropped afterwards.
5. **One Gamma deviate for the low-electron gain** (`AtPulse::MeanPolyaGain`). The Polya gain with parameter `b` is a Gamma distribution, so the mean of `n` draws is one Gamma(`n(b+1)`, `G/(b+1)`) deviate: an Erlang deviate (minus the log of a product of uniforms) for integer shapes up to 16, `std::gamma_distribution` above. It replaces up to ten `TF1::GetRandom` table lookups per deposit (118 million per event), for deposits of 10 or fewer electrons with the default fast gain and for every deposit with `UseFastGain(false)`. It is exact; the old table draw's variance was 2% low and its mean 0.2% low. It changes the random stream, in the digi stage too, so it is validated statistically: gtests against the old draw and the exact moments, the gate above, and the [Geant4 check](#geant4-digitization-check). `MeanPolyaGain`'s comment derives the identity; the old `TF1` formula survives only in `AtPulseTest.cxx` (`MakeOldGainFunc`). **This change is subject to revision following a detailed human review** of the derivation and of the Polya model itself.
6. **Threading doc**: its bug list brought up to date.

## Phase 1: every change tried, kept or dropped

CPU-s/event is the saving out of the reference's 533, from the variants above and local per-step timings scaled to the cluster's v2 total (v2 rows about ±10 CPU-s). "Lines" counts code added or changed, tests excluded. Readability is what the change asks of someone debugging through it later. Bit-identical output was a free check where a change kept the random stream; it is not a requirement, and no code should exist only to preserve it. Statistical equivalence under the gate is the standard.

| Change | Where | CPU-s/event saved | Lines | Readability cost | Decision |
|---|---|---|---|---|---|
| Skip the ADC convolution in the fit | `run_fit.C` | ~310 | 1 | low; saved simulated events in fit files have empty traces (noted at the call site and in the manual) | kept |
| Pad inhibition looked up once per pad | `AtPulse::ThrowGain`, `AtPulseLine` | ~50 | 15 | low; `ThrowGain` assumes the pad is not totally inhibited, which its doc says | kept |
| Pad lookup grid in front of `FindBin` | `AtMap/AtPadLookup` | ~45 | 260 | medium; two code paths (grid or `FindBin`), but one immutable 60 MB vector built once, no atomics. Mitigated by `AtMap::SetUsePadLookup(false)` for bisecting, and tests against `FindBin` on the AT-TPC and GADGET II planes | kept, with switch |
| Thread-local `TF1` reuse | `AtE12014`, `AtPSADeconvFit` | ~35 | 50 | medium; state persists between calls | replaced in phase 2 by a plain Gaussian (`AtTools::GaussianTB`), with no per-thread state |
| One Gamma deviate for the low-electron gain | `AtPulse::MeanPolyaGain` | ~25 | 40 | medium; a mathematical identity replaces a formula. The comment derives it and says the shortcut holds only for a Gamma-shaped gain; the old `fGainFunc` can no longer be plotted | kept for now, documented; **subject to revision after a detailed human review** of the derivation and of whether the Polya model itself should stay |
| Seedable per-thread RNG | `AtTools/AtRandom`, `AtMCFitter`, distributions | ~10 (2%; 23% on the desktop) | 290 | medium; readers must know `Random::Get()` is `gRandom` unless a generator is installed. Makes fits reproducible, which pays for itself in debugging | kept |
| Flat vector and sorted pad list for the integration map | `AtPulseLine` | ~12 | 30 | medium; three scratch members and a sort whose only purpose was to keep the fit output bit-identical to the old code, for one `std::map` line | dropped: bit-identical output is not a requirement (the physics is not known to that precision) and the cost per CPU-second was the worst of the set |
| Hoist bin centers out of the time-bucket loop | `AtPulseLine` | small (part of the row above) | 5 | none; a local vector | kept |
| "Energy converged" log line to debug | `AtELossTable` | ~2 | 2 | none; shrinks logs from 5 MB per event | kept |
| Cheaper diffusion draws | `AtPulseLine::generateIntegrationMap` | ~45 estimated, never built | — | would change the random stream for a draw that is easy to read today | not built |
| Fewer integration points per line | `AtPulseLine` | ~14% per halving | 0 | none, but it changes the physics of the charge sharing | not built: a physics choice, not an optimization |
| Geometry lock, persistent thread pool, dynamic work split, fewer copies | `AtMCFitter` | each <2% in the profile | — | — | not built: measured too small |

## Phase 2: what changed

1. **Build Minuit2 directly** (`AtTools::MinimizeMigrad`, used by `AtPSADeconvFit::FitGaussian` and the amplitude fit in `AtMCFission`). `ROOT::Fit::Fitter` creates its minimizer through ROOT's plugin manager under ROOT's global write lock (11–16 µs per fit, local). With about 250 pad fits per fitter iteration, the threads queued on that lock (a lock convoy), seen as system time of 9.6, 48 and 138 s/event at 16, 32 and 64 threads, which made 32 and 64 threads slower than 16 (wall 10.7, 15.8, 27.7 s/event). `MinimizeMigrad` builds a `Minuit2Minimizer` on a `Chi2FCN` with `Fitter`'s step sizes, call limit (1000 + 100n + 5n²) and default options, without the lock. Fits are bit-identical (20 000 pad-like and 5 000 amplitude fits compared locally, and the seeded 5-event fit). System time is now 1.3, 3.9 and 14.4 s/event at 16, 32 and 64 threads.
2. **No thread-local `TF1`s** (`AtE12014`, `AtPSADeconvFit`). They only evaluated a Gaussian. `AtTools::GaussianTB` and a lambda do the same with no per-thread state (bitwise equal over 10 million evaluations, local), and `SetHitFunctionTB` is gone. `ContainerManip::CreateHistFromData` no longer toggles the global `TH1::AddDirectory` flag from worker threads, and the result set compares `AtMCResult` by const reference instead of copying it.
3. **Prebuilt pad grid** (`AtMap/AtPadLookup`). The phase-1 grid was filled lazily in tiles, with atomics and a mutex. It is now one immutable `std::vector<int16_t>` at 0.1 mm (60 MB for the AT-TPC plane), built once per process on the first lookup (single-threaded: about 5 s on a desktop core, 12 s on a cluster node) and shared read-only by all threads. A cell stores a pad only if its center and four inset corners agree and no pad edge crosses it; otherwise the lookup calls `FindBin`. The corner test fixes slivers at the bottom rim of the AT-TPC plane, where `FindBin` changes across internal `TH2Poly` partition lines (90 mismatches in a 598 M-point scan of the lazy grid). The grid agrees with `FindBin` at every point tested: a 150 M-point scan of the whole plane, and unit tests that include points 1e-7 mm inside every cell side of the bottom 5 mm. 10% of the points in the AT-TPC disk go through `FindBin` (5% before). 61 lines fewer (322 → 261), no atomics.
4. **Small fixes.** `AtStudentDistribution::SetDoF` now reaches the sampler. The Polya fast-path width is computed inline as G/√((b+1)n), which cannot go stale. `AtPulseLineTask` draws from the per-thread generator. `AtPSADeconv` stops with a fatal error when `SetUseSimCharge(true)` meets a pad without "Q", instead of running the shared, unprotected FFT path. The Minuit2 setup is one helper. `AtMCFitter` now derives each thread's seed as `MixSeed(MixSeed(seed, entry), round, thread)` and passes the event seed to each round. Fits are still reproducible for a given seed and thread count, but fits on this branch do not reproduce `fit-speed` fits made with the same `fit.seed`.
5. **Less serial work per event** (`run_fit.C`). FairFileSource reads every branch of the input, including the digi `AtRawEvent` traces (16 MB compressed per event) that the fit never uses. The fit now turns that branch off after `Init`. A new setting, `fit.saveEvents` (default 10, as before), sets how many simulated events are saved per event. Both reading the branch and writing the saved events are main-thread work between rounds while the workers wait (see [Serial work per event](#serial-work-per-event)).

| Change | Where | Effect | Lines | Readability cost | Decision |
|---|---|---|---|---|---|
| Direct Minuit2 | `AtTools::MinimizeMigrad` | removes the lock convoy: wall at 64 threads 27.7 → 3.85 s/event; nothing measurable at ≤ 8 threads | one 20-line helper | low; one comment says why `Fitter` is bypassed | kept |
| Plain Gaussian instead of thread-local `TF1`s | `AtE12014`, `AtPSADeconvFit`, `AtTools::GaussianTB` | removes per-thread state; no measurable time | +115/−125 together with the row above | lower than before | kept |
| Prebuilt pad grid instead of lazy tiles | `AtMap/AtPadLookup` | same steady-state speed; 5–12 s once per process; fixes the rim slivers | −61 | lower: no atomics or cell states | kept |
| Skip the input `AtRawEvent` branch | `run_fit.C` | about 0.1 s/event less main-thread time | 1 | none | kept |
| `fit.saveEvents` | `run_fit.C` | 0 saves about 0.35 s/event of main-thread time, about 10% of the wall at 32–64 threads | 1 | none | kept as a setting; default unchanged at 10 |
| Dynamic work split (atomic counter) | `AtMCFitter::RunRound` | does not remove the granularity loss: 100 equal iterations on 64 threads still take two waves | ~15 | would make fits independent of the thread count | not built |

### Geant4 digitization check

The digitization changes were also checked on Geant4 events (local). One 500-event Geant4 AT-TPC simulation (`macro/tests/AT-TPC/run_sim_attpc.C`) was digitized with `run_digi_attpc.C` by the reference build and by `fit-speed-v4`, each with two seeds. The new gain draw uses a different number of random numbers, so the same seed gives a different event, and reference against branch is compared like two seeds of one build.

The two are equivalent. Every distribution (fired pads, total ADC and Q, per-pad maximum ADC and its time bucket, PSA hits, hit charge, z and r, RANSAC track candidates) agrees with KS p ≥ 0.80 between reference and branch, and ≥ 0.60 over all pairs including seed against seed. The per-event Jaccard overlap of the fired-pad sets is 0.869–0.872 for every pair, seed against seed included. The one systematic difference is +0.04% in total charge (±0.003%), about four times the seed-to-seed shift and far below the 0.06% per-event spread. The likely origin is the old Polya draw: `TF1::GetRandom` sampled a table cut off at 5 times the gain, whose single-electron mean is 0.18% low. That draw is used only for deposits of 10 or fewer electrons, so a shift of the total of about a fifth of that is the expected size. The branch digitizes 15–25% faster in that local run. The cluster timing of a field-on two-body case below is smaller: 5.5% less CPU per event, start-up included.

#### Geant4 10Be(d,p) timing (cluster)

**Case.** 10Be(d,p) two-body kinematics (`AtTPCIonGenerator` and `AtTPC2Body`, 20–40° in the center of mass) in the AT-TPC deuterium gas geometry `ATTPC_D1bar_v2.root`, tracked by `TGeant4` in a 30 kG field. The macros are `macro/Simulation/ATTPC/10Be_dp/Be10dp_sim.C` and `run_digi_attpc.C` there. The digitization runs `AtClusterizeTask`, `AtPulseTask` with `AtPulse` (Q saved), `AtPSAtask` with `AtPSAMax` (threshold 0) and `AtPRAtask`. Only the event count, the seed and the task list were made arguments of the digitization macro, and the inhibit list `inhibit.txt` it reads was supplied from `macro/tests/AT-TPC/data`.

**Setup.** One 3000-event simulation with the reference code is the input of every digitization. Every run is a single process on one core. 1458 of the 3000 events fire at least one pad; the rest fire none.

**Timing.** CPU is user plus system time of the whole process, including start-up. The spread is the standard deviation of three digitization repeats with one seed.

| Stage | Reference CPU-s/event | Branch CPU-s/event | Ratio |
|---|---|---|---|
| Geant4 simulation (3000 events, one run each) | 0.0086 | 0.0086 | 1.00 |
| Digitization, all four tasks | 0.1906 ± 0.0013 | 0.1800 ± 0.0017 | 0.945 |

- **Start-up.** It is part of both rows: 4.1 s for the simulation (0.0014 CPU-s/event), 6.1 s for the reference digitization and 18.2 s for the branch. The branch's 12 s extra is the pad lookup grid, built on the first pad lookup. Without start-up the digitization is 0.1885 and 0.1739 CPU-s/event, a ratio of 0.923.
- **Simulation.** The simulation code is untouched by the branch, and the two times agree within 1%.
- **Run to run.** Individual digitization runs of one build differ by less than 2%. Each branch run is 4–7% below each reference run.

**Per-task split on the branch.** Single runs with tasks removed from the end; each task's cost is the difference. The pulse row includes the one-time 12 s grid build, and the clusterize row includes about 6 s of initialization.

| Task | CPU-s/event | Share |
|---|---|---|
| `AtClusterizeTask` | 0.0646 | 36% |
| `AtPulseTask` (`AtPulse`) | 0.0663 | 37% |
| `AtPSAtask` (`AtPSAMax`) | 0.0024 | 1% |
| `AtPRAtask` | 0.0467 | 26% |
| Total | 0.1800 | 100% |

The Geant4 simulation and the digitization tasks are single-threaded: the Geant4 build is not multithreaded and the tasks run serially. These stages scale only by running chunks of events as separate processes.

**Equivalence check.** The 3000-event simulation was digitized by the old code (seed 1) and by the new code with two seeds (A = 1, B = 2).

- **Why it is compared statistically.** The new gain draw uses a different number of random numbers, so the same seed gives a different event. Old against new is therefore compared like two seeds of one build. Repeats of one build with one seed are bit-identical.
- **Distributions.** Every distribution agrees with KS p ≥ 0.995 for old against A, old against B and A against B (seed-to-seed). The distributions are: fired pads, total ADC and Q per event, per-pad maximum ADC, PSA hits, hit charge, z and r, and pattern-recognition track candidates. With threshold 0 every fired pad is one PSA hit, so the hit count and charge repeat the pad quantities. Means and standard deviations agree: fired pads 230.0 / 230.2 / 230.1 per event (old / A / B, σ ≈ 318), hit z 859.4 mm in all three, hit r 116.6 mm in all three.
- **Fired pads.** The per-event Jaccard overlap of the fired-pad sets is 0.901 (old against A), 0.901 (old against B) and 0.902 (A against B), over the 1459 events in which either set is non-empty.
- **Total charge.** The one systematic difference is +0.19% (±0.04%) in the new code's total ADC per event against the old for seed A, and +0.18% (±0.03%) for seed B, the same for Q. A against B differs by 0.04% (±0.03%), and the per-event spread is 1.0–1.4%. The per-pad maximum ADC mean shifts the same way, from 658.6 to 659.5 (A) and 659.8 (B). The shift equals the old draw's 0.2% low single-electron mean.

## Thread scaling

`studies/thread_scan.ini` fits one 30-event digitization (chunk 0 of `speed_ref.ini`) with 1 to 64 threads, with `iter 100`, `rounds 2` and a fixed `fit.seed`. It ran on `fit-speed` and on `fit-speed-v4` (at 47ae41d02).

**How SLURM places threads.** The cluster's SLURM allocates whole cores (`select/cons_tres` with `CR_CORE`). `--cpus-per-task=N`, which the driver requests for `fit.threads = N`, gives N/2 physical cores with both hardware threads, so the fit's threads share cores in pairs. With `--hint=nomultithread`, it gives N whole cores, one thread per core. A 1-CPU job gets a whole core. The 16- to 64-thread fits ran with `--exclusive`, which left their threads unbound across all 128 CPUs of the node, so from 16 threads up the two arms are the same configuration.

| Threads | `fit-speed` wall-s/ev | CPU-s/ev (sys) | `fit-speed-v4` wall-s/ev | CPU-s/ev (sys) | v4, one thread per core: wall-s/ev | CPU-s/ev |
|---|---|---|---|---|---|---|
| 1 | 70.09 | 69.8 (0.6) | 68.17 | 67.8 (0.6) | — | — |
| 2 | 48.55 | 95.5 (0.7) | 47.93 | 94.0 (0.6) | 34.91 | 68.2 |
| 4 | 24.69 | 95.4 (0.8) | 25.35 | 96.9 (0.7) | 23.81 | 80.7 |
| 8 | 13.01 | 94.3 (0.8) | 13.35 | 94.6 (0.6) | 12.79 | 79.4 |
| 16 | 10.73 | 102.0 (9.6) | 8.06 | 96.9 (1.3) | 8.08 | 97.2 |
| 32 | 15.84 | 138.0 (48.1) | 5.21 | 90.9 (3.9) | 5.27 | 91.6 |
| 50 | — | — | 3.69 | 95.3 (8.3) | 3.75 | 97.1 |
| 64 | 27.68 | 231.6 (137.7) | 3.85 | 102.5 (14.4) | 3.84 | 102.9 |

`fit-speed` values are the mean of two repeats, which differ by 1–4% in wall; v4 values are single runs. Its one-thread-per-core arm (2–8 threads) agrees with v4's within 5%.

- Up to 8 threads the two branches agree within 5%. Phase 2 matters from 16 threads up.
- CPU per thread, relative to one thread: one thread per core 1.01 at 2 threads and 1.17–1.19 at 4 and 8; two threads sharing a core 1.39–1.43, which still gives 1.42x the events per core of one thread alone; 1.34–1.52 at 16 threads and up (unbound).
- **Process control.** Sixteen 1-thread fits at once, each on its own core of one node, used 67.7 CPU-s/event each (67.4–68.3), the same as one fit alone (67.8). A busy node (turbo, shared caches) costs nothing; the inflation comes with threads in one process or with shared cores.

### Serial work per event

The main thread reads each input event and writes the results and saved events while the workers wait. Wall-s/event, with the main-thread seconds per event between rounds in parentheses:

| Threads | input `AtRawEvent` read, 10 saved | branch skipped, 10 saved | skipped, `fit.saveEvents = 0` |
|---|---|---|---|
| 1 | 68.17 (0.57) | 68.29 (0.44) | 69.25 (0.17) |
| 16 | 8.08 (0.53) | 7.54 (0.52) | 7.66 (0.10) |
| 32 | 5.27 (0.58) | 5.12 (0.48) | 4.82 (0.14) |
| 64 | 3.84 (0.55) | 3.75 (0.46) | 3.44 (0.12) |

The first column is the scan above (one thread per core from 16 up); the others ran at 7eb94a990 in the same way. The 16-thread values carry about ±0.3 s of run-to-run noise. Skipping the branch saves about 0.1 s/event, and saving no events about 0.35 s more: about 10% of the wall at 32–64 threads, nothing at 1–4.

### What remains at high thread counts

At 64 threads with `fit.saveEvents = 0` a 30-event job takes 103 s: 12 s of Init, a first event 13 s longer than the others, and 29 events of 2.5 s. The open items, each with the experiment that would settle it:

- **Static split.** Each round splits 100 iterations into contiguous blocks, so the busiest thread runs ceil(100/N): at least 11% idle at 16 threads, 22% at 32 and 64, none at 50. 50 threads measured 2–4% faster than 64 (single runs, within the run-to-run spread, but in the predicted direction on both arms). A dynamic split would not help, since the iterations cost the same. Use `fit.iter` as a multiple of `fit.threads`.
- **Per-thread CPU inflation of 1.34–1.52x at 16 threads and up.** The cause is untested. The exclusive jobs' threads were unbound, so two threads sharing a core is a candidate, as are memory effects inside the process (bound one per core, 4 and 8 threads already show 1.17–1.19x). Experiment: run 16 and 32 threads bound one per core (`--hint=nomultithread` without `--exclusive`, or `--cpu-bind=cores`) and compare CPU-s/event with the process control.
- **Start-up and the first event.** Init takes 9–12 s per job. At 16 threads and up, the first event takes 10–14 s longer than the rest, of which the pad grid build is 5–12 s depending on the core; the rest is untested (per-round thread and pulse-clone setup are candidates). At 30 events per job this is about a quarter of the wall at 64 threads. Experiment: log timers around the grid build, the clone construction and the first round. Longer chunks amortize it.
- **Serial main-thread work**: 0.45 s/event with 10 saved events, 0.12 with none. Whether `fit.saveEvents` should default lower than 10 is an analysis choice: the saved events are what one inspects to check a fit.

### Recommendation

- **Throughput (production, many chunks):** few threads per job, two per allocated core. `fit.threads = 2` with the default allocation runs on one core at 47.9 core-s/event. One thread on a core takes 68.2, and the current 4 threads on 2 cores take 50.7, so 2 or 4 threads are both fine and 1 wastes the second hardware thread. With `--hint=nomultithread`, one thread per core is as efficient as one process per core (2 threads: 69.8 core-s/event). Keep `fit.iter` a multiple of `fit.threads`, and fill the nodes with chunks.
- **Turnaround on one chunk:** 32–50 threads on a whole node: 5.21–3.69 s/event at 91–95 CPU-s/event. This holds the node's 64 cores for about 236 core-s/event at 50 threads, so use it only when latency matters.

## Reproducing

From `macro/e12014/fissionStudies` on the branch to measure, with a cluster set up as in the manual (one `remotes.ini` section per checkout, all sharing `env.FISSION_DATA`):

```bash
./fission.py --remote <name> --sync                                  # once per checkout
./fission.py studies/speed_ref.ini --remote <name> --slurm           # once, the shared digi
cp studies/speed_fit_TEMPLATE.ini studies/speed_fit_<variant>.ini    # per variant
./fission.py studies/speed_fit_<variant>.ini --remote <name> --slurm # per checkout
./fission.py studies/speed_fit_<variant>.ini --remote <name> --fetch
./speed_report.py --ref <ref-name>:speed_fit_ref <name>:speed_fit_<variant> ...
```

Thread scaling, after `speed_ref.ini` (edit the node names in the `slurm.partition` values first):

```bash
./fission.py studies/thread_scan.ini --remote <name> --slurm
./fission.py studies/thread_scan.ini --remote <name> --fetch         # results and .time files
./fission.py studies/thread_scan.ini --remote <name> --fetch-logs    # logs, for the per-round times
./speed_report.py --ref <name>:thread_scan                           # one row per section, against t1
```

`speed_report.py` gives wall-s/event, CPU-s/event and the gate for every section. The derived columns come from the logs: Init is the `.time` wall minus `run_fit.C`'s "Real time", and the main-thread seconds per event are that "Real time" minus the sum of the "Simulation of 100 events took" lines, divided by the events. Outputs are named after the study, so to scan a second checkout that shares `FISSION_DATA`, run a copy of `thread_scan.ini` under another name. For the serial-work table, add copies of the 1-, 16-, 32- and 64-thread sections with `saveEvents = 0`.

To check whether a change keeps fit results bit-identical, fit the same digi file twice with the same `fit.seed` and `fit.threads` and compare a dump of every `AtMCResult`:

```cpp
void dump_all(TString path, TString out)
{
   std::unique_ptr<TFile> f(TFile::Open(path));
   TTreeReader r("cbmsim", f.get());
   TTreeReaderValue<TClonesArray> res(r, "AtMCResult");
   FILE *o = fopen(out, "w");
   while (r.Next())
      for (int i = 0; i < res->GetEntriesFast(); ++i) {
         auto x = dynamic_cast<MCFitter::AtMCResult *>(res->At(i));
         fprintf(o, "ev %lld res %d obj %.17g", r.GetCurrentEntry(), i, x->fObjective);
         for (auto &[k, v] : x->fParameters)
            fprintf(o, " %s=%.17g", k.c_str(), v);
         fprintf(o, "\n");
      }
   fclose(o);
}
```

This is a free check when a change happens to preserve the random stream, not a requirement. It only works on the fit: the digi stage's hits vary with machine load (see [Open items](#open-items)), so compare fits of one digi file, not of two digi runs.

## Open items

- Tied objectives (bug 2 in [mc-fitter-threading.md](mc-fitter-threading.md)) make which of several equal results is kept depend on thread timing, so a multi-threaded fit is reproducible except for those. `std::multiset` would fix it.
- The thread-scaling items [above](#what-remains-at-high-thread-counts): the per-thread CPU inflation and the slow first event.
- Peak memory was not measured on the cluster (its compute nodes lack GNU `time`); the pad grid adds 60 MB per process, and the 4 GB jobs ran.
- The digi stage is not reproducible under load, and never was: `AtPSADeconv` creates its FFT plans with FFTW's "measure" option (`"R2C M K"`), which picks the algorithm by timing it, so hit positions differ at the 1e-9 level between runs on a busy machine (four quiet runs agreed exactly, three concurrent runs all differed). The fit reads the simulated charge directly and is unaffected. `"ES"` (estimate) in place of `"M"` would make the plan deterministic.
- Cheaper diffusion draws in `AtPulseLine::generateIntegrationMap` were not built. They are estimated at about 9% of the CPU and would be validated with the statistical gate.
- The reference checkout's fits were made before the sync-guard fix; it is one driver commit behind, which changes nothing in the fit.

## Appendix: machines and paths used in this study

- Cluster checkouts: `/home/faculty/aanthony/fission/adam/speed/<ref|v2|v3|combo|v4>` (`v4` is `fit-speed-v4`), shared data in `/home/faculty/aanthony/fission/adam/speed/data` (about 40 GB, mostly digi and fit files; the results files are small). The `base` and `v1` checkouts were removed after their fits; their results are in the data directory. `/home/faculty/aanthony/fission/adam/speed/scaling` holds the output of the SLURM CPU-affinity tests.
- Desktop: `TPC_SHARED_INFO=/home/adam/fair_install/tpcSharedInfo`. Worktrees: `.claude/worktrees/fit-speed-base` (branch `fit-speed`), `fit-speed-profile` (`fit-speed-v2`), `fit-speed-v3`, `fit-speed-v4`.
- Phase 1 session scratch (fetched results, the profile report `profile_report.md`, the run log and the dump macros): `/tmp/claude-1000/-home-adam-ATTPCROOTv2/9eb326f4-a40c-4c1f-97fa-859190aa8f84/scratchpad/`.
- Phase 2 session scratch: `/tmp/claude-1000/-home-adam-ATTPCROOTv2/f27309a2-cb0f-4468-be05-67fba84e1d06/scratchpad/`. `scaling/` has the thread scans (`cluster_scan.md`, `table_v3.md`, `table_v4.md`, the fetched data), the serial-work runs (`serial_scan.md`), and the lock and load-balance analysis with its desktop benchmarks (`static_analysis.md`). `gate-v4/speed_report_gate.txt` is the `fit-speed-v4` gate. `geant/` has the Geant4 check (`geant_check.md`, `geant_check_v4.md`, `run_geant_check.sh`). `phase2/dump_all.C` is the dump macro above.
- Geant4 10Be(d,p) timing (host `hpc`):
  - **Reference checkout:** `/home/faculty/aanthony/fission/adam/speed/ref`, commit `049853dd` (the code before the speed-up).
  - **Branch checkout:** `/home/faculty/aanthony/fission/adam/speed/v4`, commit `7eb94a99` (the speed-up code; `fit-speed-develop` is this code rebased and squashed onto develop, with no further digitization changes).
  - **Work directory:** `/home/faculty/aanthony/fission/adam/speed/geant/`. It holds `NOTES.md` (commands, job ids, raw timings), the comparison macro `compare.C` and its text output, the job script `scripts/job.sh`, and `runs/` with the simulation and digitized files.
