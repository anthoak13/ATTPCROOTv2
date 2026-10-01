# Review: fission study driver

Open issues in the fission study driver, `fission.py`: planning, running, SLURM submission and the files it manages. Line numbers refer to 2bf67f8cb. Each item has a severity:

- **High**: changes the numbers a study reports.
- **Medium**: produces wrong or misleading output in some cases, deletes results, or crashes.
- **Low**: annoying, or a trap to fix eventually.

## Driver bugs and fragile spots

1. **Medium. A file `input` that matches fewer files silently shrinks the section and deletes its outputs.** `InputFiles` takes whatever the glob matches now (`fission.py:317`) and has one chunk per file. If the owner deletes one chunk's digi, or is midway through rewriting it, `input = a-base.digi.c*.root` drops from 2 files to 1. The driver then removes both existing fit chunks as an "old chunk count" and refits only the remaining file into an unchunked output. Nothing warns that events went missing.
   - Reproduce with fake outputs: delete `a-base.digi.c01.root`; `--dry-run` of the study with `[fit f] input = a-base.digi.c*.root` plans `c-f.fit` and would remove `c-f.fit.c00.*` and `c-f.fit.c01.*`.

2. **Medium. The SLURM busy check misses jobs of other studies that read this study's files.** `blockers` checks the queue only for the sections `affected_sections` returns (`fission.py:624-627`), and that looks only in the study being run (L528-540). `digi_scan.ini`'s `[digi resp050]` reads `smoke:base`. While it is queued or running in SLURM, rerunning `smoke.ini`'s `[sim base]` gives no warning and rewrites `smoke-base.sim.root` under the job that reads it.
   - Reproduce with fake outputs and a fake `squeue` that lists `digi_scan-resp050.digi`: change `sim.events` in `smoke.ini`. `--dry-run` of `smoke.ini base.sim` plans the sim and reports nothing queued.
   - Same cause as item 3. A section knows only what it reads, so `affected_sections` finds readers by building every section of the study being run and walking each input chain. Studies that read this one are never looked at.
   - Fix: start from the queue. `job_section` already parses each queued job's name, so load that job's study and walk its section's input chain to see whether it reaches a section being rewritten.

3. **Medium. One section whose input files don't exist yet stops every run of its study.** `affected_sections` builds every section of the study, selected or not (`study.sections()`, `fission.py:540`). An `input = <path or glob>` that matches nothing exits (L318-319).
   - Reproduce: with `[sim x]`, `[digi x]` and `[fit later] input = notyet-base.digi.root`, `fission.py c.ini x --dry-run` exits with `[fit later] … no files match`. The sim and digi can't run until something else makes the fit's input. `--status` without a selector exits the same way.
   - Same cause as item 2. Fix: when looking for readers, treat a section whose input doesn't resolve as reading nothing. It still fails with that error when it is selected.

4. **Medium. Chunks from 100 up are invisible to everything but the planner.** `Section.stem` writes `.c100` (`fission.py:279`), but `JOB_NAME` matches `c\d\d` (`fission.py:76`):
   - `old_chunking` filters with it, so those files are never removed after a chunk-count change.
   - `job_section` parses with it, so the SLURM busy check misses those jobs and can overwrite their files.
   - The plotting macros also assume two digits (`IsFitFile`, `plot_fit.C:214`), so plots silently drop those chunks.
   - MANUAL.md recommends more chunks over more threads, so a few hundred chunks on a cluster is a plausible setting. Nothing rejects it.

5. **Low. `sim.chunks`, `sim.events`, seeds and `-j` aren't validated.**
   - `sim.chunks = 0` plans no jobs: the driver says "Everything is up to date.", while `--status` shows an empty state and says "0 of 1 sections finished. --dry-run shows why the rest would run."
   - A non-integer `sim.chunks`, `sim.events` or `<stage>.seed` (e.g. `sim.events = 1e3`) raises a Python traceback.
   - `-j 0` raises `ValueError: max_workers must be greater than 0`.

6. **Low. Nothing stops two local drivers working on the same files.** The busy check covers only SLURM jobs. Starting a second `fission.py` on the same study (or on one that reads its sections) while the first is still running reruns the same jobs into the same outputs.

