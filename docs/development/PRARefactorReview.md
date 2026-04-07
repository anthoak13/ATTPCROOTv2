# PRA Refactor Behavioral Equivalence Review

Review of commit `89d8f071` — "Refactor AtPRA into composable AtPatternFinder/AtPatternTransform pipeline"

## 1. Algorithmic Summary

### Before

`AtTrackFinderTC::FindTracks()` was a monolithic pipeline:

1. TriplClust hierarchical clustering producing raw clusters
2. Per-track: `ClusterizeSmooth3D` -> `OrderClustersAlongTrack` -> (optionally) `PruneTrack`
3. `SelectAndMergeTracks`: beam rejection -> primary identification -> fragment merging (with per-merge re-cluster+reorder) -> non-primary removal
4. Per-track: `SetTrackInitialParameters` (circle+line RANSAC seeding)

### After

`AtTrackFinderTC::FindTracks()` returns raw tracks (hits only, no clustering). `AtPRAtask::Exec()` runs a chain of `AtPatternTransform` steps:

1. `fClusterer->Transform` (AtSmooth3DClusterer)
2. `fPruner->Transform` (AtTrackPruner) — optional
3. `fOrderer->Transform` (AtClusterOrderer)
4. `fBeamRejector->Transform` (AtBeamTrackRejector)
5. `fFragmentMerger->Transform` (AtFragmentMerger) — internally re-clusters+reorders after each merge
6. `fVertexSelector->Transform` (AtVertexTrackSelector)
7. `fSeeder->Transform` (AtCircleSeeder)

### Invariants assumed preserved

Same clustering algorithm (delegated to shared `AtTools::ClusterizeSmooth3D`), same ordering algorithm, same beam rejection logic, same merge logic, same seeding logic. Per-track vs batch processing does not change results because each transform processes tracks independently.

## 2. Behavioral Equivalence Risks

### RISK 1 (LOW): Pruning runs before ordering in new code

**Old:** Per-track: cluster -> order -> prune.
**New:** Cluster all -> prune all -> order all.

However, `PruneTrack`/`AtTrackPruner` operates on the raw hit array, not on clusters, so the cluster ordering is irrelevant to it. **No behavioral change.**

### RISK 2 (MEDIUM): BeamTrackRejector rejects tracks with < 2 clusters; old code checked direction magnitude

**New code** (`AtBeamTrackRejector.cxx`):
```cpp
if (cl->size() < 2)
   return true;  // rejected as beam-like
```

**Old code** (`SelectAndMergeTracks`):
```cpp
if (dir.R() < 1e-3)
   return true; // degenerate track
```

Both reject single-cluster tracks (front == back, so dir.R() == 0). But a track with 2+ clusters whose endpoints happen to be < 1e-3 mm apart would NOT be rejected in the new code but WOULD be in the old code. Practically unlikely with real data but a real code path difference.

### RISK 3 (LOW): Fragment merger re-clustering fallback defaults removed

The old `SelectAndMergeTracks` called:
```cpp
fTrackTransformer->ClusterizeSmooth3D(tracks[i],
    fClusterRadius > 0 ? fClusterRadius : 10.0,
    fClusterDistance > 0 ? fClusterDistance : 20.0);
```

The new code always uses the clusterer's configured radius/distance. Since `fClusterRadius` defaults to 20.0 and `fClusterDistance` to 15.0 in `AtPRAtask.h`, the fallback path (radius=10, distance=20) was dead code. **No behavioral change with defaults.** Only diverges if a user explicitly sets `fClusterRadius=0`.

### RISK 4 (NONE): GetSign implementation change in CircleSeeder

Old used a template helper `GetSign(T)`. New inlines `(0 < Y) - (Y < 0)`. Mathematically identical: returns {-1, 0, 1}.

### RISK 5 (NONE): ClusterizeSmooth3D extraction

