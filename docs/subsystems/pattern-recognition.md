# Pattern Recognition

Pattern recognition converts a hit cloud (`AtEvent`) into candidate tracks (`AtPatternEvent`). The subsystem has two usage modes: the monolithic `AtPRAtask` convenience class (backwards-compatible, TriplClust with full transform chain), and the composable `AtPatternFindingTask` + `AtPatternTransformTask` pipeline (fully configurable). Both produce the same output type: `AtPatternEvent → TClonesArray[AtPatternEvent]`.

All pattern recognition components live in the `AtPATTERN` namespace.

## Interfaces

**`AtPatternFinder`** (`AtReconstruction/AtPatternRecognition/AtPatternFinder.h`)
```cpp
virtual std::unique_ptr<AtPatternEvent> FindTracks(AtEvent &event) = 0;
```
Takes a hit cloud, returns an `AtPatternEvent` with raw track candidates. Output tracks contain only raw hits — no clustering, ordering, or geometric seeding.

**`AtPatternTransform`** (`AtReconstruction/AtPatternRecognition/AtPatternTransform.h`)
```cpp
virtual void Transform(AtPatternEvent &event) = 0;
```
Modifies an `AtPatternEvent` in place. May cluster hits, order clusters, reject tracks, merge fragments, or seed geometric parameters.

## AtPatternFinder Implementations

### `AtTrackFinderTC` — TriplClust
**Header:** `AtReconstruction/AtPatternRecognition/AtTrackFinderTC.h`

Hierarchical clustering algorithm (Dalitz et al.) that assigns hits to raw track candidates.

| Setter | Default | Meaning |
|--------|---------|---------|
| `SetScluster(float s)` | 0.3 | Scale parameter for triplet clustering |
| `SetKtriplet(size_t k)` | 19 | Number of triplet neighbors |
| `SetNtriplet(size_t n)` | 2 | Minimum shared triplet pairs |
| `SetMcluster(size_t m)` | 15 | Minimum cluster size |
| `SetRsmooth(float r)` | 2.0 | Smoothing radius |
| `SetAtriplet(float a)` | 0.03 | Angular threshold |
| `SetTcluster(float t)` | 4.0 | Distance threshold |

### `AtRANSACPatternFinder` — Sample Consensus
**Header:** `AtReconstruction/AtPatternRecognition/AtRANSACPatternFinder.h`

Wraps `AtSampleConsensus::Solve`. Supports multiple estimators and pattern types. Does not populate `fGeo*` fields — add `AtCircleSeeder` as a subsequent transform if a fitter needs geometric seeds.

| Setter | Notes |
|--------|-------|
| `SetPatternType(AtPatterns::PatternType)` | `kLine`, `kRay`, `kCircle2D`, `kY`, `kFission` |
| `SetEstimator(SampleConsensus::Estimators)` | `RANSAC`, `LMedS`, `MLESAC`, `WRANSAC`, `Chi2`, `YRANSAC` |
| `SetNumIterations(int n)` | RANSAC iterations |
| `SetMinHitsPattern(int n)` | Minimum hits to accept a pattern |
| `SetDistanceThreshold(double mm)` | Inlier distance threshold |
| `SetChargeThreshold(double q)` | Minimum hit charge |

## AtPatternTransform Implementations

Transform steps are applied in sequence after the finder. The standard TriplClust chain (used by `AtPRAtask`) applies them in the order listed below.

### `AtSmooth3DClusterer`
**Header:** `AtReconstruction/AtPatternRecognition/AtSmooth3DClusterer.h`

Two-pass smoothed 3D clustering per track. First pass clusters hits by proximity to reference points; second pass re-clusters at midpoints between adjacent clusters with half the radius. Covariance matrices incorporate detector diffusion and pad resolution.

```cpp
AtSmooth3DClusterer(double radius = 20.0, double distance = 15.0)
```

| Setter | Default | Notes |
|--------|---------|-------|
| `SetRadius(double mm)` | 20.0 | First-pass clustering radius |
| `SetDistance(double mm)` | 15.0 | Second-pass clustering distance |
| `SetCovarianceMode(AtTools::CovarianceMode)` | `TransformerDirect` | How cluster covariances are computed |
| `SetDiffusionParams(coefT, coefL, driftVel, tbTime, padResXY)` | 0.00009, 0.0000009, 1.0 cm/us, 0.320 us, 2.3 mm | Detector physics for covariance |

