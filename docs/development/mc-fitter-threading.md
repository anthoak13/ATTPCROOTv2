# MC Fitter Threading

How `MCFitter::AtMCFitter` parallelizes, and the bugs and bottlenecks found in a read-through. Survey date: 2026-09. Nothing here has been fixed or benchmarked yet. Line numbers are as of commit fa5a9c655 and will drift; bugs that can be checked in output have a check that does not depend on them.

The production configuration is `macro/e12014/fissionStudies/run_fit.C`: `AtMCFission` with `AtClusterizeLine`, `AtPulseLine`, `AtPSADeconvFit` with `SetUseSimCharge(true)`, `fit.rounds = 2`, space charge off.

## Parallel Model

`AtMCFitter` (`AtReconstruction/AtFitter/AtMCFitter.cxx`) is the only threaded code in the fitter, simulation or digitization path. Events are processed serially by `AtMCFitterTask`. Within one event, `Exec` (L97–116):

1. Clears `fResults` and resizes `fRawEventArray`/`fEventArray` to `fNumIter`.
2. Runs `fNumRounds` rounds. Each round (`RunRound`, L117–156) splits `fNumIter` iterations into contiguous blocks, spawns `fNumThreads` new `std::thread`s (even when `fNumThreads == 1`), and joins them.
3. After each round, including the last, `RecenterParamDistributions` (L207–214) moves every parameter distribution's mean to the best result so far and narrows it (`AtUniformDistribution` ×0.8, `AtStudentDistribution` ×0.5).
4. `FillResultArrays` (L177–198), called by the task, copies results into `TClonesArray`s in objective order and saves the first `fNumEventsToSave` simulated events.

One iteration (`RunIterRange`, L72–95) samples parameters, simulates (`AtMCFission::SimulateEvent`), clusterizes, pulses, runs PSA, and computes the objective (a Minuit2 fit inside `AtMCFission`). The result is inserted into `fResults` under `fResultMutex`. Digitized events are written to `fRawEventArray[idx]`/`fEventArray[idx]` without a lock, which is safe within a round because each thread owns its index range.

### Shared vs. per-thread state

| Object | Per thread? | Notes |
|---|---|---|
| `AtPulse` / `AtPulseLine` | Yes | Cloned into `fThPulse` in `Init` (L67–69). Each clone builds a `TH1F` per pad. The map and response function are still shared (read-only). |
| `AtSimpleSimulation` | Shared | Point array and track ID are `static thread_local`. Geometry lookups go through `fGeoMutex`. |
| Space-charge model | Shared, **written per iteration** | See bug 6. |
| `AtClusterize` / `AtClusterizeLine` | Shared | `thread_local` statics. Uses `gRandom`. |
| PSA | Shared | `AtPSADeconvFit` uses a `thread_local` histogram. The FFT objects and response cache in `AtPSADeconv` are shared and unprotected. They are bypassed only when `SetUseSimCharge(true)` is set **and** every pad has a `"Q"` augment (`AtPSADeconv.cxx` L219–220), i.e. the pulse has `SetSaveCharge(true)`. Without the bypass, `createResponsePad` (L138) calls `FairRun::Instance()` from a worker thread. |
| Parameter distributions | Shared | Mean and spread are read-only during a round. The RNG is one `static thread_local std::mt19937` (`AtParameterDistribution.h` L14) shared by every distribution on that thread. See bug 2. |
| `gRandom` | Shared global | See bug 3. |
| `AtDigiPar` | Cached in `fPar` | `FairRun::Instance()` is thread-local. |

Knobs: `SetNumThreads`, `SetNumIter`, `SetNumRounds`, `SetNumEventsToSave`, `SetTimeEvent`. `SetNumThreads` calls `ROOT::EnableThreadSafety()` when > 1 and must be called before `Init()`, which sizes `fThPulse`; raising it afterwards indexes past the end. `fNumThreads <= 0` divides by zero (L124).

## Bugs

Ordered by impact on current runs.

### Live, affects results

1. **Bad events keep the previous event's results.** `AtMCFitterTask::Exec` (`AtMCFitterTask.cxx` L42–43) returns when the pattern event is not good, before the result and saved-event arrays are cleared (L46–48). That entry of the output tree carries the previous event's results, which plots then pair with the current event's truth. If the first event is bad, the arrays are empty.
   - Check: in a fit output, compare `AtMCResult` of consecutive entries; identical arrays mark a skipped event.
   - Fix: clear the arrays before the early return.
2. **The fit cannot be seeded, so it is never reproducible.**
   - Every distribution in `AtMCFission::CreateParamDistros` (L59–70) is created with `seed = 0`, and there is no setter. On first use on each thread the shared RNG is seeded from `std::random_device` (`AtParameterDistribution.cxx` L14–26), logged only at `LOG(info)`. Threads are recreated every round, so this happens per thread per round.
   - Seeding `gRandom` in a macro does not affect parameter sampling. A fit is not reproducible even with one thread, so no other fitter bug can be checked by rerunning with the same seed.
   - Latent: if a nonzero seed is ever wired in as-is, only the first distribution sampled on a thread sets it, every thread draws the same sequence within a round, and every round replays the same raw draws (mapped through the recentred distributions).
   - Fix: one seed per fitter, mixed with event number, round and thread index.
