#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# copy associated scripts files
rm -rf nightly
cp -r $WK/build_scripts/nightly .

# Generate the defs
python $WK/build_scripts/nightly/build.py

# load the generated defs, *ASSUMES* server is running
python $WK/build_scripts/nightly/load.py

# start the GUI
module swap ecflow/dev
ecflow_ui &
