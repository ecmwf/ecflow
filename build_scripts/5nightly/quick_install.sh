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
ECFLOW_VERSION=5.2.0
#export ECF_SSL=`hostname`.4142 # use server specfic <host>.<port>.*** certificates
export ECF_PORT=4142
export PATH=/tmp/${USER}/install/cmake/ecflow/${ECFLOW_VERSION}/bin:$PATH
PYTHON=python3
if [[ $PYTHON == "python3" ]] ; then
   module load python3
   export PYTHONPATH=/tmp/${USER}/install/cmake/ecflow/${ECFLOW_VERSION}/lib/python3.6/site-packages
else
   export PYTHONPATH=/tmp/${USER}/install/cmake/ecflow/${ECFLOW_VERSION}/lib/python2.7/site-packages
fi

# =======================================================================
# Kill the server
# =======================================================================
which ecflow_client
ecflow_client --version
ecflow_client --terminate=yes
sleep 3

# =======================================================================
# Create build scripts files. Must be before python $WK/build_scripts/5nightly/build.py
# =======================================================================
rm -rf 5nightly
cp -r $WK/build_scripts/5nightly .
cd 5nightly

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
cd /var/tmp/${USER}/workspace/metabuilder
git checkout develop
$PYTHON ./clean.py -s ecflow 
$PYTHON ./generate.py -s ecflow
$PYTHON ./reload.py -s ecflow
git checkout master

# ========================================================================
# test suites. Use installed ecflow:
# ========================================================================
cd $WK
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/limit/basic.def
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/trigger/all_trigger_examples.def
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/limit/sub_only1.def
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/limit/inlimit_node.def
$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/label/multi_line_lables.def 
                                   ANode/parser/test/data/good_defs/label/multi_line_lables.def
# Use the installed ecflow   
# must be done after since TestBench.py will use build dir
ecflow_client --alter change variable ECF_CLIENT_EXE_PATH "/tmp/${USER}/install/cmake/ecflow/${ECFLOW_VERSION}/bin/ecflow_client" /
       
       
# =======================================================================
# Start the GUI
# =======================================================================
cd $SCRATCH
export ECFLOWUI_DEVELOP_MODE=1      # enable special menu to diff ecflowui defs and downloaded defs
#export ECFLOWUI_SESSION_MANAGER=1  # to minimise output for debug, use session with a single server
#ecflow_ui.x > ecflow_ui.log 2>&1 & 
ecflow_ui -confd ${HOME}/.ecflow5_ui &