`AtTools::ClusterizeSmooth3D` is a line-by-line extraction of the old `AtTrackTransformer::ClusterizeSmooth3D`. Same loop structure, same reference-point advancement logic, same smoothing pass, same `radius/2.0`. Bitwise identical.

## 3. Numerical & Ordering Sensitivity

**Container ordering preserved.** The track vector ordering is maintained: TriplClust assigns tracks by `cluster_index` order, and the transform chain processes them in vector order. `erase(remove_if(...))` preserves relative order. The fragment merger uses the same index-based loop as the old code.

**RANSAC non-determinism.** The circle/line RANSAC fits in `AtCircleSeeder` are inherently non-deterministic (random sampling). This was true before and after. No new source of non-determinism.

**Floating-point.** No accumulation order changes. The clustering, covariance, and seeding computations are structurally identical.

## 4. Pipeline Trace

A hit from a 50-hit track in a 3-track event:

| Step | OLD | NEW |
|------|-----|-----|
| TriplClust assigns hit to cluster 1 | Same | Same |
| Hit added to track 1 | Same | Same |
| Track 1 clustered (Smooth3D) | Per-track in `clustersToTrack` | Batch in `fClusterer->Transform` |
| Track 1 ordered | Per-track in `clustersToTrack` | Batch in `fOrderer->Transform` |
| Track 1 pruned (if enabled) | Per-track in `clustersToTrack` | Batch in `fPruner->Transform` |
| Beam rejection | In `SelectAndMergeTracks` | In `fBeamRejector->Transform` |
| Fragment merging | In `SelectAndMergeTracks` | In `fFragmentMerger->Transform` |
| Vertex selection | In `SelectAndMergeTracks` step 4 | In `fVertexSelector->Transform` |
| Seeding | Per-track after `SelectAndMergeTracks` | In `fSeeder->Transform` |

No decision points identified where the main path could diverge.

## 5. Physics Impact Assessment

If any risks materialize:

- **Risk 2 (degenerate direction check):** Could retain a track that would have been rejected, affecting track multiplicity. Very unlikely with real data.
- **Risk 3 (fallback radius=0):** Only affects users who explicitly set `ClusterRadius(0)`. Not a realistic scenario.

**Most likely observable:** No change. The refactor is a clean structural decomposition.

## 6. High-Risk Findings

| # | Risk | Mechanism | Confidence |
|---|------|-----------|------------|
| 1 | **Degenerate-direction check removed** | Old: `dir.R() < 1e-3` rejects any track with near-zero endpoint separation. New: only rejects `< 2 clusters`. A 2-cluster track at the same position would survive beam rejection and reach the angle calculation, possibly producing NaN/inf in `acos`. | Medium — practically unlikely but a real code path difference |
| 2 | **Cluster radius=0 fallback removed** | Old code fell back to radius=10, distance=20 when values were 0. New code uses configured values directly. | Low — requires explicit misconfiguration |
| 3 | **AtTrackFinderTC no longer inherits AtPRA** | Now inherits `AtPatternFinder`. Any downstream code that `dynamic_cast`s to `AtPRA*` will fail. | Medium — depends on user macros |
| 4 | **`fPRA` type narrowed to `AtTrackFinderTC*`** | `AtPRA::SetDiffusionParams`, `SetClusterRadius`, `SetClusterDistance` removed. Old user macros calling these through `AtPRA*` will fail to compile. | Medium — API breaking change |
| 5 | **Non-primary tracks left by AtFragmentMerger** | Old `SelectAndMergeTracks` removed non-primaries in step 4. New code separates this into `AtVertexTrackSelector`. If someone uses `AtFragmentMerger` without `AtVertexTrackSelector`, non-primary tracks survive. Within `AtPRAtask`, both are always called — **no change** for the standard pipeline. | Low for standard use |

## 7. Suggested Validation Tests

1. **Bitwise regression test.** Run the full PRA pipeline on a reference dataset (the integration test `run_pra_sim_integration.C` already exists). Compare track count, cluster positions, geo-parameters (theta, phi, radius, center) to stored baselines.

