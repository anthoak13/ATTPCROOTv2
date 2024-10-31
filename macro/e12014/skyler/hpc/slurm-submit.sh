#!/bin/bash
#SBATCH --job-name=sim_fission_fit
#SBATCH --output=log/run_%j/log.out
#SBATCH --error=log/run_%j/log.out
#SBATCH --partition=cs
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=1G
#SBATCH --time=UNLIMITED
#SBATCH --chdir=/home/faculty/aanthony/fission/adam/ATTPCROOTv2/macro/e12014/skyler/hpc

# This macro will generate fisison events and then fit them using the MC fitting code
# The number of events is set by the NUM_EVENTS variable
# The number of threads to use when running each job is set by the SLURM_CPUS_ON_NODE variable
# The run number is set to be the same as the job number (the %j variable)
# The output will be saved to the log directory of the current directory

NUM_EVENTS=1
SAVE_RAW_EVENT=true
LOG_FOLDER="log/run_${SLURM_JOB_ID}/"

echo "For run number  ${SLURM_JOB_ID}"
echo "Starting to simulate fission events at  $(date)"

# The following command will generate the fission events
root -l -q -b "slurm_sim_fiss.C(${SLURM_JOB_ID}, ${NUM_EVENTS})" &> ${LOG_FOLDER}/sim.log

echo "Finished to simulating fission events at  $(date)"
echo "Starting to fit fission events at  $(date) with ${SLURM_CPUS_ON_NODE} threads"

# The following command will fit the fission events
root -l -q -b "slurm_fit_sim.C(${SLURM_JOB_ID}, ${SLURM_CPUS_ON_NODE}, ${SAVE_RAW_EVENT})" &> ${LOG_FOLDER}/fit.log

echo "Finished fitting fission events at  $(date)"