Also exposes `ClusterizeTrack(AtTrack &track)` for single-track use (called internally by `AtFragmentMerger`).

### `AtGroupClusterer`
**Header:** `AtReconstruction/AtPatternRecognition/AtGroupClusterer.h`

Simpler alternative: groups `hitsPerCluster` consecutive hits into a charge-weighted centroid. No smoothing pass.

```cpp
AtGroupClusterer(int hitsPerCluster = 15)
```

| Setter | Default |
|--------|---------|
| `SetHitsPerCluster(int n)` | 15 |
| `SetCovarianceMode(AtTools::AtTrackTransformer::CovarianceMode)` | `TransformerDirect` |
| `SetDiffusionParams(coefT, coefL, driftVel, tbTime, padResXY)` | same as AtSmooth3DClusterer |

### `AtClusterOrderer`
**Header:** `AtReconstruction/AtPatternRecognition/AtClusterOrderer.h`

Nearest-neighbor walk starting from the highest-Z cluster (vertex end). No configuration parameters. Also exposes `OrderTrack(AtTrack &track)` for single-track use.

### `AtBeamTrackRejector`
**Header:** `AtReconstruction/AtPatternRecognition/AtBeamTrackRejector.h`

Removes tracks whose lab scattering angle is below `minLabTheta` (forward beam-like) or above `180 - minLabTheta` (backward beam-like). Angle is computed from the vector between vertex-end and far-end clusters.

```cpp
AtBeamTrackRejector(double minLabTheta = 10.0)
```

Setter: `SetMinLabTheta(double deg)`.

### `AtFragmentMerger`
**Header:** `AtReconstruction/AtPatternRecognition/AtFragmentMerger.h`

Identifies primary tracks (front cluster within `vertexRadiusXY` of the beam axis and within `vertexZTolerance` mm in Z of the maximum front-cluster Z). For each primary, finds non-primary fragments whose nearest endpoint is within `mergeDist` mm of the primary's far end and appends their hits. Repeats until no further merges are possible. Non-merged non-primaries remain in the event — use `AtVertexTrackSelector` afterward to remove them.

```cpp
AtFragmentMerger(double mergeDist = 30.0, double vertexRadiusXY = 80.0)
```

| Setter | Default |
|--------|---------|
| `SetMergeDist(double mm)` | 30.0 |
| `SetVertexRadiusXY(double mm)` | 80.0 |
| `SetVertexZTolerance(double mm)` | 50.0 |
| `SetClusterer(AtSmooth3DClusterer *)` | nullptr — if set, re-clusters after each merge |
| `SetOrderer(AtClusterOrderer *)` | nullptr — if set, re-orders after each merge |

Setting a non-owning clusterer/orderer causes post-merge re-clustering so the next fragment search uses an up-to-date far-end position.

### `AtVertexTrackSelector`
**Header:** `AtReconstruction/AtPatternRecognition/AtVertexTrackSelector.h`

Keeps only tracks whose vertex-end cluster is within `vertexRadiusXY` mm of the beam axis AND within `vertexZTolerance` mm in Z of the maximum front-cluster Z.

```cpp
AtVertexTrackSelector(double vertexRadiusXY = 80.0)
```

Setters: `SetVertexRadiusXY(double mm)`, `SetVertexZTolerance(double mm)` (default 50.0).

### `AtCircleSeeder`
**Header:** `AtReconstruction/AtPatternRecognition/AtCircleSeeder.h`

Sets `GeoCenter`, `GeoRadius`, `GeoPhi`, and `GeoTheta` on each track. Fits a 2D circle to hits near the vertex end for center/radius/phi; then fits arc-length vs Z for theta. Required before any fitter that reads `fGeo*` fields for Brho seeding.

| Setter | Default |
|--------|---------|
| `SetRadiusFitFraction(double frac)` | 1.0 — fraction of hits (from vertex) used for circle fit |
| `SetMinHitsRadius(int n)` | 3 |
| `SetMaxHitsRadius(int n)` | 1000 |

### `AtTrackPruner`
**Header:** `AtReconstruction/AtPatternRecognition/AtTrackPruner.h`

Removes outlier hits using k-nearest-neighbor distances within each track. If `(mean_kNN_dist + stdDev * stdDevMul) > kNNDist`, the hit is removed.

