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
└── e12014_zap.csv           # low-gain pads to inhibit
```

The fragment tables (Z 26–59) are in the repo at `macro/e12014/adam/determineZ/eLoss/`. `respAvg.root` is at `macro/e12014/curtis/viewer/respAvg.root`. The ²⁰⁰Bi beam table (`83_200.txt`) and the zap file are not in the repo. Simulation cannot run without the beam table. See the [appendix](#appendix-known-paths-by-machine) for where copies were last seen.

Set the environment:

```bash
export TPC_SHARED_INFO=/path/to/shared/info
export FISSION_DATA=/path/to/data   # optional, default: data/ next to these macros
```

`FISSION_DATA` is where every output file goes. Put it on a disk with plenty of room. Digi files store raw traces, and fit files store the 10 best simulated events per event, so each costs roughly 20–25 MB per event. A 500-event run is about 20–25 GB.

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
done   smoke-base.sim (4 s)
...
```

Run it again and it prints `Everything is up to date.`

Plot one run:

```bash
root -l
.L plot_fit.C
plot_fit("smoke", "base")
```

## Writing a study

`studies/` holds `smoke.ini` (a quick end-to-end check), `bench.ini` (fit time vs. threads), and three studies rebuilt from Skyler's results: `decay_angle.ini`, `mass_dev.ini` and `eloss_mismatch.ini`. Their settings are approximate.

A study is one `.ini` file in `studies/`. The file name (without `.ini`) is the study name. It holds the question, the shared settings, the runs, and the result once you have it. It is the record of what was done, so commit it.

```ini
[study]
question = Does fixing the decay angle improve Z resolution?
result   =                    ; fill in when done

[defaults]                    ; applies to every run
events = 500
sim.zToSim = 50
sim.massDev = 0
sim.eloss = LISE
fit.iter = 100

[run 90deg]                   ; one section per run; the name is free text
sim.decayAngle = 90

[run 120deg]
sim.decayAngle = 120
```

Rules:

