#!/bin/bash

#Break if something errors
set -e
echo "Renaming all cpp source files (cxx, cpp, cc) to .cc and all header mathing header files to hh. A matching header file is one that has the same name as a cpp source file in the same directory as the source file."

#Make sure $VMCWORKDIR is set. 
if [ -z "$VMCWORKDIR" ]
then
    echo "VMCWORKDIR is unset. Please run config.sh on build directory first. Aborting..."
    return
else
    echo "VMCWORKDIR is set to '$VMCWORKDIR'. Using indent profile $VMCWORKDIR/.indent.pro"
fi

find -L . -type f | grep -w cxx$ | xargs -I fname bash -c 'A=fname; mv ${A%.*}.h ${A%.*}.hh'
find -L . -type f | grep -w cxx$ | xargs -I fname bash -c 'A=fname; mv fname ${A%.*}.cc'

find -L . -type f | grep -w cpp$ | xargs -I fname bash -c 'A=fname; mv ${A%.*}.h ${A%.*}.hh'
find -L . -type f | grep -w cpp$ | xargs -I fname bash -c 'A=fname; mv fname ${A%.*}.cc'

#Look for all .h files that are not linkdef, or in the following directories
#build, cmake, CMakeFiles, compiled, macro
# then rename them to .hh
#find -L $VMCWORKDIR -type f -name '*.h' ! -iname '*LinkDef*' -print -o \
#     \( -path $VMCWORKDIR/build -o -path $VMCWORKDIR/cmake -o \
#     -path $VMCWORKDIR/CMakeFiles -o -path $VMCWORKDIR/compiled -o -path $VMCWORKDIR/macro \) -prune | \
#    xargs -I fname bash -c 'A=fname; echo fname ${A%.*}.hh'

#Repeat with same exclusions for cpp and cxx files

echo "Success! Renamed all files"
