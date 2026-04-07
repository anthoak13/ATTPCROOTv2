# Data Model

Core runtime objects passed between simulation, unpacking, reconstruction, and fitting.

Most FairRoot branches in this tree are not bare `AtEvent*` or `AtRawEvent*` objects. They are `TClonesArray` branch containers, usually holding one event object at slot `0`. Agents should treat the branch name and the contained event class as related but distinct concepts.

## Reconstruction Objects

| Object | Typical branch container | Main producer | Main consumer | Persisted | Key downstream fields |
|--------|--------------------------|---------------|---------------|-----------|-----------------------|
| `AtRawEvent` | `TClonesArray` branch such as `AtRawEvent` / `AtRawEventFiltered` | `AtUnpackTask`, `AtPulseTask` | `AtFilterTask`, `AtPSAtask` | yes | pad traces, aux/FPN pads, good flag, optional MC map |
| `AtEvent` | `TClonesArray` branch such as `AtEventH` / `AtEventCleaned` | `AtPSAtask` | `AtDataCleaningTask`, `AtPRAtask`, `AtSampleConsensusTask` | yes | hit list, event charge, mesh signal |
| `AtPatternEvent` | `TClonesArray` branch such as `AtPatternEvent` / `AtPatternEventTransformed` | `AtPRAtask`, `AtPatternFindingTask`, `AtSampleConsensusTask`, `AtPatternTransformTask` | `AtPatternTransformTask`, `AtFitterTask`, `AtMCFitterTask` | yes | candidate `AtTrack` vector (`fTrackCand`), noise hit vector (`fNoise`) |
| `AtTrack` | contained inside `AtPatternEvent`, not its own top-level branch | pattern-recognition code | fitters, track-level analysis | yes | track ID, hit/cluster-hit collections, geometric estimates, Bragg-curve values |
| `AtTrackingEvent` | `TClonesArray` branch `AtTrackingEvent` | `AtFitterTask` | downstream analysis | yes | fitted tracks, optional copied track array, event vertex fields |
| `AtFittedTrack` | contained inside `AtTrackingEvent`, not its own top-level branch | `AtFitterTask` | downstream analysis | yes | `Kinematics` struct (kineticEnergy, theta, phi); `TrackProperties` struct (initialPosition, trackLength, estimateTotalCharge, fSmoothedPositions); `ParticleInfo` struct; `fVertex`; see sections below |
| `AtMCResult` | `TClonesArray` branch `AtMCResult` | `AtMCFitterTask` | MC-fitting analysis | yes | objective value and fitted/sampled parameter summary |

## AtTrack Fields

`AtTrack` (`AtData/AtTrack.h`) is the per-candidate-track object inside `AtPatternEvent`. Key fields and which component sets them:

| Field | Setter | Set by |
|-------|--------|--------|
| `fHitArray` | `AddHit` | All `AtPatternFinder` implementations |
| `fHitClusterArray` | `AddClusterHit` | `AtSmooth3DClusterer`, `AtGroupClusterer` |
| `fPattern` (`AtPatterns::AtPattern*`) | `SetPattern` | `AtRANSACPatternFinder` (type depends on `SetPatternType`) |
| `fGeoRadius` | `SetGeoRadius` | `AtCircleSeeder` |
| `fGeoCenter` | `SetGeoCenter` | `AtCircleSeeder` |
| `fGeoThetaAngle` | `SetGeoTheta` | `AtCircleSeeder` |
| `fGeoPhiAngle` | `SetGeoPhi` | `AtCircleSeeder` |

The `fGeo*` fields are the Brho seed inputs for `AtFitterUKF`. They are populated by `AtCircleSeeder` (TriplClust path) or by `AtRANSACPatternFinder` after its internal circle fit.

## AtPattern Hierarchy

`AtPattern` (`AtData/AtPattern/AtPattern.h`) is the abstract base for geometric track shapes. All patterns expose `DistanceToPattern`, `ClosestPointOnPattern`, `FitPattern`, and `GetPatternPar`.

| Concrete class | `PatternType` enum | Key accessors | Header |
|----------------|--------------------|---------------|--------|
| `AtPatternLine` | `kLine` | `GetPoint()`, `GetDirection()` | `AtData/AtPattern/AtPatternLine.h` |
| `AtPatternRay` | `kRay` | `GetPoint()`, `GetDirection()` (one-ended) | `AtData/AtPattern/AtPatternRay.h` |
| `AtPatternCircle2D` | `kCircle2D` | `GetCenter()`, `GetRadius()` | `AtData/AtPattern/AtPatternCircle2D.h` |
| `AtPatternY` | `kY` | `GetVertex()`, `GetBeamDirection()`, `GetFragmentDirection(int)`, `GetPointAssignment(XYZPoint)` | `AtData/AtPattern/AtPatternY.h` |
| `AtPatternFission` | `kFission` | same as `AtPatternY`, different minimizer | `AtData/AtPattern/AtPatternFission.h` |

