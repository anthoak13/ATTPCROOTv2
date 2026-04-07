# Reconstruction Pipeline

The reconstruction pipeline converts raw pad traces into hit clouds, candidate tracks, and fitted track results. Each stage is a `FairTask` that reads and writes branches through `FairRootManager`. Branches are `TClonesArray` containers; tasks read the event object at slot `0`.

## Full Pipeline

```text
AtUnpackTask
  │  reads raw GET/GRAW, HDF5, or ROOT data
  └─ output: AtRawEvent → TClonesArray[AtRawEvent]
        ▼
AtFilterTask   [optional]
  │  filters raw traces
  └─ output: AtRawEventFiltered → TClonesArray[AtRawEvent]
        ▼
AtPSAtask
  │  converts traces into hits via a pluggable AtPSA algorithm
  └─ output: AtEventH → TClonesArray[AtEvent]
        ▼
AtDataCleaningTask   [optional]
  │  removes or adjusts hits before pattern recognition
  └─ output: AtEventCleaned → TClonesArray[AtEvent]
        ▼
Pattern recognition stage  (choose one path — see below)
  └─ output: AtPatternEvent → TClonesArray[AtPatternEvent]
        ▼
AtFitterTask
  │  fits each AtTrack; stores results in AtTrackingEvent
  └─ output: AtTrackingEvent → TClonesArray[AtTrackingEvent]
             AtFitMetadata   → TClonesArray[AtFitMetadata]   [optional]
```

Branch names are defaults. Most tasks expose `SetInputBranch` / `SetOutputBranch`. See [branch-io-contracts.md](../reference/branch-io-contracts.md).

## Pattern Recognition Stage

Three paths produce `AtPatternEvent`. See [pattern-recognition.md](pattern-recognition.md) for full detail on the composable pipeline.

**Path A — `AtPRAtask` (monolithic, TriplClust)**

Convenience class. Internally constructs `AtTrackFinderTC` and applies the full transform chain (optional pruning, clustering, ordering, beam rejection, fragment merging, vertex selection, Brho seeding). Output tracks have `fHitClusterArray` populated and `fGeo*` fields filled — ready for `AtFitterUKF`.

**Path B — `AtPatternFindingTask` + `AtPatternTransformTask` (composable)**

`AtPatternFindingTask` wraps any `AtPATTERN::AtPatternFinder`. One or more `AtPatternTransformTask` instances (or a single task holding an `AtTransformChain`) apply post-processing. Provides the same per-step components as `AtPRAtask` with mix-and-match freedom and per-step branch inspection.

**Path C — `AtSampleConsensusTask` (RANSAC, legacy)**

Sample-consensus pattern finding. Does not populate `fGeo*` fields by default; add `AtCircleSeeder` (via `AtPatternTransformTask`) before running a Brho-seeded fitter.

## Fitting Stage

`AtFitterTask` (`AtReconstruction/AtFitterTask.h`) wraps any `EventFit::AtFitter`. It reads `AtPatternEvent`, fits each `AtTrack`, and writes `AtTrackingEvent`. An optional `AtFitMetadata` branch is registered only when `SetFitMetadataBranch` has been called.

### `AtFitterUKF` — Unscented Kalman Filter
**Header:** `AtReconstruction/AtFitter/AtFitterUKF.h`

The primary fitter on this branch. For each `AtTrack`:
1. Seeds initial momentum from the circle-fit Brho (`fGeoRadius`, `fGeoCenter`).
2. Runs the UKF forward pass (predict + correct per cluster).
3. Runs the RTS smoother (backward pass).
4. Returns an `AtFittedTrack` with vertex kinematics and smoothed cluster positions.

Constructor:
```cpp
// namespace EventFit
AtFitterUKF(double charge_coulombs, double mass_MeV,
            std::unique_ptr<AtTools::AtELossModel> elossModel)
```

Key configuration:

| Setter | Default | Notes |
|--------|---------|-------|
| `SetBField(XYZVector)` | `{0, 0, 2.85}` T | Solenoidal field along Z |
| `SetEField(XYZVector)` | `{0, 0, 0}` V/m | Electric drift correction |
| `SetUKFParameters(alpha, beta, kappa)` | `1e-3, 2.0, 0.0` | Sigma-point scaling |
| `SetMeasurementSigma(double mm)` | 1.0 | Position measurement sigma |
| `SetMomentumSigmaFrac(double frac)` | 0.1 | Fractional initial momentum uncertainty |
| `SetMinClusters(int n)` | 3 | Skip tracks below this cluster count |
| `SetEnableEnergyStraggling(bool)` | true | Requires ELoss model with `GetRangeVariance()`; disable for SRIM tables |
| `SetNIterations(int n)` | 1 | >1 re-seeds from previous result with tighter covariance |
| `SetZPadPlane(double mm)` | -1 (off) | If >0, converts digi Z to lab: `Z_lab = ZPadPlane - Z_digi` |
| `SetAdaptiveClustering(bool)` | true | Re-clusters hits based on Brho momentum estimate |
| `SetMomentumSeed(double MeV/c)` | -1 (off) | If >0, overrides Brho seed |
| `SetUsePerClusterCov(bool)` | false | Use per-cluster covariance from `AtHitCluster::GetCovMatrix()` |
| `SetClusterCovarianceMode(CovarianceMode)` | `TransformerDirect` | Covariance computation mode |

Energy loss models (`AtTools/AtELossModel.h`): `AtELossCATIMA` supports straggling; `AtELossTable` (SRIM table) does not — call `SetEnableEnergyStraggling(false)` when using it.

## Full Chain Macro Example

Condensed from `macro/tests/AT-TPC/run_pra_ukf_attpc.C` (real data):

```cpp
// Pattern recognition
auto *praTask = new AtPRAtask();
praTask->SetInputBranch("AtEventCleaned");
praTask->SetOutputBranch("AtPatternEvent");
praTask->SetPersistence(kTRUE);

// Energy loss model (SRIM table — disable straggling)
auto eloss = std::make_unique<AtTools::AtELossTable>(0);
eloss->LoadSrimTable("resources/energy_loss/HinH.txt");

// UKF fitter for protons
auto ukfFitter = std::make_unique<EventFit::AtFitterUKF>(
    1.602176634e-19, 938.272, std::move(eloss));
ukfFitter->SetBField({0, 0, 2.85});
ukfFitter->SetUKFParameters(1e-3, 2, 0);
ukfFitter->SetMeasurementSigma(1.0);
ukfFitter->SetMomentumSigmaFrac(0.1);
ukfFitter->SetMinClusters(3);
ukfFitter->SetEnableEnergyStraggling(false);

auto *fitterTask = new AtFitterTask(std::move(ukfFitter));
fitterTask->SetInputBranch("AtPatternEvent");
fitterTask->SetOutputBranch("AtTrackingEvent");
fitterTask->SetPersistence(kTRUE);

fRun->AddTask(praTask);
fRun->AddTask(fitterTask);
```

For the full simulation chain (clusterize + pulse + PSA + PRA + UKF), see `macro/Simulation/ATTPC/16C_pp/run_reco_ukf.C`.

For runtime object fields, see [data-model.md](../reference/data-model.md). For branch names, see [branch-io-contracts.md](../reference/branch-io-contracts.md). For pattern recognition pipeline detail, see [pattern-recognition.md](pattern-recognition.md). For example macros, see [macro-cookbook.md](../reference/macro-cookbook.md).
