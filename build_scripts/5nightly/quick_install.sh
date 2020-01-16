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
ecflow_client --terminate=yes >> /dev/null
 
set +e # ignore error 
count=0
while [ 1 ] ; do   
    ecflow_client --ping 2> /dev/null
    if [[ $? == 1 ]] ; then
        echo "server terminates after $count seconds"
        break
    fi
    sleep 1
    count=$((count + 1))
    #echo $count
    if [ "$count" -gt "3" ] ; then
        echo "Timed out after 3 seconds"
        break
    fi
done
set -e  # re-enable error

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

# wait for server to start
set +e # ignore error 
count=0
while [ 1 ] ; do   
    ecflow_client --ping 2> /dev/null
    if [[ $? == 0 ]] ; then
        echo "server up and running after $count seconds"
        break;
    fi
    sleep 1
    count=$((count + 1))
    #echo $count
    if [ "$count" -gt "4" ] ; then
        echo "Timed out after 4 seconds"
        exit 1
    fi
done
set -e  # re-enable error 

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
for defs_file in $(find ANode/parser/test/data/good_defs -type f); do
   echo "->$defs_file"
   $PYTHON Pyext/samples/TestBench.py $defs_file
done
 
# Use the installed ecflow for ecflow_client, to stop mixing of ecflow 4/5
# must be done after since TestBench.py will use build dir
ecflow_client --alter change variable ECF_CLIENT_EXE_PATH "/tmp/${USER}/install/cmake/ecflow/${ECFLOW_VERSION}/bin/ecflow_client" /
ecflow_client --alter change variable METAB_PYTHON_VERSION $PYTHON /ecflow
       
       
# =======================================================================
# Start the GUI
# =======================================================================
export ECFLOWUI_DEVELOP_MODE=1      # enable special menu to diff ecflowui defs and downloaded defs
#export ECFLOWUI_SESSION_MANAGER=1  # to minimise output for debug, use session with a single server
#ecflow_ui.x > ecflow_ui.log 2>&1 & 
ecflow_ui -confd ${HOME}/.ecflow5_ui &