| Setter | Default |
|--------|---------|
| `SetKNN(int k)` | 5 |
| `SetStdDevMul(double mul)` | 0.0 |
| `SetKNNDist(double dist)` | 10.0 mm |

## `AtTransformChain`
**Header:** `AtReconstruction/AtPatternRecognition/AtTransformChain.h`

Non-owning composite `AtPatternTransform`. Applies registered steps in insertion order. `Add()` silently skips null pointers (safe for optional steps). Callers retain ownership of all added steps.

```cpp
AtTransformChain chain;
chain.Add(&clusterer);
chain.Add(&orderer);
chain.Add(&seeder);
chain.Transform(event); // applies all three in order
```

`AtPRAtask` builds this chain internally during `Init()`. `AtTransformChain` itself implements `AtPatternTransform`, so it can be passed to `AtPatternTransformTask`.

## FairTask Wrappers

### `AtPatternFindingTask`
**Header:** `AtReconstruction/AtPatternFindingTask.h`

Runs one `AtPatternFinder` per event. Constructor takes ownership of the finder.

```cpp
AtPatternFindingTask(std::unique_ptr<AtPATTERN::AtPatternFinder> finder)
```

| Setter | Default |
|--------|---------|
| `SetInputBranch(TString)` | `"AtEventH"` |
| `SetOutputBranch(TString)` | `"AtPatternEvent"` |
| `SetPersistence(bool)` | false |
| `SetMinNumHits(int)` | 10 — events below this are skipped |
| `SetMaxNumHits(int)` | 5000 — events above this are skipped |

### `AtPatternTransformTask`
**Header:** `AtReconstruction/AtPatternTransformTask.h`

Copies the input `AtPatternEvent` to a new output branch and calls `Transform()` on the copy. Constructor takes ownership of the transform.

```cpp
AtPatternTransformTask(std::unique_ptr<AtPATTERN::AtPatternTransform> transform)
```

| Setter | Default |
|--------|---------|
| `SetInputBranch(TString)` | `"AtPatternEvent"` |
| `SetOutputBranch(TString)` | `"AtPatternEventTransformed"` |
| `SetPersistence(bool)` | false |

The default output branch is intentionally different from the default input so chained tasks do not overwrite each other's output. Wire branch names explicitly when connecting multiple transform tasks. To apply several transforms without creating multiple intermediate branches, wrap them in a single `AtTransformChain` passed to one `AtPatternTransformTask`.

**Ownership caveat:** `AtTransformChain` holds non-owning pointers. When a chain is passed to `AtPatternTransformTask`, the task owns the chain, but the individual transform objects the chain points to must remain alive for the full `fRun->Run()` call. In ROOT macros, keep the `unique_ptr` objects as local variables in the same scope as `fRun->Run()`.

### `AtPRAtask` (convenience class)
**Header:** `AtReconstruction/AtPRAtask.h`

Backwards-compatible monolithic task. Internally constructs `AtTrackFinderTC` and owns the full transform chain (optional `AtTrackPruner`, `AtSmooth3DClusterer`, `AtClusterOrderer`, `AtBeamTrackRejector`, `AtFragmentMerger`, `AtVertexTrackSelector`, `AtCircleSeeder`). Exposes all finder and transform parameters directly.

| Setter | Default |
|--------|---------|
| `SetInputBranch(TString)` | `"AtEventH"` |
| `SetOutputBranch(TString)` | `"AtPatternEvent"` |
| `SetPersistence(bool)` | false |
| TriplClust params (SetKtriplet, SetMcluster, etc.) | see AtTrackFinderTC above |
| `SetClusterRadius(double mm)` | 20.0 |
| `SetClusterDistance(double mm)` | 15.0 |
| `SetMinLabTheta(double deg)` | 10.0 |
| `SetVertexRadiusXY(double mm)` | 80.0 |
| `SetMergeDist(double mm)` | 30.0 |
| `SetDiffusionParams(...)` | propagated to clusterer |

Use `AtPRAtask` for standard TriplClust+full-chain processing. Use the modular approach when you need a different finder, a partial chain, per-step branch inspection, or a custom transform ordering.

## Usage Examples

### Monolithic — `AtPRAtask`

