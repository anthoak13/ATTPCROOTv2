# Branch and IO Contracts

Default branch names, contained types, and persistence settings for major tasks. These come from task constructors and `Init()` implementations in the current source.

Every branch listed here is a `TClonesArray` holding one event object at slot `0` — not a bare pointer:

```cpp
auto *arr = dynamic_cast<TClonesArray *>(ioMan->GetObject("AtEventH"));
auto *event = dynamic_cast<AtEvent *>(arr->At(0));
```

## Reconstruction Tasks

| Task | Input branch / type | Output branch / type | Persist default | Branch setters |
|------|---------------------|----------------------|-----------------|----------------|
| `AtUnpackTask` | external file via unpacker | `AtRawEvent` → `TClonesArray<AtRawEvent>` | **true** | — |
| `AtFilterTask` | `AtRawEvent` → `TClonesArray<AtRawEvent>` | `AtRawEventFiltered` → `TClonesArray<AtRawEvent>` | false | `SetInputBranch`, `SetOutputBranch` |
| `AtPSAtask` | `AtRawEvent` → `TClonesArray<AtRawEvent>` | `AtEventH` → `TClonesArray<AtEvent>` | false | `SetInputBranch`, `SetOutputBranch` |
| `AtDataCleaningTask` | `AtEventH` → `TClonesArray<AtEvent>` | `AtEventCleaned` → `TClonesArray<AtEvent>` | **true** | `SetInputBranch`, `SetOutputBranch` |
| `AtPRAtask` | `AtEventH` → `TClonesArray<AtEvent>` | `AtPatternEvent` → `TClonesArray<AtPatternEvent>` | false | `SetInputBranch`, `SetOutputBranch` |
| `AtPatternFindingTask` | `AtEventH` → `TClonesArray<AtEvent>` | `AtPatternEvent` → `TClonesArray<AtPatternEvent>` | false | `SetInputBranch`, `SetOutputBranch`, `SetMinNumHits` (default 10), `SetMaxNumHits` (default 5000) |
| `AtPatternTransformTask` | `AtPatternEvent` → `TClonesArray<AtPatternEvent>` | `AtPatternEventTransformed` → `TClonesArray<AtPatternEvent>` | false | `SetInputBranch`, `SetOutputBranch` |
| `AtSampleConsensusTask` | `AtEventH` → `TClonesArray<AtEvent>` | `AtPatternEvent` → `TClonesArray<AtPatternEvent>` | false | `SetInputBranch`, `SetOutputBranch` |
| `AtFitterTask` | `AtPatternEvent` → `TClonesArray<AtPatternEvent>` | `AtTrackingEvent` → `TClonesArray<AtTrackingEvent>`; optional `AtFitMetadata` → `TClonesArray<AtFitMetadata>` (registered only when `SetFitMetadataBranch` is called) | false | `SetInputBranch`, `SetOutputBranch`, `SetFitMetadataBranch`, `SetRawEventBranch`, `SetEventBranch` |
| `AtMCFitterTask` | `AtPatternEvent` → `TClonesArray<AtPatternEvent>` | `AtMCResult` (**true**), `SimEvent` (false), `SimRawEvent` (false) | per-branch | `SetPatternBranchName`, `SetSaveResult/Event/RawEvent` |

## Simulation and Digitization Tasks

| Task | Input branch / type | Output branch / type | Persist default | Branch setters |
|------|---------------------|----------------------|-----------------|----------------|
| `AtClusterizeTask` | `AtTpcPoint` → `TClonesArray<AtMCPoint>` | `AtSimulatedPoint` → `TClonesArray<AtSimulatedPoint>` | false | `SetPersistence` |
| `AtPulseTask` | `AtSimulatedPoint` → `TClonesArray<AtSimulatedPoint>`, optional `AtTpcPoint` → `TClonesArray<AtMCPoint>` | `AtRawEvent` → `TClonesArray<AtRawEvent>` (**true**); re-registers `AtTpcPoint` (false) | true / false | `SetOutputBranch`, `SetPersistence`, `SetPersistenceAtTpcPoint`, `SetSaveMCInfo` |

## Reading This Page

- These are defaults, not hard-coded universals. Most tasks expose `SetInputBranch` / `SetOutputBranch`.
- `AtPRAtask`, `AtPatternFindingTask`, and `AtSampleConsensusTask` all write `AtPatternEvent` by default — only one should produce a given branch name per run.
- `AtPatternTransformTask` defaults to a different output branch (`AtPatternEventTransformed`) so it can be chained without overwriting the finder output. Wire branch names explicitly when chaining multiple transform tasks.
- `AtFitterTask` writes `AtFitMetadata` only when `SetFitMetadataBranch` has been called; otherwise that branch is not registered.
- `AtMCFitterTask` writes three separate branches; each has its own save toggle.
- In the AT-TPC path, `AtTpcPoint` is a branch name whose current container holds `AtMCPoint` objects.
- If you add a task that reads or writes FairRoot branches, add a row above.
- For what the contained objects hold (fields, ownership), see [data-model.md](data-model.md).
- For pattern recognition pipeline architecture, see [../subsystems/pattern-recognition.md](../subsystems/pattern-recognition.md).
