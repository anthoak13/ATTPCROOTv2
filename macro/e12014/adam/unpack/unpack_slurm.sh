#!/bin/bash
#SBATCH --job-name=unpack_fission
#SBATCH --output=log/unpack_fission_%j.out
#SBATCH --error=log/unpack_fission_%j.err
#SBATCH --partition=cs
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem-per-cpu=2G
#SBATCH --time=UNLIMITED
#SBATCH --mail-type=END
#SBATCH --mail-user=aanthon2@highpoint.edu
#SBATCH --chdir=/home/faculty/aanthony/attpcroot/macro/e12014/adam/unpack


# The --cpus-per-task option is what sets the number of files we will
# unpack at once 

# The --chdir is the directory you are working in. It would you your directory
# in the macro folder where you run these scripts manually.

echo "Begining to unpack files"

#The following command is what executes and manages all of the processes
#When one run finishes, it will automatically start the next in the list
#It will print a summary to a log file
./parallel --jobs ${SLURM_CPUS_ON_NODE} --joblog ./log/150torr_${SLURM_JOB_ID}.log < ./input/150torr.txt

echo "Finished unpacking files"