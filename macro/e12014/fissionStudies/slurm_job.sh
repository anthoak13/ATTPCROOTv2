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
# Any other @key@ is the job's slurm.<key> setting, which must be set (no defaults), from lowest to
# highest priority: the remote in remotes.ini (or --slurm-set), the study's [defaults]
# (<stage>.slurm.<key>), the section (slurm.<key>). To add an sbatch option, add a line here and set
# the value in one of those places. Comment lines (# but not #SBATCH, so also ##SBATCH) are left as
# they are.
# Jobs that wait for another job get --dependency on the sbatch command line.
#SBATCH --job-name=@job@
#SBATCH --output=@log@
#SBATCH --chdir=@dir@
#SBATCH --cpus-per-task=@cpus@
#SBATCH --time=@time@
#SBATCH --mem=@mem@

@command@
