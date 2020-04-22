#!/bin/bash

#source build/config.sh

module purge
module load gnu/gcc/6.4
module load fairroot/18.00

export VMCWORKDIR="/mnt/simulations/attpcroot/adam/ATTPCROOTv2"
export FAIRLIBDIR="/mnt/simulations/attpcroot/adam/ATTPCROOTv2/build/lib"


export ROOT_LIBRARY_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib/root"
export ROOT_LIBRARIES="-L/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib/root -lGui -lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -Wl,-rpath,/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib/root -lm -ldl -rdynamic"

export ROOT_INCLUDE_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/include/root"
export ROOT_INCLUDE_PATH="/mnt/simulations/attpcroot/adam/ATTPCROOTv2/include:/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairroot/include"

export LD_LIBRARY_PATH=$FAIRLIBDIR:$LD_LIBRARY_PATH


#Geant 4 stuff
export GEANT4_LIBRARY_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib"
export GEANT4_INCLUDE_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/include/Geant4:/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/transport/geant4/source/interfaces/common/include:/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/transport/geant4/physics_lists/hadronic/Packaging/include:/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/transport/geant4/physics_lists/hadronic/QGSP/include"
export GEANT4VMC_INCLUDE_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/include/geant4vmc"
export GEANT4VMC_LIBRARY_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib"
export GEANT4VMC_MACRO_DIR="GEANT4VMC_MACRO_DIR-NOTFOUND"
export CLHEP_INCLUDE_DIR=""
export CLHEP_LIBRARY_DIR=""
export CLHEP_BASE_DIR=""
export PLUTO_LIBRARY_DIR=""
export PLUTO_INCLUDE_DIR=""
export PYTHIA6_LIBRARY_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib"
export G3SYS="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/geant3"
export GEANT3_INCLUDE_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/include/TGeant3"
export GEANT3_LIBRARY_DIR="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib"
export GEANT3_LIBRARIES="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/lib/libgeant321.so"
export USE_VGM="1"
export PYTHIA8DATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/pythia8/xmldoc"
export CLASSPATH=""
export G4LEDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4EMLOW"
export G4LEVELGAMMADATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/PhotonEvaporation"
export G4NeutronHPCrossSections="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4NDL"
export G4NEUTRONHPDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4NDL"
export G4NEUTRONXSDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4NEUTRONXS"
export G4PARTICLEXSDATA=""
export G4PIIDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4PII"
export G4RADIOACTIVEDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/RadioactiveDecay"
export G4REALSURFACEDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/RealSurface"
export G4SAIDXSDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4SAIDDATA"
export G4ENSDFSTATEDATA="/mnt/misc/sw/x86_64/Debian/8/fairroot/18.00/fairsoft/share/Geant4/data/G4ENSDFSTATE"
export G4INCLDATA=""
