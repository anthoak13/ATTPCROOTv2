#!/bin/bash
# Job script template for fission.py --slurm. For each job, fission.py replaces every @name@ and
# submits the result with sbatch. Edit freely.
#
# Set by fission.py:
#   @job@      job name, <study>-<name>.<stage>[.cNN]
#   @log@      the job's log file
#   @dir@      directory the job runs in
#   @cpus@     CPUs: fit.threads for a fit, 1 otherwise
#   @command@  the commands that run the stage (keep it)
# Any other @key@ is the job's slurm.<key> setting set (in priority order) by
# remotes.ini (or --slurm-set), the study's [defaults], the section (slurm.<key>).

#SBATCH --job-name=@job@
#SBATCH --output=@log@
#SBATCH --chdir=@dir@
#SBATCH --cpus-per-task=@cpus@
#SBATCH --time=@time@
#SBATCH --mem=@mem@

@command@