2. **Edge case: single-cluster track.** Construct a synthetic event with a track that produces exactly 1 cluster after Smooth3D. Verify it is rejected by `AtBeamTrackRejector` the same way it would have been by `SelectAndMergeTracks`.

3. **Edge case: two clusters at identical positions.** Create a track with 2 clusters at the same XYZ. In old code, `dir.R() < 1e-3` would reject it. In new code, it passes the `cl->size() < 2` check. Verify the downstream `acos` does not produce NaN.

4. **Permutation invariance.** Run the same event with hits in different order. Verify TriplClust and the transform chain produce the same output.

5. **Fragment merger isolation test.** Create a 3-track event where one primary has a nearby fragment. Verify merged track's cluster count and positions match the old `SelectAndMergeTracks` output.

6. **API compatibility.** Compile all existing user macros against the new headers to catch removed methods.

---

# PRA Refactor Architectural Review

Review of commit `89d8f071` — design quality, direction, and long-term maintainability.

## 1. Architectural Overview

### Before

A classic god-class hierarchy. `AtPRA` was simultaneously:
- An abstract base class for pattern finding (`FindTracks()`)
- The owner of clustering logic (`SetTrackInitialParameters`, `OrderClustersAlongTrack`)
- The owner of track selection/merging logic (`SelectAndMergeTracks`)
- The owner of outlier pruning (`PruneTrack`, `kNN`)
- The owner of the `AtTrackTransformer` instance (for `ClusterizeSmooth3D`)

`AtTrackFinderTC::clustersToTrack()` was a 70-line function that did everything: build tracks, cluster, order, prune, select/merge, seed. Meanwhile `AtTrackTransformer` in AtTools owned the actual clustering algorithm (ClusterizeSmooth3D) with 250+ lines of covariance math buried in an anonymous namespace.

`AtPRAtask` was the FairTask wrapper that created the right `AtPRA` subclass, configured it, and called `FindTracks()` — which did all of the above in one shot.

### After

Two new interfaces define a two-phase pipeline:
- **`AtPatternFinder`** — takes a hit cloud, returns raw track candidates (hits only)
- **`AtPatternTransform`** — transforms a pattern event in-place

Each monolithic responsibility is extracted into a single-purpose class:

| Old location | New class | Role |
|---|---|---|
| `AtTrackTransformer::ClusterizeSmooth3D` | `AtSmooth3DClusterer` | Hit -> cluster |
| `AtPRA::OrderClustersAlongTrack` | `AtClusterOrderer` | Nearest-neighbor ordering |
| `AtPRA::SelectAndMergeTracks` (beam reject) | `AtBeamTrackRejector` | Angle-based filtering |
| `AtPRA::SelectAndMergeTracks` (merge) | `AtFragmentMerger` | Endpoint-proximity merging |
| `AtPRA::SelectAndMergeTracks` (vertex) | `AtVertexTrackSelector` | Vertex-proximity filtering |
| `AtPRA::SetTrackInitialParameters` | `AtCircleSeeder` | Circle + theta fit seeding |
| `AtPRA::PruneTrack` | `AtTrackPruner` | kNN outlier removal |
| (new) | `AtGroupClusterer` | Fixed-size grouping alternative |

The shared clustering math is extracted into `AtTools::AtTrackClusterBuilder` and the free function `AtTools::ClusterizeSmooth3D()`, giving both `AtTrackTransformer` and `AtSmooth3DClusterer` a single authoritative implementation.

New FairTask wrappers (`AtPatternFindingTask`, `AtPatternTransformTask`) allow arbitrary composition at run time. `AtPRAtask` is updated to orchestrate the new components internally while preserving its public API.

**The core problem being solved:** The old architecture made it impossible to modify, test, or replace any single step of pattern recognition without understanding and touching the entire pipeline. The new design makes each step independently testable, configurable, and replaceable.

