#!/bin/sh
# assume $WK and $SCRATCH is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# copy associated scripts files
cd $SCRATCH
cp -r $WK/build_scripts/nightly .
