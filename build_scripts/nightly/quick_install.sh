#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

ECFLOW_VERSION=4.16.0
export ECF_PORT=4141
export PATH=/tmp/ma0/install/cmake/ecflow/${ECFLOW_VERSION}/bin:$PATH
PYTHON=python
if [[ $PYTHON == "python3" ]] ; then
   module load python3
   export PYTHONPATH=/tmp/ma0/install/cmake/ecflow/${ECFLOW_VERSION}/lib/python3.6/site-packages
else
   export PYTHONPATH=/tmp/ma0/install/cmake/ecflow/${ECFLOW_VERSION}/lib/python2.7/site-packages
fi


# =======================================================================
# Create build scripts files. Must be before python $WK/build_scripts/nightly/build.py
# =======================================================================
rm -rf nightly
cp -r $WK/build_scripts/nightly .

# =======================================================================
# Kill the server
# =======================================================================
which ecflow_client
ecflow_client --version
ecflow_client --terminate=yes

# =======================================================================
# Start server. 
# =======================================================================
rm -rf `hostname`.${ECF_PORT}.*

#export ECF_ALLOW_OLD_CLIENT_NEW_SERVER=9
ecflow_server&
sleep 4
ecflow_client --server_version

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
python Pyext/samples/TestBench.py  ANode/parser/test/data/good_defs/limit/basic.def
python Pyext/samples/TestBench.py  ANode/parser/test/data/good_defs/trigger/all_trigger_examples.def
python Pyext/samples/TestBench.py  ANode/parser/test/data/single_defs/test_time_why.def

       
# =======================================================================
# Start the GUI
# =======================================================================
cd $SCRATCH
export ECFLOWUI_DEVELOP_MODE=1      # enable special menu to diff ecflowui defs and downloaded defs
#export ECFLOWUI_SESSION_MANAGER=1  # to minimise output for debug, use session with a single server
#ecflow_ui.x > ecflow_ui.log 2>&1 & 
ecflow_ui &

