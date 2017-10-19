#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed


if [[ "$#"  == 0 ]] ; then
   export PATH=/tmp/ma0/install/cmake/ecflow/4.8.0/bin:$PATH
   export PYTHONPATH=/tmp/ma0/install/cmake/ecflow/4.8.0/lib/python2.7/site-packages
else
   module unload ecflow
   module load ecflow/dev
fi

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

# ========================================================================
# test suites
# ========================================================================
cd $WK
python Pyext/samples/TestBench.py  ANode/parser/test/data/good_defs/trigger/all_trigger_examples.def

       
# =======================================================================
# Start the GUI
# =======================================================================
cd $SCRATCH
ecflow_ui &
