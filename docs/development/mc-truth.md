# Monte Carlo Truth

Design notes on how simulated ground truth is recorded and carried to downstream stages. Status: **under discussion, nothing implemented**.

## Problem

Validation studies (e.g. "does the fitter recover the simulated Z?") need per-event truth next to reconstructed or fit results. Simulation, digitization and fitting are usually separate `FairRunAna` jobs writing separate files, so truth has to survive every stage.

This is a general problem, not specific to one analysis. Anyone who simulates with `AtSimpleSimulation` and then reconstructs hits it.

## Current State

| Path | Truth written | Notes |
|------|---------------|-------|
| Geant4/VMC | `MCTrack` (`AtMCTrack`, via `AtStack::Register`), `MCEventHeader.` (stock FairRoot), per-step `AtMCPoint` | `AtMCPoint` repeats `fEnergyIni`, `fAngleIni`, `fAiso`, `fZiso` on every step. `AtVertexPropagator` holds reaction-level truth (vertex, beam energy, Ex, recoil/scatter kinematics) but is never persisted. |
| `AtSimpleSimulation` | `AtMCPoint` steps only | Records no tracks or primaries. `AddHit` does not set `fAiso` or `fZiso`, even though Z and A are known. |

Downstream carry-forward:

- `AtCopyTreeTask` (`AtReconstruction/`) re-registers input branches as outputs. It handles only `TClonesArray`, copies all branches or none, and its header warns it must be the first task because it relies on fragile `FairRootManager` ordering.
- `AtPulseTask::SetPersistenceAtTpcPoint` forwards `AtTpcPoint` only.
- Most macros avoid the problem by running every stage in one job, or by adding the upstream tree as a friend and aligning by entry index.

### `AtMCTrack` Is Effectively Unused

It is written by `AtStack` and read by nothing in the library. The only macro reference is the auto-generated `macro/Simulation/ATTPC/d2He/Test_Ana_d2He.h`. The `FairMCTracks` in `eventDisplay.C` macros is FairRoot's trajectory display, not this branch.

It is also a poor fit for this repo's physics:

- It stores a PDG code instead of Z and A, and `GetMass()`/`GetEnergy()` use `TDatabasePDG`, which returns 0 for heavy ions.
- It uses cm and GeV, while everything after the Geant4 boundary uses mm and MeV (see [units-and-conventions.md](units-and-conventions.md)).
- It is GSI/FairRoot boilerplate coupled to `FairLink`.

Do not build new truth handling on `AtMCTrack`.

### Known Workaround to Remove

`macro/e12014/skyler/eventSim.h` stores truth in a `TH1D` with labeled bins (`Info`), forwards it as `SimInfo` via `DigiSimInfo.h`, and `plot_fit.C` adds the digi tree as a friend of the fit tree to read it. Fields are addressed by bin index, integers are stored as doubles, and only one fragment is recorded (the other is inferred from the fit, which is circular). Do not copy this pattern.

## Direction Under Discussion

1. **A lean, repo-owned truth class.** One entry per simulated particle: Z, A, mother ID, and start position and momentum in mm and MeV, with the z convention documented. No PDG code, particle table or FairRoot dependency beyond being a persistable `TObject`.
   - `AtSimpleSimulation` writes it first.
   - `AtStack` could write it later, so both simulation paths share one format. Replacing `MCTrack` is cheap because nothing reads it.
   - Derived quantities (vertex, beam energy at the vertex, CoM angles) are computed in analysis from the particle records, not stored.
2. **Keep FairRoot coupling thin.** Truth is plain data owned by the simulation class, and FairRoot registration stays a one-line optional layer (like `AtSimpleSimulation::RegisterBranch`). The long-term goal is to drop FairRoot, so avoid new designs that depend on its I/O manager behavior.

## Open Questions

- **Carry truth forward or link back?** Either copy truth into every downstream file (e.g. `AtCopyTreeTask` extended with a branch-name list, which builds on the fragile mechanism above), or give every event a stable ID shared across stages and join at analysis time. Linking back keeps files small and does not depend on FairRoot's branch copying.
- **Name and exact fields of the truth class.** Should it also cover what `AtVertexPropagator` knows today (excitation energies, reaction channel)?
- **Event alignment.** Not verified: whether `AtMCFitterTask` output keeps exactly one entry per input event when the fitter skips an event. Friend-tree joins by entry index depend on it.