Factory function: `AtPatterns::CreatePattern(PatternType)`.

## AtFittedTrack Layout

`AtFittedTrack` (`AtData/AtFittedTrack.h`) is the per-track output of `AtFitterTask`.

```
AtFittedTrack
├── fTrackID                              from pattern recognition
├── fKinematics[]          Kinematics    { kineticEnergy [MeV], theta [rad], phi [rad] }
├── fKinematicsXtr[]       Kinematics    same, back-extrapolated to beam axis
├── fParticleInfo[]        ParticleInfo  { idPDG [TString], charge, mass [amu] }
├── fVertex[]              XYZVector     vertex position(s)
└── fTrackProperties       TrackProperties
    ├── initialPosition                  position of first cluster
    ├── initialPositionXtr               closest point on track to (0,0)
    ├── extrapolatedDistance             arc length: initialPosition→initialPositionXtr
    ├── distancePOCA                     distance: initialPositionXtr→(0,0)
    ├── trackLength                      arc length from start to Bragg peak
    ├── trackLengthXtr                   arc length from extrapolated start to Bragg peak
    ├── estimateTotalCharge              sum of hit charges
    ├── estimateDeDx                     estimateTotalCharge / trackLengthXtr
    ├── trackPoints                      number of hits
    └── fSmoothedPositions[]             XYZPoint per cluster — UKF smoother output only
```

Accessors use index 0 by default: `GetKinematics(0)`, `GetVertex(0)`. Multiple indices support multi-hypothesis fitting. `GetSmoothedPositions()` is populated by `AtFitterUKF`; it is empty for other fitters.

## AtFitMetadata Layout

`AtFitMetadata` (`AtData/AtFitMetadata.h`) is the optional per-event fit statistics object. Registered as a branch only when `AtFitterTask::SetFitMetadataBranch` is called.

```
AtFitMetadata
├── fEventID              ULong_t
└── fMetadatas            map<trackID, vector<AtFitTrackMetadata>>
    └── AtFitTrackMetadata  { fTrackID, fPValue, fChi2, fNdf, fFitConverged }
```

Multiple `AtFitTrackMetadata` per track support fitting under multiple particle hypotheses.

## Simulation Objects

| Object | Typical branch container | Main producer | Main consumer | Persisted | Notes |
|--------|--------------------------|---------------|---------------|-----------|-------|
| `AtMCPoint` | MC-point branches such as `AtTpcPoint` | Geant/VMC transport, `AtSimpleSimulation` | `AtClusterizeTask` | yes | primary simulation-to-digitization handoff object; also used for MC bookkeeping in simulated-data reconstruction |
| `AtSimulatedPoint` | `TClonesArray` branch `AtSimulatedPoint` | `AtClusterizeTask` | `AtPulseTask` | yes | intermediate ionization-electron / charge-cluster representation between MC points and pad traces |
| `AtMCTrack` | `TClonesArray` branch `MCTrack` | simulation stack | analysis, truth matching | yes | simulated particle tracks |
| `AtVertexPropagator` | no FairRoot branch | generators/runtime simulation logic | generators and downstream simulation code | no | singleton shared state |

## Container Pattern

```text
FairRoot branch name          -> TClonesArray container -> runtime object

AtRawEvent                    -> TClonesArray -> AtRawEvent
AtEventH                      -> TClonesArray -> AtEvent
AtPatternEvent                -> TClonesArray -> AtPatternEvent
AtPatternEventTransformed     -> TClonesArray -> AtPatternEvent
AtTrackingEvent               -> TClonesArray -> AtTrackingEvent
AtFitMetadata                 -> TClonesArray -> AtFitMetadata
MCTrack                       -> TClonesArray -> AtMCTrack
AtTpcPoint                    -> TClonesArray -> AtMCPoint
AtSimulatedPoint              -> TClonesArray -> AtSimulatedPoint
```

For the tasks that produce and consume these objects, see [branch-io-contracts.md](branch-io-contracts.md). For pipeline order, see [simulation-pipeline.md](../subsystems/simulation-pipeline.md) and [reconstruction-pipeline.md](../subsystems/reconstruction-pipeline.md). For pattern recognition pipeline architecture, see [../subsystems/pattern-recognition.md](../subsystems/pattern-recognition.md).