7. **Low. The driver needs Python 3.8, but nothing says so.** `shlex.join` (`fission.py:401`) is 3.8+ and `parse_intermixed_args` (L693) is 3.7+. MANUAL.md (line 23) says "Python 3". On a cluster whose `python3` is 3.6 (RHEL 8), the driver fails with an `AttributeError` traceback.

8. **Low. `squeue --me` needs Slurm 20.11 or newer** (`fission.py:586`). On an older cluster the call fails, the driver prints a warning and skips the busy check, so it can delete or overwrite files of queued jobs. `-u $USER` works everywhere.

9. **Low. A fit accepts a sim output as its input file.** `InputFiles` rejects a file only if it has settings of a later stage than expected (`fission.py:330-331`). A sim's `.cfg` has only `sim.*` keys, so `[fit f] input = smoke-base.sim.root` plans like any fit and fails only inside ROOT, possibly after waiting in the SLURM queue. Every digi output has at least `digi.seed`, so also requiring a key of the expected stage would catch it.

10. **Low. A rejected `sbatch` hides why, after earlier jobs are already submitted.** `run_slurm` runs `sbatch` with `check=True, capture_output=True` (`fission.py:614`). sbatch's error message is thrown away, and the driver stops with a `CalledProcessError` traceback. Nothing checks `<stage>.slurm.time` or `.mem` before submitting, so a typo like `digi.slurm.mem = 8 G` shows up only after the sims are queued and the failing job's `.cfg` has been removed.

11. **Low. Hand-set seeds close together repeat events.** Chunk k runs with `seed + k` (`fission.py:293`), so chunked sections with `seed = 100` and `seed = 101` share seeds: the first's chunk 1 and the second's chunk 0 run with the same seed. For two sims, those chunks simulate the same events. Numbering seeds 1, 2, 3 by hand is natural, and nothing warns. Derived seeds (a CRC of the name) are far apart, so only hand-set ones hit this.

12. **Low. Two studies that read each other send you the wrong way.** With `[sim s]` and `[fit f] input = pb:d` in `pa.ini`, and `[digi d] input = pa:s` in `pb.ini`, running `pa.ini` says to bring `pb.ini d.digi` up to date first. `pb.ini` then says to run `pa.ini s.sim`.
    - A stale job of another study blocks every job of the run (`fission.py:628-636`, L664-665), including `pa`'s own sim, which doesn't depend on it. Holding back only the jobs that depend on the stale external job would let it run.
    - The plan also lists `pb-d.digi` before the `pa-s.sim` it reads, because `print_plan` is given `external + todo` (L659).

## Hard to use

13. **Medium. Nothing checks the environment before running or submitting.** A missing `TPC_SHARED_INFO`, beam table, zap file or `respAvg.root` is found only when each job fails, after it has waited in the SLURM queue. Without ROOT on `PATH`, every job that starts fails with `root: not found` in its log, and the jobs that depend on it are skipped. One up-front check of the ROOT environment and every file the planned jobs need would catch all of these.

14. **Medium. Deleting sim or digi files to free disk reruns them.** A missing output is always `missing` (`fission.py:379-380`), and everything that reads it becomes `upstream rerun`. With digi files at 20–25 MB per event, a finished study can't drop its intermediates without the next invocation redoing digi and fit (or all three stages, if the sim file goes).

15. **Settings that don't change results still force a refit.** `fit.threads` and `fit.timeEvent` are `fit.*` keys, so changing them (e.g. to match a cluster's core count) marks every fit as changed.

16. **Moving the data directory reruns everything.** Each `.cfg` records the absolute `input` and `output` paths (`fission.py:370-371`). After copying `FISSION_DATA` to another disk or machine, every stage shows as changed.

17. **A relative `FISSION_DATA` points to different places.** `fission.py` resolves it against the shell's directory (`fission.py:70`), the plotting macros against ROOT's directory (`RunConfig.h:104-105`). They agree only if the driver is run from `fissionStudies/`.

## Known, not being fixed

Real issues that have been reviewed and deliberately left alone. They stay listed so they aren't reported again as new.

18. **Low. More chunks than events isn't rejected.** With `sim.events = 2, sim.chunks = 4`, two chunks get `sim.events = 0` (`fission.py:296`). Those jobs produce empty files and are counted as successful.
