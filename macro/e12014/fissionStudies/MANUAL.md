# Fission Fit Validation: User Manual

How to run simulate → digitize → fit studies for e12014 fission and look at the results. The macros started as copies of Skyler Gangestad's validation work in [`../skyler/`](../skyler/); see her [NOTES.md](../skyler/NOTES.md) for the physics, results so far and known issues.

## Contents

1. [Setup](#setup)
2. [Quick start](#quick-start)
3. [Writing a study](#writing-a-study)
4. [Settings reference](#settings-reference)
5. [Running a study](#running-a-study)
6. [Looking at results](#looking-at-results)
7. [Running on a cluster](#running-on-a-cluster)
8. [Running macros by hand](#running-macros-by-hand)
9. [Troubleshooting](#troubleshooting)
10. [Appendix: known paths by machine](#appendix-known-paths-by-machine)

## Setup

You need:

- A built ATTPCROOT with its environment loaded (`source build/config.sh`). See [docs/tooling/installation.md](../../../docs/tooling/installation.md).
- Python 3 (standard library only).
- The shared inputs, in one directory pointed to by `TPC_SHARED_INFO`:

```
$TPC_SHARED_INFO/
├── eLoss/
│   ├── LISE/<Z>_<A>.txt     # one per fragment in the ion list, plus the beam: 83_200.txt
│   └── SRIM/<Z>_<A>.txt     # same, for SRIM
├── respAvg.root             # averaged pad response
└── e12014_zap.csv           # low-gain pads: digi turns them down, the fit skips them
```

The fragment tables (Z 26–59) are in the repo at `macro/e12014/adam/determineZ/eLoss/`. `respAvg.root` is at `macro/e12014/curtis/viewer/respAvg.root`. The ²⁰⁰Bi beam table (`83_200.txt`) and the zap file are not in the repo. Simulation cannot run without the beam table, and digi and fit stop if the zap file is missing or lists no pads. See the [appendix](#appendix-known-paths-by-machine) for where copies were last seen.

Set the environment:

```bash
export TPC_SHARED_INFO=/path/to/shared/info
export FISSION_DATA=/path/to/data   # optional, default: data/ next to these macros
```

`FISSION_DATA` is where every output file goes. Put it on a disk with plenty of room. Digi files store raw traces, and fit files store the 10 best simulated events per event, so each costs roughly 20–25 MB per event. A 500-event sim with one digi and one fit is about 20–25 GB.

## Quick start

From this directory:

```bash
./fission.py studies/smoke.ini --dry-run   # list what would run
./fission.py studies/smoke.ini -j 2        # run it (a few minutes)
```

Output:

```
smoke-base.sim                           missing
smoke-base.digi                          missing
smoke-base.fit                           missing
smoke-srimfit.fit                        missing
start  smoke-base.sim
done   smoke-base.sim (5 s)
...
```

Run it again and it prints `Everything is up to date.` `--status` shows what is finished:

```
section      input         state
sim base                   done
digi base    <- sim base   done
fit base     <- digi base  done
fit srimfit  <- digi base  done

4 of 4 sections finished.
```

Plot one fit:

```bash
root -l
.L plot_fit.C
plot_fit("smoke", "base")
```

## Writing a study

`studies/` holds:

- `smoke.ini`: a quick end-to-end check.
- `smoke_refit.ini`: refits smoke's digi from another study.
- `digi_scan.ini` and `fit_scan.ini`: digitize smoke's sim several ways, then fit each digi from a third study.
- `bench.ini`: fit time vs. threads.
- `decay_angle.ini`, `mass_dev.ini` and `eloss_mismatch.ini`: rebuilt from Skyler's results. Their settings are approximate.

A study is one `.ini` file in `studies/`. The file name (without `.ini`) is the study name. It holds the question, the outputs to make and their settings, and the result once you have it. It is the record of what was done, so commit it.

Each section makes exactly one output: a simulation `[sim <name>]`, a digitization `[digi <name>]` or a fit `[fit <name>]`.

```ini
[study]
question = Does fixing the decay angle improve Z resolution?
result   =                    ; fill in when done

[defaults]                    ; <stage>.<key>, applies to every section of that stage
sim.events = 500
sim.zToSim = 50
sim.eloss = LISE
fit.iter = 100

[sim 90deg]                   ; keys in a section have no prefix: decayAngle is sim.decayAngle
decayAngle = 90

[digi 90deg]                  ; digitizes [sim 90deg]

[fit 90deg]                   ; fits [digi 90deg]

[fit 90deg-srim]              ; fits [digi 90deg] again, with SRIM tables
input = 90deg
eloss = SRIM
```

Rules:

- A key `k` in a `[<stage> <name>]` section is the setting `<stage>.k`. In `[defaults]`, write the full `<stage>.k`. This holds for every setting, including `sim.events`, `sim.chunks`, `<stage>.seed`, `<stage>.input` and `<stage>.slurm.*` (see the [reference](#settings-reference)).
- A section's settings are the `[defaults]` keys of its stage, overridden by the section. Other stages' defaults never reach it: `fit.iter` in `[defaults]` changes the fits only.
- Anything not set uses the macro's built-in default.
- Comments start with `;` or `#`.
- Section names may use letters, digits, `_` and `-`, and are unique per stage (a sim and a fit may share a name). Study names (the file name) may use letters, digits and `_`, so `<study>-<name>` splits only one way.
- Any section header other than `[study]`, `[defaults]` and `[sim|digi|fit <name>]` is an error, as is a key with a stage prefix inside a stage's section.

### What a section reads

A digi reads a sim, and a fit reads a digi: always an output of the stage just before it. Which one is set by `input`, looked up in this order:

1. `input` in the section.
2. `<stage>.input` in `[defaults]`, e.g. `fit.input = t1` to fit one digi several ways.
3. The section of the previous stage with the same name: `[fit 90deg]` reads `[digi 90deg]`. If there is none, the driver stops and names the section it looked for.

`input` takes three forms:

| Form | Reads |
|---|---|
| `input = 90deg` | `[digi 90deg]` (for a fit) in this study |
| `input = decay_angle:90deg` | `[digi 90deg]` in `studies/decay_angle.ini` |
| `input = try.digi.root`, `input = a-b.digi.c*.root` | existing files: a path or glob (anything containing `/` or ending in `.root`), absolute or relative to `$FISSION_DATA` |

A sim has no input.

Selecting a section also brings its inputs in this study up to date: running a fit runs its digi and sim first if they are missing or stale.

### What a stage is passed

Each stage's macro is passed its own settings plus every setting of its input, and of that input's input. A fit is therefore passed every `sim.*`, `digi.*` and `fit.*` setting of its chain, which is how a change anywhere in the chain reaches the fits that depend on it. `--show` prints what each section's macro is passed.

This is also how a fit learns about the simulation it is fitting: `fit.eloss` falls back to `sim.eloss`, and the fit uses the sim's `sim.ions.*`.

### Reading another study

`input = <study>:<name>` reads a section of another study in the same directory, without touching that study's file:

```ini
; studies/fit_scan.ini
[defaults]
fit.iter = 10

[fit resp050]
input = digi_scan:resp050
```

The other study's files are read-only from here. The driver never runs, forces or removes them, and `--force` reaches only this study's sections. If the other study's section (or anything it reads) is missing, out of date with its own study file, or queued in SLURM, the driver stops and prints the commands that bring it up to date:

```
Bring other studies' sections up to date first:
  ./fission.py studies/smoke.ini base.sim
  ./fission.py studies/digi_scan.ini resp050.digi resp075.digi resp100.digi
```

Once that study reruns its section, the fits here show `input is newer` and run again next time.

The other study can't see the sections here. Don't rerun its outputs while sections reading them are queued or running in SLURM. If its chunk count changes, the sections here follow on their next run and their old chunks are removed.

### Reading files

`input` may also be a path or glob for files, such as ones made [by hand](#running-macros-by-hand):

```ini
[fit mytry]
input = try.digi.root           ; or other-study-name.digi.c*.root, /elsewhere/x.digi.root
iter = 200
```

Each matching file is one chunk. Each must have the `.cfg` written next to it when its stage finished, and that `.cfg` takes the place of the input's settings. The files must agree on every setting except `*.events` and `*.seed`. A `.cfg` with unprefixed keys (`events`, `seed`, `ions.*`) was made before study files had stage sections, and is rejected: rerun it. As with another study, the files are never rerun or removed.

### Chunks and seeds

`sim.chunks = N` splits a sim's events into N independent pieces (`.c00`, `.c01`, …) that can run in parallel. Chunk k gets its share of `sim.events`. The digis and fits that read a chunked sim have one job per chunk too. A digi or fit has as many chunks as its input: one per chunk of the section it reads, or one per matched file.

Every stage has its own seed: `sim.seed`, `digi.seed` and `fit.seed`. Each defaults to a value derived from `<study>-<name>.<stage>`, so it is fixed for a given section, and chunk k uses seed + k.

## Settings reference

Defaults are the macros' built-in values, except where marked as the driver's.

**Driver settings, every stage.** These are used by the driver and never passed to a macro, except for the seed.

| Key | Default | Meaning |
|---|---|---|
| `<stage>.seed` | from study, name and stage (driver) | Random seed of the stage. Fixed for a given section, so sim and digi reruns reproduce. The fit does not (see [parallel running](#parallel-running)). |
| `<stage>.input` | the section of the previous stage with the same name (driver) | What a digi or fit reads. See [What a section reads](#what-a-section-reads). Not allowed for a sim. |
| `<stage>.slurm.time`, `<stage>.slurm.mem` | per stage (driver) | `sbatch` request. See [Running on a cluster](#running-on-a-cluster). |

**Simulation (`sim.`)**

| Key | Default | Meaning |
|---|---|---|
| `sim.events` | 500 (driver) | Events to simulate. Digi and fit process every event they receive. |
| `sim.chunks` | 1 (driver) | Split the sim, and everything that reads it, into this many independent pieces. See [chunks and seeds](#chunks-and-seeds). |
| `sim.ions.zmin`, `sim.ions.zmax` | 26, 59 | Fragment Z range that can be simulated and fit. Every Z needs tables. Both fragments of a split must be in range, so `zmin + zmax` must equal `zcn`, or every stage stops with an error. |
| `sim.ions.zcn`, `sim.ions.acn` | 85, 204 | Compound nucleus (²⁰⁴At). The simulation fissions it, the fit assumes it, and `plot_fit` scales its ranges by it. A fragment's A is `round(Z / zcn * acn)`. The fit takes these from its sim, so it can only assume the sim's nucleus. |
| `sim.zToSim` | 50 | Sets the mean fragment mass, as the fraction `zToSim / zcn` of the compound nucleus. That mass must belong to a Z in `sim.ions.zmin`–`sim.ions.zmax`, or the sim stops with an error. |
| `sim.massDev` | 0 | Width of the fragment mass distribution (amu). 0 gives a single split. |
| `sim.decayAngle` | 90 | CoM decay angle in degrees. 0 samples the measured angular distribution. |
| `sim.eloss` | LISE | Energy-loss tables used in simulation: `LISE` or `SRIM`. |
| `sim.beamE`, `sim.beamEsig` | 2700.13, 128.122 | Beam energy and spread at the window (MeV). |
| `sim.zCutoff` | 300 | Vertices are simulated from the window up to this distance from the pad plane (mm). |

**Digitization (`digi.`)**

| Key | Default | Meaning |
|---|---|---|
| `digi.lowGain` | 0.19 | Gain factor for the zapped low-gain pads. |
| `digi.respScale` | 0.0075 | Scaling applied to the measured pad response. |

**Fit (`fit.`)**

| Key | Default | Meaning |
|---|---|---|
| `fit.eloss` | same as `sim.eloss` | Energy-loss tables used in the fit. Set it only to study a table mismatch on purpose. |
| `fit.iter` | 100 | Monte Carlo iterations per round. |
| `fit.rounds` | 2 | Rounds. Each round narrows the parameter distributions around the best result. |
| `fit.threads` | 4 | Threads per fit job. |
| `fit.psaThreshold` | 25 | PSA threshold for the fit's simulated events. |
| `fit.timeEvent` | 0 | 1 logs the time per round, for benchmarking. |

To expose a new setting, read it in the macro with `cfg.Get("stage.name", default)`, `cfg.GetInt` or `cfg.GetStr`, and give it the prefix of the stage that reads it. A later stage that needs it (as the fit needs `sim.ions.*`) reads the same key, which it is passed through its input.

Misspelled settings are errors. The driver rejects misplaced keys (no stage prefix in `[defaults]`, a prefix inside a section, `input` in a sim, `events` or `chunks` outside a sim, an unknown `slurm.` key). Each macro calls `cfg.CheckUnused(...)` before running and fails on any setting it was passed but never read, except for the earlier stages' prefixes it lists.

## Running a study

```bash
./fission.py STUDY.ini [SECTION ...] [options]
```

| Option | Effect |
|---|---|
| `SECTION ...` | Only these sections (default: all). `NAME` selects every section with that name, `NAME.STAGE` (e.g. `90deg.digi`, spelled like its files) one section. Their inputs in this study come up to date first. |
| `--stage sim\|digi\|fit` | Only sections of this stage (repeatable). A filter on the selection: `--stage sim` runs only the sims, and `--stage digi` the digis and the sims they read. |
| `--dry-run` | Show what would run and why, then stop. |
| `--status` | Show one row per section: its input and its state, `done` or how many chunks are `missing`, `changed`, `input is newer`, `upstream rerun` or `queued` in SLURM, then stop. |
| `--show` | Print each section's settings, with where each comes from (`section`, `defaults`, or a driver default), what it reads (and what that reads), and what its macro would be passed (chunk 0's `.cfg.in`) and request from SLURM, then stop. |
| `-j N` | Run up to N jobs at once (default 1). |
| `--force sim\|digi\|fit` | Rerun this stage and the later ones, even if up to date. Never reaches another study's files. |
| `--slurm` | Submit to SLURM instead of running here. See [Running on a cluster](#running-on-a-cluster). |

### What reruns

The driver only runs jobs that are out of date and gives the reason for each:

| Reason | Meaning |
|---|---|
| `missing` | No output, or the last attempt did not finish. |
| `changed: sim.massDev 0->2` | A setting this section or its input chain depends on changed in the study file. |
| `input is newer` | The input was rerun since. |
| `upstream rerun` | The input is about to rerun. |
| `forced` | `--force` was given. |

A section depends on its own settings and on its input's, all the way back to the sim. Changing a `[fit]` setting refits only that fit. Changing a `[digi]` setting redoes the digi and every fit that reads it. Changing a `[sim]` setting redoes everything downstream of it.

Outputs are overwritten in place. To keep an old result, give the new version a new name.

### Files produced

All files go in `$FISSION_DATA`:

```
<study>-<name>.sim.root    simulated energy deposits + truth
<study>-<name>.digi.root   traces, hits, Y-pattern fits + truth
<study>-<name>.fit.root    fit results (AtMCResult), best simulated events + truth
<study>-<name>.<stage>.cfg     copy of the .cfg.in, written only on success
<study>-<name>.<stage>.cfg.in  settings passed to the macro
<study>-<name>.<stage>.log     full ROOT output
```

With `sim.chunks`, each name gets `.cNN` before the extension.

The driver compares the `.cfg` with what it would pass now to decide what to rerun. It does not see macro defaults, so changing a default in a macro reruns nothing (use `--force`).

Each ROOT file also records every setting the stage ran with, including the macro defaults it used, as a `TNamed` called `RunConfig`:

```cpp
TFile f("data/smoke-base.fit.root"); cout << f.Get<TNamed>("RunConfig")->GetTitle();
```

### Parallel running

There are two independent levers:

- **`-j N`** runs separate jobs at once, such as different sections, or a fit while another sim runs.
- **`sim.chunks = N`** splits a sim into N pieces (`.c00`, `.c01`, …), each with its own seeds. Each piece goes through digi and fit on its own. Plots combine the pieces automatically.

Every fit job also uses `fit.threads` threads. Keep `-j × fit.threads` near your core count.

Prefer more chunks over more threads. The fitter's threads wait on a shared lock, so speed does not grow with thread count, and they draw from one shared random-number generator. Separate processes avoid both. `studies/bench.ini` measures how fit time scales with threads on a given machine.

No fit is reproducible, even with one thread: the fitter seeds its parameter sampling randomly and `fit.seed` does not reach it. See bug 2 in [mc-fitter-threading.md](../../../docs/development/mc-fitter-threading.md).

Changing `sim.chunks` renames the files, so the sim is redone and its old files are removed. So are the old files of every section in the study that reads it, directly or through a digi, even with `--stage sim`, so no plot can mix chunk counts. `--dry-run` lists the files it would remove.

## Looking at results

Start ROOT from this directory, with `FISSION_DATA` set as it was when the study ran.

**One fit:**

```cpp
.L plot_fit.C
plot_fit("decay_angle", "90deg")     // [fit 90deg] of decay_angle.ini: fills and draws the Z histogram
zHistDiff->Draw()                    // true Z minus fitted Z
```

Events the fitter skipped (no good Y-pattern) have no fit results and are left out of every plot. Only finished fit files are read: a fit that failed, is running or is queued is skipped with a warning.

The histograms are globals in `plot_fit.C`: `zHist`, `aHist`, `hMR`, `hAmp`, `hObj`, `hObjPos`, `hObjQ`, `hBeam`, `hZvsObj`, `hZvsAmp`, `hAmpvsPosObj`, `hAmpvsObj`, `hAmpvsLoc`, `zHistSim`, `hZvsObjSim`, `zHistDiff`. `FillPlots(...)` refills them with different cuts.

To plot specific files, give a list. Wildcards are allowed, and the files are read as they are, finished or not:

```cpp
plot_fit(std::vector<TString>{"data/decay_angle-90deg.fit*.root", "data/decay_angle-120deg.fit*.root"});
```

**Compare fits by a setting:**

```cpp
.L group_fit_plots.C
group_fit_plots("decay_angle", "sim.decayAngle")   // writes ./groups_decay_angle/<plot>.root
show_groups("Z", "./groups_decay_angle")           // side by side
show_groups("Z", "./groups_decay_angle", true)     // overlaid
```

Fits with the same value of the setting are combined. The values come from the settings recorded in each fit file, which include its whole chain's settings and the fit macro's defaults. Only the `[fit <name>]` sections in `studies/<study>.ini` are used, so leftover files from renamed or removed fits are ignored. Start ROOT from this directory so it can find the study file. The plot names are `Z`, `A`, `Amp`, `Obj`, `ObjPos`, `ObjQ`, `MR`, `Beam`, `ZvsObj`, `ZvsAmp`, `AmpvsPosObj`, `AmpvsObj`, `AmpvsLoc`, `ZvsObjSim`, `ZSim` and `ZDiff`.

When the study is done, write the answer in its `result =` line and commit the study file.

## Running on a cluster

```bash
./fission.py studies/decay_angle.ini --slurm
```

This submits one `sbatch` job per section and chunk. Each job waits for the job that makes its input (`--dependency=afterok`). As with local runs, a job succeeds if the macro wrote its `.cfg`, whatever ROOT's exit code. Fit jobs request `fit.threads` CPUs, and other jobs request one. Logs go to the usual `.log` files.

Each stage also requests a time limit and memory:

| Stage | `--time` | `--mem` |
|---|---|---|
| sim | 0:10:00 | 4G |
| digi | 8:00:00 | 8G |
| fit | 3-00:00:00 | 8G |

These are rough upper bounds for a 500-event chunk, not measurements. Override them in the study file, in `[defaults]` with `<stage>.slurm.time` and `<stage>.slurm.mem` (e.g. `fit.slurm.time = 12:00:00`), or in one section with `slurm.time` and `slurm.mem`. They only change the `sbatch` request and are never passed to a macro, so changing them never reruns anything. `--show` prints what each section would request. They take precedence over `SBATCH_TIMELIMIT` and similar environment variables.

Before submitting:

- Load the ATTPCROOT environment in the shell you submit from. Jobs inherit it.
- Set `TPC_SHARED_INFO` and `FISSION_DATA` to paths the compute nodes can see.
- Add a partition or account through the usual `SBATCH_*` environment variables (e.g. `export SBATCH_PARTITION=cs`).

Check status with `squeue`. If a job fails, SLURM cancels the jobs waiting on it. To check progress, use `--status`: sections with jobs still in SLURM show as `queued`. When the jobs finish, every section shows `done`. If a job failed, the next run reruns only that section and what reads it.

While any job of a section whose files it would write or remove is queued or running, the driver refuses to run, with or without `--slurm`. Those sections are the ones it would rerun, plus every section of the study that reads them. Jobs are matched by name (`<study>-<name>.<stage>`, with or without `.cNN`), so this covers jobs from an old chunk count too, even ones that haven't started. Their files are never removed or written over while they are still pending. Wait for them, or `scancel` them first. On a machine without `squeue` the check is skipped.

## Running macros by hand

Every stage is an ordinary ROOT macro and still runs on its own, which is the easy way to try out a change.

**With built-in defaults:**

```bash
root -l -b -q run_simp_fiss.C    # writes $FISSION_DATA/manual.sim.root
root -l -b -q run_digi_fiss.C    # reads manual.sim.root,  writes manual.digi.root
root -l -b -q run_fit.C          # reads manual.digi.root, writes manual.fit.root
```

Each stage writes `manual.<stage>.cfg`, listing every setting and the seed it used. To try something, edit the default in the macro.

**With a settings file:** write `key = value` lines, with the full `<stage>.` prefix, and pass the file:

```bash
cat > try.cfg <<EOF
sim.events = 20
sim.decayAngle = 120
output = data/try.sim.root
EOF
root -l -b -q 'run_simp_fiss.C("try.cfg")'
```

`input` and `output` set the file paths. Without them, the `manual.*` names are used.

Interactive viewers (`run_eve_*.C`) take file names directly and are not part of the driver.

## Troubleshooting

**`FAILED <job>, see <log>`.** Open the log; the error is usually near the end. Common causes:

- A missing table, for example `.../eLoss/LISE/83_200.txt`. Check `TPC_SHARED_INFO` and the [layout](#setup).
- `TPC_SHARED_INFO is not set`. Set it.
- An input file that can't be opened, because the stage before it failed. Rerunning the study retries from the first failed job.

**`Settings never read by this stage (typo?)`.** A misspelled key in the study file. Fix the spelling.

**`unknown section`, `needs a stage prefix`, `have no stage prefix`.** The study file is in the wrong shape, often an old one with `[run <name>]` sections, unprefixed `events`/`seed`, or `upstream`. Rewrite it as `[sim|digi|fit <name>]` sections (see [Writing a study](#writing-a-study)).

**`[fit x] reads [digi x] by default, but there is none`.** A digi or fit has no `input` and no same-named section of the previous stage. Add that section, or set `input`.

**`was made before study files had stage sections`.** An `input` file's `.cfg` uses the old unprefixed keys. Rerun the file's study, or make the file again.

**`Still queued or running in SLURM`.** Jobs from an earlier `--slurm` submission are still in `squeue`. Wait for them, or `scancel` them.

**`Bring other studies' sections up to date first`.** A section reads another study's section (`input = <study>:<name>`), and that section or something it reads is missing, queued, or out of date with its study file. Run the printed commands, wait for them to finish, then run this study again.

**Everything reruns when I didn't expect it.** Run `--dry-run` and read the reason. `--show NAME` shows where each of the section's settings comes from. Changing `[defaults]` affects every section of that stage, and a sim change redoes everything that reads it.

**Nothing reruns after I edited a macro.** The driver tracks settings, not code. Use `--force <stage>`.

**The plot has no files.** `plot_fit` looks in `$FISSION_DATA` relative to where ROOT started. Start ROOT from this directory with the same `FISSION_DATA`, or pass full paths.

**Log errors that are normal.** These appear in every run and are not failures:

- `FairBaseParSet not initialized` / `Error occured during initialization`
- `Position of particle … is not in active volume`
- `Hits that are points (sig_z = 0) are not supported yet!`

**Old files (`output_fitNN.root`).** Files made before this driver don't plot with the current `plot_fit.C`. Use the version from git history.

## Appendix: Known Paths by Machine

Paths seen in this code's history or found by searching, as of 2026-09. Each is specific to one machine and may be stale. Entries marked "from history" were never checked.

**Adam's laptop (WSL)**
- eLoss tables (fragments only): `<repo>/macro/e12014/adam/determineZ/eLoss/`
- `respAvg.root`: `<repo>/macro/e12014/curtis/viewer/respAvg.root`
- `e12014_zap.csv`: `/mnt/e/Users/Adam/Dropbox/MSU/fission/e12014_zap.csv`
- Raw SRIM outputs: `/mnt/e/Users/Adam/Dropbox/MSU/fission/SRIM/`, `/mnt/e/Users/Adam/Documents/SRIM/SRIM Outputs/`
- `83_200.txt`: not found on any drive
- Old shared info `/home/adam/fair_install/tpcSharedInfo/`: from history, gone now

**Skyler's machine** (from history; the `physics` and `skyler` users are the same machine)
- Shared info: `/home/skyler/fission/tpcSharedInfo/`. This is the only known complete set, including `83_200.txt`.
- Shared info, older (2024): `/home/physics/fair_install/tpcSharedInfo/`
- Data: `/mnt/tpc-data/`

**Faculty cluster, `aanthony`** (from history; SLURM, `cs` partition)
- Shared info: `/home/faculty/aanthony/fission/data/e12014/tpcSharedInfo/`
- Unpacked e12014 data: `/home/faculty/aanthony/fission/data/e12014/unpacked/`
- Repo checkout: `/home/faculty/aanthony/fission/adam/ATTPCROOTv2/`

**FRIB analysis machines** (from history; experiment data, not shared info)
- `/mnt/analysis/e12014/TPC/<pressure>Torr/`
- `/mnt/simulations/attpcroot/adam/`
