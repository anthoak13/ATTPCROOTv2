#!/usr/bin/env python3
"""Run the fission-fit validation chain (sim -> digi -> fit) for the runs of a study file.

A study file (studies/*.ini) holds shared [defaults] and one [run <name>] section per run. Keys are
prefixed by the stage that uses them (sim., digi., fit.). ions.*, events and seed are used by every
stage. chunks splits a run's events into independent pieces that can run in parallel. A run with
`upstream = <other run>` reuses that run's sim and digi output and only refits.

Each stage writes <data>/<study>-<run>.<stage>[.cNN].root and, when it finishes, a .cfg file with the
settings it used. A stage is rerun if its output is missing, its settings (or any upstream settings)
changed, or its input is newer than its output.

    ./fission.py studies/smoke.ini --dry-run       # show what would run and why
    ./fission.py studies/smoke.ini -j 4            # run everything that is stale, 4 jobs at a time
    ./fission.py studies/smoke.ini 90deg --stage digi
    ./fission.py studies/smoke.ini --force fit     # rerun fit even if up to date
    ./fission.py studies/smoke.ini --slurm         # submit the jobs with sbatch instead

Environment: FISSION_DATA (default ./data next to this script), TPC_SHARED_INFO (tables, response).
"""
import argparse
import configparser
import glob
import os
import shutil
import subprocess
import sys
import time
import zlib
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait

HERE = os.path.dirname(os.path.abspath(__file__))
STAGES = ["sim", "digi", "fit"]
MACROS = {"sim": "run_simp_fiss.C", "digi": "run_digi_fiss.C", "fit": "run_fit.C"}
GLOBAL_KEYS = ("events", "seed")  # Plus everything starting with "ions."
DATA = os.path.abspath(os.environ.get("FISSION_DATA", os.path.join(HERE, "data")))


def used_by(key, stage):
    """True if a setting affects the output of stage (its own keys, upstream keys, and globals)."""
    if key in GLOBAL_KEYS or key.startswith("ions."):
        return True
    prefix = key.split(".", 1)[0]
    return prefix in STAGES and STAGES.index(prefix) <= STAGES.index(stage)


def read_cfg(path):
    """Parse a key = value file (the format RunConfig.h reads and writes)."""
    values = {}
    with open(path) as f:
        for line in f:
            if "=" in line and not line.startswith("#"):
                key, val = line.split("=", 1)
                values[key.strip()] = val.strip()
    return values


class Study:
    def __init__(self, path):
        self.name = os.path.splitext(os.path.basename(path))[0]
        parser = configparser.ConfigParser(inline_comment_prefixes=(";", "#"))
        parser.optionxform = str  # Keep key case (sim.massDev)
        parser.read(path)
        self.defaults = dict(parser["defaults"]) if parser.has_section("defaults") else {}
        self.sections = {s.split(None, 1)[1]: dict(parser[s]) for s in parser.sections() if s.startswith("run ")}
        if not self.sections:
            sys.exit(f"{path}: no [run <name>] sections")

    def settings(self, run):
        """Resolved settings for a run, and the run that owns its sim and digi output."""
        if run not in self.sections:
            sys.exit(f"{self.name}: no run named {run}")
        section = dict(self.sections[run])
        upstream = section.pop("upstream", None)
        settings = {**self.defaults, **section}
        owner = run
        if upstream:
            up_settings, owner = self.settings(upstream)
            settings = {**up_settings, **section}
            for key in settings:
                if not (used_by(key, "digi") or key == "chunks"):
                    continue
                if settings[key] != up_settings.get(key):
                    sys.exit(f"{self.name}: run {run} changes {key} but reuses sim/digi of {upstream}")
        settings.setdefault("seed", str(zlib.crc32(f"{self.name}-{owner}".encode()) & 0x7FFFFFFF or 1))
        settings.setdefault("events", "500")
        return settings, owner