## 2. Design Quality Assessment

**Separation of concerns -- significantly improved.** Clustering, ordering, selection, merging, and seeding are now in distinct classes with distinct headers. `AtTrackFinderTC::FindTracks()` went from a function that did everything (find + cluster + order + prune + select/merge + seed) to one that does exactly one thing: assign hits to raw track candidates. This is a major clarity win.

**Abstraction boundaries -- well-chosen at the top level.** The `AtPatternFinder`/`AtPatternTransform` split maps cleanly to the real algorithmic boundary: "which hits belong to which track?" vs "what do we do with these tracks?" The `AtPatternTransform` interface (`void Transform(AtPatternEvent&)`) is about as minimal as it can be -- one method, in-place mutation, no hidden state. This is a good interface for a pipeline step.

However, at the concrete level the boundaries are less clean. `AtFragmentMerger` has a direct dependency on `AtSmooth3DClusterer*` (non-owning pointer) and embeds an `AtClusterOrderer` as a member. This means it internally re-runs two other transforms -- it is not a pure transform, it is a mini-pipeline. More on this below.

**Data flow clarity -- much improved.** The old code had data flow hidden inside `clustersToTrack()` where you had to trace through 70 lines to see that it clustered, then ordered, then optionally pruned, then selected/merged, then seeded. Now the pipeline is explicit in `AtPRAtask::Exec()`:

```
FindTracks -> Cluster -> Prune -> Order -> BeamReject -> Merge -> VertexSelect -> Seed
```

This is one of the strongest aspects of the refactor -- the pipeline is visible and linear.

**Extensibility -- good.** Adding a new pipeline step is: (1) write a class implementing `AtPatternTransform`, (2) either inject it into `AtPRAtask` or compose it via `AtPatternTransformTask`. Swapping the pattern finder is similarly straightforward -- the existing `AtRANSACPatternFinder` demonstrates this. Adding a new clustering algorithm (like `AtGroupClusterer`) is trivially slotted in.

**Coupling -- mostly improved, with one notable exception.** `AtFragmentMerger` is coupled to `AtSmooth3DClusterer` (via raw pointer) and to `AtClusterOrderer` (via embedded member). This is the tightest coupling in the new design and represents a leaked concern: the merger is responsible for re-clustering/re-ordering after each merge, which is really a pipeline concern, not a merging concern. This is discussed further in section 5.

## 3. Algorithm-Structure Alignment

The new structure maps well to the algorithmic stages. Each class name describes exactly what the algorithm does:
- `AtSmooth3DClusterer` -- smoothed 3D clustering
- `AtClusterOrderer` -- nearest-neighbor ordering
- `AtBeamTrackRejector` -- beam-angle filter
- `AtFragmentMerger` -- endpoint-proximity merging
- `AtCircleSeeder` -- circle + dip-angle fit for initial parameters

The physical/algorithmic intent is much easier to find and reason about. Someone asking "how does beam rejection work?" goes to `AtBeamTrackRejector.cxx` (40 lines) instead of hunting through a 400-line method on `AtPRA`.

**Where abstraction improves clarity:** `AtTrackClusterBuilder` extracts the covariance calculation into a builder with a config struct. The old code had this buried in an anonymous namespace with scattered parameters -- now the config is explicit and the builder's methods are named for what they compute (`BuildTransformerDirectClusterStats`, `GetPerHitVariance`).

**Where abstraction introduces unnecessary indirection:** `AtTrackSeeder` is a class with a single public static-like method (`SetTrackInitialParameters`) that takes all its parameters as arguments. It does not implement `AtPatternTransform`, it is not used in the pipeline, and it duplicates `AtCircleSeeder` almost line-for-line. It appears to be dead or transitional code. More below.

## 4. Tradeoffs and Direction