3. **`gRandom` is shared across threads.**
   - In the production configuration the workers draw from it in `AtPulseLine::throwRandomAndGetPadAfterDiffusion` (`AtPulseLine.cxx` L36–37, two draws per integration point, the heaviest use), `AtClusterize::getNumberOfElectronsGenerated` (`AtClusterize.cxx` L119, also used by `AtClusterizeLine`), and `AtPulse` noise and gain (`AtPulse.cxx` L123, L217, and `fGainFunc->GetRandom()` at L221, which uses `gRandom` on the slow-gain path: fast gain off or at most 10 electrons). `AtClusterize::applyDiffusion` (L133–135) is used only by the base `AtClusterize`.
   - `ROOT::EnableThreadSafety()` does not make `gRandom` per-thread in ROOT 6.26 (`TRandom.h` L62).
   - The race corrupts or correlates the RNG state whenever `threads > 1` (default 4). The effect on results is probably statistical and cannot be measured by a seed comparison until bug 2 is fixed.
   - Fix: a per-thread RNG passed in or held `thread_local`.

### Live, affects saved events or counts

4. **Saved events are paired with the wrong result after round 1.** Live with the default `fit.rounds = 2`.
   - `fResults` is cleared once per event, but every round reuses indices `0..fNumIter-1` (L79), so `fRawEventArray[idx]` and `fEventArray[idx]` are overwritten.
   - A round-1 result with `fIterNum = 17` ends up pointing at round 2's event 17, and `FillResultArrays` saves that event next to it.
   - If two saved results share an `fIterNum` (one per round), `FillResultArrays` moves from the same slot twice (L191–192), so the second gets an empty event.
   - Parameters and objectives are correct, so histograms built from `AtMCResult` are unaffected. Anything that inspects the saved best-fit `AtEvent`/`AtRawEvent` may be looking at the wrong simulation.
   - Check: in a fit output with `rounds = 2`, look for repeated `fIterNum` among the saved results of one event, or for saved events with no hits.
   - Fix: index by `round * fNumIter + idx`, or store the event with the result.
5. **Tied objectives are dropped.**
   - `fResults` is a `std::set` ordered by `fObjective` (`AtMCFitter.h` L70, comparator at `.cxx` L33), so results with equal objectives count as duplicates.
   - `AtMCFission` returns `std::numeric_limits<double>::max()` when the charge fit fails (L305) and when the experimental and simulated charge do not overlap (L231, 246, 263). L180 is in `ObjectiveChargePads`, whose call is commented out (L113). All such results but one vanish.
   - Result counts per event are therefore not `iter × rounds`, and failure rates can't be measured.
   - Check: count `AtMCResult` entries per event and compare with `iter × rounds`.
   - Fix: `std::multiset`.

### Dormant or latent

6. **The shared space-charge model is mutated per iteration.**
   - `AtMCFission::SimulateEvent` (L582, existing TODO) calls `SetDistortionField` (L586) or `SetLambda` (L590) on the simulation's single model while other threads apply it.
   - `SetDistortionField` reassigns a `std::function` (`AtRadialChargeModel.h` L43) that other threads may be calling, which is undefined behaviour and can crash, not just use a stale lambda.
   - Dormant only because current macros leave space charge off; it becomes a race as soon as it is enabled.
7. **Parameter spreads shrink from event to event.** `TruncateSpace` is never undone, and `AtMCFission::SetParamDistributions` (L73–106) resets only `Z`'s spread. Today this affects only `thBeam`, which is sampled but unused (`GetBeamDirSample` is never called), and the uniform spreads, which are 0. It will matter as soon as a macro sets nonzero spreads.
8. **Smaller defects.**
   - `AtStudentDistribution::SampleSpread` builds a fresh `student_t_distribution<>{1}`, so `SetDoF` has no effect.
   - `AtPulse::ApplyNoise` (`AtPulse.cxx` L123) multiplies the signal by `Gaus(0, fNoiseSigma)` instead of adding noise; with nonzero noise the signal is scrambled.
   - `AtClusterize::getNumberOfElectronsGenerated` returns a `Gaus` draw as `uint64_t`; a negative draw is undefined.
   - `RecenterParamDistributions` dereferences `fResults.begin()` on an empty set when `fNumIter == 0`.

## Likely Bottlenecks

Ordered by expected payoff. Measure before fixing.

1. **Global geometry lock.** `AtSimpleSimulation::GetVolume` (L65–75) takes `fGeoMutex` around `gGeoManager->FindNode` on every step (default 1 mm), so stepping is effectively serialized across threads. Per-thread `TGeoNavigator`s would remove the lock.
2. **Static split.** Iteration cost varies widely (fragment Z, track length, Minuit convergence), so the slowest block sets the round time. A shared atomic counter would balance the load.
3. **Thread churn.** Threads are spawned and joined `fNumRounds` times per event, and `thread_local` objects (point array, RNG, PSA histogram) are rebuilt each time, including a reseed and log line per thread. A persistent pool avoids this.
4. **Logging in hot paths.** `LOG(info)` per step in `AtELossTable::GetEnergy` (L93), likely the worst; per iteration in `AtMCFission::ObjectiveFunction` (L115), `AtSimpleSimulation::NewEvent` (L169) and `AtPulse::GenerateEvent` (L71); per failed pad fit in `AtPSADeconvFit` (L83); per out-of-volume point in `AtClusterizeLine` (L38); per thread per round in `AtMCFitter::RunRound` (L133) and `AtParameterDistribution::Sample` (L22).
5. **Copies.** `SimulateEvent` returns a `TClonesArray` by value (L669). `AtMCFission::ObjectiveFunction` (L111) deep-copies the experimental event every iteration (`auto` instead of `const auto &`).

Until these are addressed, running independent processes over event ranges scales better than adding threads and sidesteps bugs 3 and 6.
