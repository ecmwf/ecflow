#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

#set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

#export ECF_DEBUG_CLIENT=1
ECFLOW_VERSION=5.1.0
#export ECF_SSL=polonius.4142 # use server specfic <host>.<port>.*** certificates
export ECF_PORT=4142
export PATH=/tmp/ma0/install/cmake/ecflow/${ECFLOW_VERSION}/bin:$PATH
PYTHON=python3
if [[ $PYTHON == "python3" ]] ; then
   module load python3
   export PYTHONPATH=/tmp/ma0/install/cmake/ecflow/${ECFLOW_VERSION}/lib/python3.6/site-packages
else
   export PYTHONPATH=/tmp/ma0/install/cmake/ecflow/${ECFLOW_VERSION}/lib/python2.7/site-packages
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
rm -rf `hostname`.${ECF_PORT}.*


ecflow_server&
sleep 4
ecflow_client --server_version

# =======================================================================
# start with a clean slate
# =======================================================================
ecflow_client --restart
ecflow_client --delete=_all_ yes


# ======================================================================
# ecflow metabuilder.  
# ======================================================================
cd /var/tmp/ma0/workspace/metabuilder
git checkout develop
$PYTHON ./clean.py -s ecflow 
$PYTHON ./generate.py -s ecflow
$PYTHON ./reload.py -s ecflow
git checkout master

# ========================================================================
# test suites
# ========================================================================
cd $WK
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/trigger/all_trigger_examples.def
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/limit/sub_only1.def
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/limit/inlimit_node.def
       
# =======================================================================
# Start the GUI
# =======================================================================
cd $SCRATCH
ecflow_ui &