class Job:
    """One stage of one chunk of one run."""

    def __init__(self, study, run, stage, chunk, settings, input_run):
        self.study, self.run, self.stage, self.chunk = study, run, stage, chunk
        nchunks = int(settings.get("chunks", 1))
        suffix = f".c{chunk:02d}" if nchunks > 1 else ""
        self.base = os.path.join(DATA, f"{study}-{run}.{stage}{suffix}")
        self.output = self.base + ".root"
        self.deps = []

        cfg = {k: v for k, v in settings.items() if used_by(k, stage)}
        events = int(settings["events"])
        cfg["events"] = str(events // nchunks + (chunk < events % nchunks))
        cfg["seed"] = str(int(settings["seed"]) + chunk)
        cfg["output"] = self.output
        if stage != "sim":
            prev = STAGES[STAGES.index(stage) - 1]
            cfg["input"] = os.path.join(DATA, f"{study}-{input_run}.{prev}{suffix}.root")
        self.cfg = cfg

    @property
    def key(self):
        return (self.run, self.stage, self.chunk)

    def __str__(self):
        return os.path.basename(self.base)

    def stale_reason(self):
        sidecar = self.base + ".cfg"
        if not (os.path.exists(self.output) and os.path.exists(sidecar)):
            return "missing"
        done = read_cfg(sidecar)
        changed = [f"{k} {done.get(k, '-')}->{self.cfg.get(k, '-')}"
                   for k in sorted(set(done) | set(self.cfg)) if done.get(k) != self.cfg.get(k)]
        if changed:
            return "changed: " + ", ".join(changed)
        if "input" in self.cfg and os.path.exists(self.cfg["input"]) and \
                os.path.getmtime(self.cfg["input"]) > os.path.getmtime(self.output):
            return "input is newer"
        return None

    def write_cfg(self):
        with open(self.base + ".cfg.in", "w") as f:
            f.write("".join(f"{k} = {v}\n" for k, v in self.cfg.items()))
        return self.base + ".cfg.in"

    def command(self):
        return ["root", "-l", "-b", "-q", f'{MACROS[self.stage]}("{self.write_cfg()}")']

    def run_local(self):
        """Run the macro. Success means the macro wrote its .cfg (RunConfig::Finish)."""
        sidecar = self.base + ".cfg"
        if os.path.exists(sidecar):
            os.remove(sidecar)
        start = time.time()
        with open(self.base + ".log", "w") as log:
            subprocess.run(self.command(), cwd=HERE, stdout=log, stderr=subprocess.STDOUT)
        ok = os.path.exists(sidecar)
        print(f"{'done' if ok else 'FAILED':6} {self} ({time.time() - start:.0f} s)"
              + ("" if ok else f", see {self.base}.log"), flush=True)
        return ok


def plan(study, runs, last_stage, force):
    """Build the jobs needed to bring every run up to last_stage, keyed so shared upstream runs once."""
    jobs = {}
    for run in runs:
        settings, owner = study.settings(run)
        for chunk in range(int(settings.get("chunks", 1))):
            prev = None
            for stage in STAGES[: STAGES.index(last_stage) + 1]:
                job_run = run if stage == "fit" else owner
                key = (job_run, stage, chunk)
                if key in jobs:  # sim/digi shared with an upstream run, already planned
                    prev = key
                    continue
                job = jobs[key] = Job(study.name, job_run, stage, chunk, settings, owner)
                reason = job.stale_reason()
                if force and STAGES.index(stage) >= STAGES.index(force):
                    reason = "forced"
                if prev is not None and jobs[prev].reason:
                    reason = reason or "upstream rerun"
                    job.deps.append(jobs[prev])
                job.reason = reason
                prev = job.key
    return [j for j in jobs.values() if j.reason]


def remove_old_chunking(todo):
    """Delete outputs from a different chunk count, so plots don't pick them up."""
    for job in todo:
        current = {j.output for j in todo if (j.run, j.stage) == (job.run, job.stage)}
        prefix = os.path.join(DATA, f"{job.study}-{job.run}.{job.stage}")
        for path in glob.glob(prefix + ".root") + glob.glob(prefix + ".c[0-9][0-9].root"):
            if path not in current:
                for ext in (".root", ".cfg", ".cfg.in", ".log"):
                    if os.path.exists(path[:-5] + ext):
                        os.remove(path[:-5] + ext)


def run_local(todo, n_jobs):
    """Run jobs in dependency order, up to n_jobs at a time. Dependents of failed jobs are skipped."""
    pending, running, finished, failed = list(todo), {}, set(), set()
    with ThreadPoolExecutor(n_jobs) as pool:
        while pending or running:
            for job in list(pending):
                if any(d in failed for d in job.deps):
                    pending.remove(job)
                    failed.add(job)
                    print(f"skip   {job} (upstream failed)")
                elif all(d in finished for d in job.deps) and len(running) < n_jobs:
                    pending.remove(job)
                    print(f"start  {job}", flush=True)
                    running[pool.submit(job.run_local)] = job
            done, _ = wait(running, return_when=FIRST_COMPLETED)
            for future in done:
                job = running.pop(future)
                (finished if future.result() else failed).add(job)
    return not failed


def run_slurm(todo):
    """Submit each job with sbatch, chained with afterok dependencies."""
    if shutil.which("sbatch") is None:
        sys.exit("sbatch not found")
    ids = {}
    for job in todo:  # plan() returns jobs in dependency order
        threads = job.cfg.get("fit.threads", "1") if job.stage == "fit" else "1"
        cmd = ["sbatch", "--parsable", f"--job-name={job}", f"--output={job.base}.log",
               f"--cpus-per-task={threads}", f"--chdir={HERE}"]
        if job.deps:
            cmd.append("--dependency=afterok:" + ":".join(ids[d.key] for d in job.deps))
        cmd.append("--wrap=" + " ".join(f"'{c}'" for c in job.command()))
        ids[job.key] = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.strip().split(";")[0]
        print(f"submitted {job} as {ids[job.key]}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("study", help="study file (.ini)")
    parser.add_argument("runs", nargs="*", help="runs to process (default: all)")
    parser.add_argument("--stage", choices=STAGES, default="fit", help="last stage to run (default: fit)")
    parser.add_argument("--force", choices=STAGES, help="rerun from this stage down even if up to date")
    parser.add_argument("--dry-run", action="store_true", help="only show what would run")
    parser.add_argument("-j", type=int, default=1, help="jobs to run at once (default: 1)")
    parser.add_argument("--slurm", action="store_true", help="submit jobs with sbatch")
    args = parser.parse_args()

    study = Study(args.study)
    todo = plan(study, args.runs or list(study.sections), args.stage, args.force)
    if not todo:
        print("Everything is up to date.")
        return
    for job in todo:
        print(f"{str(job):40} {job.reason}")
    if args.dry_run:
        return

    os.makedirs(DATA, exist_ok=True)
    remove_old_chunking(todo)
    if args.slurm:
        run_slurm(todo)
    elif not run_local(todo, args.j):
        sys.exit(1)


if __name__ == "__main__":
    main()