- A run's settings are `[defaults]` overridden by its own section. Anything not set uses the macro's built-in default (see the [reference](#settings-reference)).
- A setting's prefix says which stage uses it: `sim.`, `digi.` or `fit.`. `events`, `seed`, `chunks` and `ions.*` apply to the whole run.
- Comments start with `;` or `#`.
- Run names become part of file names, so avoid spaces and slashes.

### Reusing another run's simulation

To change only the fit, point a run at another run with `upstream`. It reuses that run's sim and digi output and only refits:

```ini
[run lise]
sim.eloss = LISE

[run lise-srimfit]
upstream = lise
fit.eloss = SRIM
```

The upstream run's sim and digi are produced if needed. A run with `upstream` may only change `fit.*` settings. Changing anything else is an error, because it would no longer match the reused simulation.

## Settings reference

Defaults are the macros' built-in values.

**Whole run**

| Key | Default | Meaning |
|---|---|---|
| `events` | 500 | Events to simulate. Digi and fit process every event they receive. |
| `seed` | from study and run name | Random seed. The derived default is fixed for a given study and run, so sim and digi reruns reproduce. The fit does not (see [parallel running](#parallel-running)). |
| `chunks` | 1 | Split the run into this many independent pieces. See [parallel running](#parallel-running). |
| `ions.zmin`, `ions.zmax` | 26, 59 | Fragment Z range that can be simulated and fit. Every Z needs tables. |
| `ions.zcn`, `ions.acn` | 85, 204 | Compound nucleus (²⁰⁴At). A fragment's A is `round(Z / zcn * acn)`. |

**Simulation (`sim.`)**

| Key | Default | Meaning |
|---|---|---|
| `sim.zToSim` | 50 | Sets the mean fragment mass, as the fraction `zToSim / zcn` of the compound nucleus. |
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

To expose a new setting, read it in the macro with `cfg.Get("stage.name", default)`, `cfg.GetInt` or `cfg.GetStr`. Give it the prefix of the stage that reads it, so the driver knows what to rerun when it changes.

## Running a study

```bash
./fission.py STUDY.ini [RUN ...] [options]
```

| Option | Effect |
|---|---|
| `RUN ...` | Only these runs (default: all). |
| `--dry-run` | Show what would run and why, then stop. |
| `-j N` | Run up to N jobs at once (default 1). |
| `--stage sim\|digi\|fit` | Stop after this stage (default fit). |
| `--force sim\|digi\|fit` | Rerun from this stage down, even if up to date. |
| `--slurm` | Submit to SLURM instead of running here. See [Running on a cluster](#running-on-a-cluster). |

### What reruns

The driver only runs stages that are out of date and gives the reason for each:

| Reason | Meaning |
|---|---|
| `missing` | No output, or the last attempt did not finish. |
| `changed: sim.massDev 0->2` | A setting this stage depends on changed in the study file. |
| `input is newer` | The stage before it was rerun since. |
| `upstream rerun` | The stage before it is about to rerun. |
| `forced` | `--force` was given. |

A stage depends on its own settings and on every earlier stage's settings. Changing `fit.*` refits only. Changing `digi.*` redoes digi and fit. Changing `sim.*`, `events`, `seed` or `ions.*` redoes everything.

Outputs are overwritten in place. To keep an old result, give the new version a new run name.

### Files produced

All files go in `$FISSION_DATA`:

```
<study>-<run>.sim.root    simulated energy deposits + truth
<study>-<run>.digi.root   traces, hits, Y-pattern fits + truth
<study>-<run>.fit.root    fit results (AtMCResult), best simulated events + truth
<study>-<run>.<stage>.cfg     settings the stage used (written only on success)
<study>-<run>.<stage>.cfg.in  settings passed to the macro
<study>-<run>.<stage>.log     full ROOT output
```

Each ROOT file also contains its settings as a `TNamed` called `RunConfig`:

```cpp
TFile f("data/smoke-base.fit.root"); cout << f.Get<TNamed>("RunConfig")->GetTitle();
```

### Parallel running

There are two independent levers:

- **`-j N`** runs separate jobs at once, such as different runs, or a fit while another run simulates.
- **`chunks = N`** in the study splits one run's events into N pieces (`.c00`, `.c01`, …), each with its own seed (seed + k). Each piece goes through sim, digi and fit on its own. Plots combine the pieces automatically.

Every fit job also uses `fit.threads` threads. Keep `-j × fit.threads` near your core count.

Prefer more chunks over more threads. The fitter's threads wait on a shared lock, so speed does not grow with thread count, and they draw from one shared random-number generator. Separate processes avoid both. `studies/bench.ini` measures how fit time scales with threads on a given machine.

No fit is reproducible, even with one thread: the fitter seeds its parameter sampling randomly and `seed` does not reach it. See bug 2 in [mc-fitter-threading.md](../../../docs/development/mc-fitter-threading.md).

Changing `chunks` renames the files, so the run is redone and the old files are removed.

## Looking at results

Start ROOT from this directory, with `FISSION_DATA` set as it was when the study ran.

**One run:**

```cpp
.L plot_fit.C
plot_fit("decay_angle", "90deg")     // fills and draws the Z histogram
zHistDiff->Draw()                    // true Z minus fitted Z
```

The histograms are globals in `plot_fit.C`: `zHist`, `aHist`, `hMR`, `hAmp`, `hObj`, `hObjPos`, `hObjQ`, `hBeam`, `hZvsObj`, `hZvsAmp`, `hAmpvsPosObj`, `hAmpvsObj`, `hAmpvsLoc`, `zHistSim`, `hZvsObjSim`, `zHistDiff`. `FillPlots(...)` refills them with different cuts.

To plot specific files, give a list. Wildcards are allowed:

```cpp
plot_fit(std::vector<TString>{"data/decay_angle-90deg.fit*.root", "data/decay_angle-120deg.fit*.root"});
```

**Compare runs by a setting:**

```cpp
.L group_fit_plots.C
group_fit_plots("decay_angle", "sim.decayAngle")   // writes ./groups_decay_angle/<plot>.root
show_groups("Z", "./groups_decay_angle")           // side by side
show_groups("Z", "./groups_decay_angle", true)     // overlaid
```

Runs with the same value of the setting are combined. The values come from the `.cfg` files, so they are what actually ran. The plot names are `Z`, `A`, `Amp`, `Obj`, `ObjPos`, `ObjQ`, `MR`, `Beam`, `ZvsObj`, `ZvsAmp`, `AmpvsPosObj`, `AmpvsObj`, `AmpvsLoc`, `ZvsObjSim`, `ZSim` and `ZDiff`.

When the study is done, write the answer in its `result =` line and commit the study file.

## Running on a cluster

```bash
./fission.py studies/decay_angle.ini --slurm
```

This submits one `sbatch` job per stage and chunk. Each job waits for the job that makes its input (`--dependency=afterok`). Fit jobs request `fit.threads` CPUs, and other jobs request one. Logs go to the usual `.log` files.

Before submitting:

- Load the ATTPCROOT environment in the shell you submit from. Jobs inherit it.
- Set `TPC_SHARED_INFO` and `FISSION_DATA` to paths the compute nodes can see.
- Add a partition or account through the usual `SBATCH_*` environment variables (e.g. `export SBATCH_PARTITION=cs`).

Check status with `squeue`. When the jobs finish, run without `--slurm` (or with `--dry-run`) to confirm everything is up to date. If a job failed, the next run reruns only that stage and what depends on it.

## Running macros by hand

Every stage is an ordinary ROOT macro and still runs on its own, which is the easy way to try out a change.

**With built-in defaults:**

```bash
root -l -b -q run_simp_fiss.C    # writes $FISSION_DATA/manual.sim.root
root -l -b -q run_digi_fiss.C    # reads manual.sim.root,  writes manual.digi.root
root -l -b -q run_fit.C          # reads manual.digi.root, writes manual.fit.root
```

Each stage writes `manual.<stage>.cfg`, listing every setting and the seed it used. To try something, edit the default in the macro.

**With a settings file:** write `key = value` lines and pass the file:

```bash
cat > try.cfg <<EOF
events = 20
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
- An input file that can't be opened, because the stage before it failed. Rerunning the study retries from the first failed stage.

**`run X changes sim.foo but reuses sim/digi of Y`.** A run with `upstream` may only change `fit.*` settings. Remove `upstream`, or move the setting into the upstream run.

**Everything reruns when I didn't expect it.** Run `--dry-run` and read the reason. Changing `[defaults]` affects every run. A `sim.*` change redoes all three stages.

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
