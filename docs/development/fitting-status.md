# Fitting Status

The UKF fitter is the current primary fitting path on this branch. It is implemented, validated, and the default choice for new analysis.

## Current State

**Primary fitter:** `EventFit::AtFitterUKF` (Unscented Kalman Filter with RTS smoother)
- Validated on 1000-event digitized ¹⁶C(p,p) test suite: ~98% convergence
- Full FairRoot integration via `AtFitterTask`
- Configuration documented in [UKF.md](UKF.md) and [reconstruction-pipeline.md](../subsystems/reconstruction-pipeline.md)

**Legacy fitters (not default, not actively maintained):**
- `AtFITTER::AtFitterOld` / `AtFITTER::AtGenfit` — GenFit2-based; present for backward compatibility
- `AtMCFitter` / `AtMCFission` — Monte Carlo sampling fitter; used for cross-checks

## History

At the start of this branch no stable fitting architecture existed. The branch was opened to integrate OpenKF/UKF into ATTPCROOT. The fitter became stable as clustering, ordering, and momentum seeding improved — cluster quality turned out to dominate fit convergence more than the filter equations themselves.
