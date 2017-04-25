#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

module unload ecflow
module load ecflow/dev

# =======================================================================
# Create build scripts files. Must be before python $WK/build_scripts/nightly/build.py
# =======================================================================
rm -rf nightly
cp -r $WK/build_scripts/nightly .

# =======================================================================
# Generate the defs, the is loaded into the server by load.py
# =======================================================================
python $WK/build_scripts/nightly/build.py
if [[ $? = 1 ]] ; then
   exit 1
fi

# =======================================================================
# Kill the server
# =======================================================================
export ECF_PORT=4141
which ecflow_client
ecflow_client --version
ecflow_client --terminate=yes

# =======================================================================
# Start server. 
# =======================================================================
rm -rf `hostname`.4141.*

#export ECF_ALLOW_OLD_CLIENT_NEW_SERVER=9
ecflow_server&
sleep 4
ecflow_client --server_version

# =======================================================================
# load the build defs, in the server then delete generated defs.
# Make sure server is running
# =======================================================================
python $WK/build_scripts/nightly/load.py


# ======================================================================
# ecflow metabuilder
# ======================================================================
cd /var/tmp/ma0/workspace/metabuilder
git checkout develop
./regenerate.sh ecflow
git checkout master

# =======================================================================
# Start the GUI
# =======================================================================
cd $SCRATCH
module swap ecflow/new
ecflow_ui &