**Flexibility vs. performance.** The in-place mutation model (`void Transform(AtPatternEvent&)`) avoids copies on the main path. `AtPatternTransformTask::Exec()` does copy the event into a new branch, but this is the FairRoot pipeline model and is acceptable. The `AtPRAtask` path avoids this copy entirely. No performance concerns.

**Generality vs. clarity.** The `AtPatternTransform` interface is maximally general -- anything that mutates an `AtPatternEvent` fits. This is appropriate here because the pipeline stages genuinely are heterogeneous (clustering, filtering, ordering, seeding). A more typed pipeline (where each stage declares its inputs and outputs) would be over-engineered for 8 stages.

**Direction: toward a modular pipeline.** This is a pipeline decomposition, not a framework. The pipeline is explicit in code (either in `AtPRAtask::Exec()` or via FairTask composition), not driven by configuration files or registries. This is the right choice for this codebase -- it's a physics analysis framework, not a general-purpose data processing engine. The direction is coherent and sustainable.

**Backward compatibility was maintained.** `AtPRAtask` still exposes all the old setter methods (`SetClusterRadius`, `SetClusterDistance`, etc.) and builds default instances of each component in `Init()`. Old macros continue to work. This is a pragmatic and correct tradeoff.

## 5. Weak Points / Design Smells

### 5.1 `AtTrackSeeder` is a near-complete duplicate of `AtCircleSeeder`

`AtTrackSeeder` (144 lines) and `AtCircleSeeder` (135 lines) are line-for-line duplicates of the same RANSAC circle + theta fit logic. `AtTrackSeeder` is not an `AtPatternTransform`, has no state, takes all parameters as function arguments, and doesn't appear in the pipeline. It looks like either: (a) transitional scaffolding that should be removed, or (b) the "old API" was preserved alongside the "new API" without deciding which one survives. Either way, this is the most significant code duplication in the refactor and should be resolved -- `AtCircleSeeder` is the canonical version and `AtTrackSeeder` should be removed unless there is a distinct use case.

### 5.2 `AtFragmentMerger` embeds pipeline logic

`AtFragmentMerger` holds a raw pointer to `AtSmooth3DClusterer` and an embedded `AtClusterOrderer`, because after each merge it needs to re-cluster and re-order the merged track to get an accurate far-end position for the next merge iteration. This makes `AtFragmentMerger` not truly composable -- it has implicit dependencies on two other transforms.

The root issue is that the merging algorithm inherently requires intermediate re-clustering to function correctly. Options:
- Accept this coupling and document it clearly (pragmatic, current approach)
- Pass a `std::function<void(AtTrack&)>` "post-merge hook" instead of concrete types (cleaner interface)
- Redesign merging to operate on raw hits instead of clusters (algorithmic change)

The current approach is defensible for now, but the raw pointer and lifetime comment ("caller must ensure the clusterer outlives this object") is a maintenance hazard.

### 5.3 `AtPRAtask` became a hardcoded pipeline orchestrator

`AtPRAtask::Exec()` now has an explicit hardcoded chain:

```
Cluster -> Prune -> Order -> BeamReject -> Merge -> VertexSelect -> Seed
```

The individual components are injectable, but the pipeline order is not. If a user wants to reorder steps (e.g., prune after ordering, or seed before vertex selection), they must use the more verbose `AtPatternFindingTask` + `AtPatternTransformTask` composition. This is fine as a convenience class, but its name (`AtPRAtask`) and its dual role (both "legacy entry point" and "preconfigured pipeline") could confuse new users. The docstring should make the fixed ordering more prominent.

### 5.4 `AtPRA` is now a vestigial base class

After the refactor, `AtPRA` provides only: `PruneTrack()`, `kNN()`, and `GetSign()` utility. `AtTrackFinderTC` no longer inherits from it -- it inherits from `AtPatternFinder`. `AtPRA` still inherits from `TObject` and has a `ClassImp` macro, but it's not clear what still uses it. The pruning logic has moved to `AtTrackPruner`. Unless `AtTrackFinderHC` (mentioned in the docstring) still inherits from `AtPRA`, this class looks like it should be deprecated or its remaining utility functions moved elsewhere.

