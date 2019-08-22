ATTPCROOT is a ROOT-based (root.cern.ch) framework to analyze data of the ATTPC detector (Active Target Time Projection Chamber) and the p-ATTPC (Prototype). For reference http://ribf.riken.jp/ARIS2014/slide/files/Jun2/Par2B06Bazin-final.pdf. The detector is based at the NSCL but experiments are performed at other facilities as well (Notre Dame, TRIUMF, Argonne...).
 
The framework allows the end user to unpack and analyze the data, as well as perform realistic simulations based on a Virtual Monte Carlo (VMC) package. The framework needs external libraries (FairSoft and FairRoot https://fairroot.gsi.de/) to be compiled and executed, which are developed by other groups and not directly supported by the AT-TPC group. Please refer to their forums for installation issues of these packages.

# Installation on NSCL cluster

ROOT and FairROOT are already installed on the system. The modules and their prerequisites just need to be loaded with the commands:
```
module load gnu/gcc/6.4
module load fairroot
```

To install ATTPCROOT checkout the repository from github and then create a folder in the repository to build the code and cd into it. From this directory you can call CMake to configure the build. Then you can build the source
```
git clone https://github.com/ATTPC/ATTPCROOTv2.git
cd ATTPCROOTv2
mkdir build
cd build
cmake ../
make -j 4
```

After the source builds, you should be good to go.

# Creating geometry files

### Adding materials

Each material that can be used in a detector is defined in the global file `geometry/media.geo`. If you would like to add any new materials, it can be done in that file. The header has detailed instructions for adding new materials.

### Creating detectors

There are a number of scripts in geometry/ for building detector geometries. Each of these output two root files that are then passed to the simulation code. In these script files, all of the materials of the detector can be changed including the gas in the attpc, and window material. If the ion chamber or some other detector is present those can also be added.

The file ATTPC_v1_2.C has the basic geometry for the full sized TPC. The file ATTPC_Proto_v1_0.C ha the basic geometry for the pATTPC. Both of these files are pretty self explanitory if opened.

To generate the root files that will be used for the physics simulation just run the macro `root ATTPC_v1_2.C`. The simulation macros will look in the `geometry/` folder for these files.

# Running simulations

### Generating physics

The folder 