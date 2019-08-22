# Configuring the enviroment

In attpcROOT/nate/ATTPCROOTv2 `source build/config.sh`. This sets the root version to 6.12/06, but doesn't load in the newest version of all of compiler libraries.

To load the proper libraries run `module load gnu/gcc/6.4` and `module load fairroot`

All of this is done for you if you `source env.sh`

# Info on setup
ATTPCROOT seems to be based on the FairRoot project template project_root_containers. Most of the simulation macros seem to be based on tutorial4 from fairroot.

# Creating a geometry file

# Running a simulation

# 