### 5.5 `AtGroupClusterer` includes `AtTrackTransformer.h` for `CovarianceMode`

`AtGroupClusterer.h` includes `AtTrackTransformer.h` solely for the `AtTrackTransformer::CovarianceMode` type alias. The enum is now defined in `AtTrackClusterBuilder.h` as `AtTools::CovarianceMode`. This header should include `AtTrackClusterBuilder.h` directly instead, breaking the unnecessary dependency on `AtTrackTransformer`.

### 5.6 LinkDef: `AtPRA` still uses `+` (full streamer)

```
#pragma link C++ class AtPATTERN::AtPRA + ;
```

Per the project's code-writing rules, non-persisted classes should use `-!`. `AtPRA` is never written to disk; this should be `-!`.

## 6. Missed Opportunities

### 6.1 A `TransformChain` composite would eliminate the hardcoded pipeline

A simple composite:
```cpp
class AtTransformChain : public AtPatternTransform {
   std::vector<std::unique_ptr<AtPatternTransform>> fSteps;
public:
   void Add(std::unique_ptr<AtPatternTransform> step);
   void Transform(AtPatternEvent &event) override {
      for (auto &step : fSteps) step->Transform(event);
   }
};
```

This would let `AtPRAtask` build a chain in `Init()` and call `chain.Transform()` in `Exec()`. It would also let users compose arbitrary pipelines without needing FairTask boilerplate. It's a ~20-line class that would clean up the orchestration significantly.

### 6.2 `AtCircleSeeder` and `AtTrackSeeder` should share implementation

The ~130-line RANSAC circle+theta fit algorithm is duplicated. If both APIs are needed, `AtCircleSeeder::SeedTrack()` should delegate to a shared implementation.

### 6.3 The `isPrimary` predicate is duplicated

Both `AtFragmentMerger` and `AtVertexTrackSelector` compute "is this track's front cluster within `vertexRadiusXY` of the beam axis and within 50mm of max Z?" with near-identical lambda bodies. This predicate could be a free function or a shared utility. The magic number `50.0` (Z tolerance) appears in both places with no way to configure it.

### 6.4 `AtSmooth3DClusterer` exposes `ClusterizeTrack()` as a public method

This is called by `AtFragmentMerger` for per-merge re-clustering. The leak of a per-track method from what is otherwise an event-level transform is a sign that the merger's coupling issue (point 5.2 above) should be addressed at a different level.

## 7. Overall Judgment

**This refactor is a clear net improvement.** It takes a monolithic, untestable, entangled pattern-recognition pipeline and decomposes it into small, named, independently testable components behind two clean interfaces. The data flow is now visible and explicit. The algorithmic intent maps to class structure. New pipeline stages can be added without touching existing code.

**Where it succeeds most:**
- The `AtPatternFinder`/`AtPatternTransform` interface split is well-chosen and stable
- `AtTrackFinderTC` is now a focused, single-responsibility class
- The `AtTrackClusterBuilder` extraction eliminates the duplicated clustering math
- The test coverage (unit tests for individual transforms, integration test for the full pipeline) validates the decomposition
- Backward compatibility is preserved through `AtPRAtask`

**Where it falls short:**
- `AtTrackSeeder` is dead/duplicate code that should be removed
- `AtFragmentMerger` has leaked coupling to the clusterer and orderer
- The `isPrimary` predicate is duplicated and has a hardcoded magic number
- `AtPRA` is now a vestigial class that should be cleaned up or removed
- A `TransformChain` composite is an obvious missing piece that would improve both the `AtPRAtask` orchestration and user-facing composition

These shortcomings are minor relative to the improvement. The design direction is coherent, the decomposition is well-motivated, and the remaining issues are all straightforward to address incrementally.