```cpp
auto *praTask = new AtPRAtask();
praTask->SetInputBranch("AtEventCleaned"); // or "AtEventH"
praTask->SetOutputBranch("AtPatternEvent");
praTask->SetPersistence(kTRUE);
// Optional tuning:
praTask->SetKtriplet(19);
praTask->SetMcluster(15);
praTask->SetClusterRadius(20.0);
praTask->SetClusterDistance(15.0);
praTask->SetMinLabTheta(10.0);
praTask->SetVertexRadiusXY(80.0);
praTask->SetMergeDist(30.0);
fRun->AddTask(praTask);
```

### Modular — `AtPatternFindingTask` + `AtTransformChain`

```cpp
// 1. Create and configure the finder
auto finder = std::make_unique<AtPATTERN::AtTrackFinderTC>();
finder->SetKtriplet(19);
finder->SetMcluster(15);

auto *findTask = new AtPatternFindingTask(std::move(finder));
findTask->SetInputBranch("AtEventH");
findTask->SetOutputBranch("AtRawPatterns");
findTask->SetMinNumHits(10);
fRun->AddTask(findTask);

// 2. Create transform components (keep unique_ptrs alive until Run() completes)
auto clusterer = std::make_unique<AtPATTERN::AtSmooth3DClusterer>(20.0, 15.0);
auto orderer   = std::make_unique<AtPATTERN::AtClusterOrderer>();
auto rejector  = std::make_unique<AtPATTERN::AtBeamTrackRejector>(10.0);
auto merger    = std::make_unique<AtPATTERN::AtFragmentMerger>(30.0, 80.0);
merger->SetClusterer(clusterer.get()); // non-owning: re-clusters after each merge
merger->SetOrderer(orderer.get());
auto selector  = std::make_unique<AtPATTERN::AtVertexTrackSelector>(80.0);
auto seeder    = std::make_unique<AtPATTERN::AtCircleSeeder>();

// 3. Build chain (non-owning pointers into the unique_ptrs above)
auto chain = std::make_unique<AtPATTERN::AtTransformChain>();
chain->Add(clusterer.get());
chain->Add(orderer.get());
chain->Add(rejector.get());
chain->Add(merger.get());
chain->Add(selector.get());
chain->Add(seeder.get());

// 4. Wrap in a task — task owns the chain, but NOT the individual transforms
auto *xformTask = new AtPatternTransformTask(std::move(chain));
xformTask->SetInputBranch("AtRawPatterns");
xformTask->SetOutputBranch("AtPatternEvent");
xformTask->SetPersistence(kTRUE);
fRun->AddTask(xformTask);

// 5. clusterer, orderer, ... must remain alive here when fRun->Run() is called
fRun->Run(0, nEvents);
```

## Key File Locations

| Component | Header |
|-----------|--------|
| `AtPatternFinder` (interface) | `AtReconstruction/AtPatternRecognition/AtPatternFinder.h` |
| `AtPatternTransform` (interface) | `AtReconstruction/AtPatternRecognition/AtPatternTransform.h` |
| `AtTransformChain` | `AtReconstruction/AtPatternRecognition/AtTransformChain.h` |
| `AtTrackFinderTC` | `AtReconstruction/AtPatternRecognition/AtTrackFinderTC.h` |
| `AtRANSACPatternFinder` | `AtReconstruction/AtPatternRecognition/AtRANSACPatternFinder.h` |
| `AtSmooth3DClusterer` | `AtReconstruction/AtPatternRecognition/AtSmooth3DClusterer.h` |
| `AtGroupClusterer` | `AtReconstruction/AtPatternRecognition/AtGroupClusterer.h` |
| `AtClusterOrderer` | `AtReconstruction/AtPatternRecognition/AtClusterOrderer.h` |
| `AtBeamTrackRejector` | `AtReconstruction/AtPatternRecognition/AtBeamTrackRejector.h` |
| `AtFragmentMerger` | `AtReconstruction/AtPatternRecognition/AtFragmentMerger.h` |
| `AtVertexTrackSelector` | `AtReconstruction/AtPatternRecognition/AtVertexTrackSelector.h` |
| `AtCircleSeeder` | `AtReconstruction/AtPatternRecognition/AtCircleSeeder.h` |
| `AtTrackPruner` | `AtReconstruction/AtPatternRecognition/AtTrackPruner.h` |
| `AtPatternFindingTask` | `AtReconstruction/AtPatternFindingTask.h` |
| `AtPatternTransformTask` | `AtReconstruction/AtPatternTransformTask.h` |
| `AtPRAtask` | `AtReconstruction/AtPRAtask.h` |
