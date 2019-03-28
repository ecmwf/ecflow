#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status


if [[ "$#"  == 0 ]] ; then
   export ECF_PORT=4142
   export PATH=/tmp/ma0/install/cmake/ecflow/5.0.0/bin:$PATH
   export PYTHONPATH=/tmp/ma0/install/cmake/ecflow/5.0.0/lib/python2.7/site-packages
else
   module unload ecflow
   module load ecflow/new
fi

# =======================================================================
# Create build scripts files. Must be before python $WK/build_scripts/5nightly/build.py
# =======================================================================
rm -rf 5nightly
cp -r $WK/build_scripts/5nightly .

# =======================================================================
# Kill the server
# =======================================================================
which ecflow_client
ecflow_client --version
ecflow_client --terminate=yes

# =======================================================================
# Start server. 
# =======================================================================
rm -rf `hostname`.4142.*

#export ECF_ALLOW_OLD_CLIENT_NEW_SERVER=9
ecflow_server&
sleep 4
ecflow_client --server_version

# Make sure server is running
# =======================================================================
python $WK/build_scripts/5nightly/load.py


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
python Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/trigger/all_trigger_examples.def
python Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/limit/sub_only1.def
python Pyext/samples/TestBench.py ANode/parser/test/data/single_defs/test_time_why.def
 
       
# =======================================================================
# Start the GUI
# =======================================================================
cd $SCRATCH
ecflow_ui &
