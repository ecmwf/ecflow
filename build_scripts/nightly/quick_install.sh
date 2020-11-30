#!/bin/sh
# assumes:
# - $WK is defined to be root of ecflow tree
# - ecflow is installed to: Linux  - /tmp/${USER}/install/cmake/ecflow/${ECFLOW_VERSION} 
#                           Darwin - $(HOME)/install/ecflow/${ECFLOW_VERSION} 
#   Linux Install using: cd $WK; ./cmake.sh make -j8 install
#   mac   Install using: cd $WK; build_scripts/mac.sh make -j8 install
# - metabuilder is at $WK/../metabuild

# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

cd $WK
export ECF_PORT=4041
PYTHON=python3

os_variant=$(uname -s)
if [[ $os_variant = Darwin ]] ; then

   # we assume $PATH and $PYTHONPATH have been set to locate ecflow_client/ecflow.so
   ECF_HOST=localhost
   export ECF_PORT=4040
   ECF_CLIENT_EXE_PATH=$(which ecflow_client)

else
   ECFLOW_VERSION=$(awk '/^project/ && /ecflow/ && /VERSION/ {for (I=1;I<=NF;I++) if ($I == "VERSION") {print $(I+1)};}' $WK/CMakeLists.txt)      
   install_prefix="/tmp/${USER}/install/cmake"
   export PATH=${install_prefix}/ecflow/${ECFLOW_VERSION}/bin:$PATH
   if [[ $PYTHON == "python3" ]] ; then
      #module load python3
  
      python_dot_version=$(python3 -c 'import sys;print(sys.version_info[0],".",sys.version_info[1],sep="")')
      export PYTHONPATH=${install_prefix}/ecflow/${ECFLOW_VERSION}/lib/python${python_dot_version}/site-packages
   else
      export PYTHONPATH=${install_prefix}/ecflow/${ECFLOW_VERSION}/lib/python2.7/site-packages
   fi
   
   ECF_CLIENT_EXE_PATH=${install_prefix}/ecflow/${ECFLOW_VERSION}/bin/ecflow_client
fi

# =======================================================================
# SSL
# =======================================================================
#export ECF_DEBUG_CLIENT=1
#export ECF_SSL=`hostname`.${ECF_PORT} # use server specfic <host>.<port>.*** certificates
#export ECF_SSL=1                      # use non server specfic  certificates


# =======================================================================
# Kill the server
# =======================================================================
which ecflow_client
ecflow_client --version
set +e # disable error
ecflow_client --terminate=yes >> /dev/null
set -e # stop the shell on first error

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
# Create build scripts files. Must be before python $WK/build_scripts/nightly/build.py
# =======================================================================
cd $SCRATCH
rm -rf nightly
cp -r $WK/build_scripts/nightly .
cd nightly

# =======================================================================
# Clean start
# =======================================================================
rm -rf `hostname`.${ECF_PORT}.*

# =======================================================================
# CUSTOM USER, this needs a ECF_CUSTOM_PASSWD, used for client and server
# =======================================================================
test_custom_user=0
if [[ $test_custom_user -eq 1 ]] ; then
   export ECF_CUSTOM_PASSWD=$SCRATCH/nightly/$(hostname).${ECF_PORT}.ecf.custom_passwd
   export ECF_USER=fred # custom user fred
cat << EOF > ${ECF_CUSTOM_PASSWD}
4.5.0  
# <user> <host> <port> <passwd>
$ECF_USER $(hostname) ${ECF_PORT} my_passwd
EOF
fi

# =======================================================================
# PASSWD access, file used for client and server
# =======================================================================
test_passwd_file=0
if [[ $test_passwd_file == 1 ]] ; then
   export ECF_PASSWD=$SCRATCH/nightly/$(hostname).${ECF_PORT}.ecf.passwd
cat << EOF > ${ECF_PASSWD}
4.5.0  
# <user> <host> <port> <passwd>
$USER $(hostname) ${ECF_PORT} my_passwd
EOF
fi


# =======================================================================
# Start server. 
# =======================================================================
export ECF_TASK_THRESHOLD=1 # 1000 milliseconds = 1 second
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
# ecflow metabuilder, this needs PYTHONPATH for local ecflow
# ======================================================================
cd $WK/../metabuilder
git checkout develop
$PYTHON ./clean.py -s ecflow 
$PYTHON ./generate.py -s ecflow
$PYTHON ./reload.py -s ecflow
#git checkout master


# ========================================================================
# Generate test suites, based on definitions known to be good 
# ========================================================================
cd $WK
#$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/JIRA/ecflow_337_.def
#$PYTHON Pyext/samples/TestBench.py ANode/parser/test/data/good_defs/JIRA/ecflow_1550.def

for defs_file in $(find ANode/parser/test/data/good_defs -type f); do
   echo "->$defs_file"
   $PYTHON Pyext/samples/TestBench.py $defs_file
done
 
# Use python3 for ecflow 5 series
# Use the installed ecflow for ecflow_client, to stop mixing of ecflow 4/5
# must be done after since TestBench.py will use build dir
ecflow_client --alter change variable ECF_CLIENT_EXE_PATH "$ECF_CLIENT_EXE_PATH" /
ecflow_client --order=/ecflow alpha      #  sort suites  
ecflow_client --order=/ecflow top    
  
# =======================================================================
# START the GUI, kill first
# =======================================================================
ps -ef | grep -v awk | awk '/ecflow_ui/ {print $2}' | xargs kill -9 || true

export ECFLOWUI_DEVELOP_MODE=1      # enable special menu to diff ecflowui defs and downloaded defs
#export ECFLOWUI_SESSION_MANAGER=1  # to minimise output for debug, use session with a single server
#ecflow_ui.x > ecflow_ui.log 2>&1 & 
#ecflow_ui -confd ${HOME}/.ecflow5_ui &
ecflow_ui&
