banner trap.h 
#==========================================================================
#  Trap handling and ECF_ initialisation
#
#  Write into the ERROR function what to do in case of any error.
#==========================================================================

typeset -l ARCH 
TASK_START_TIME=$(date +%%s)

###########################################################################
export BOOST_ROOT=%BOOST_DIR%/%BOOST_VERSION%

# Set PATH in case job files reference ecflow_client
# NOTE: /usr/local/bin defines path to python interpreter
ECFLOW_INSTALL_PATH=%ECFLOW_LAST_INSTALLED_VERSION%
export PATH=$PATH:/usr/local/bin:$ECFLOW_INSTALL_PATH/bin

# print out python version: Used to check we have the right paths
python --version

# do not export ECF_HOME unless there is good reason
ECF_HOME=%ECF_HOME%

###########################################################################
# set -a means export all variables until closing set +a
set -a
set -x
LOADL_STEP_ID=${LOADL_STEP_ID:=NOT_SET}
QSUB_REQID=${QSUB_REQID:=NOT_SET}
PBS_JOBID=${PBS_JOBID:=NOT_SET}

if [[ $LOADL_STEP_ID != NOT_SET ]] ; then
  ECF_RID=$LOADL_STEP_ID
  JOB_ID=$LOADL_STEP_ID
elif [[ $QSUB_REQID != NOT_SET ]] ; then
  ECF_RID=$(echo $QSUB_REQID | cut -f1 -d.)
  JOB_ID=$QSUB_REQID
elif [[ $PBS_JOBID != NOT_SET ]] ; then
  ECF_RID=$(echo $PBS_JOBID | cut -f1 -d.)
  JOB_ID=$PBS_JOBID
else
  ECF_RID=$$
  JOB_ID=$ECF_RID
fi

# Note: We do *NOT* _export_ ECF_PORT,ECF_NODE to avoid interferences
# from the ecflow regression tests. which also uses ECF_PORT,ECF_NODE, instead we explicitly
# set the host,port and rid on each ecflow_client call,
# i.e  --host=%ECF_NODE% --port=%ECF_PORT%
# This is only required because we are using ecflow to test ecflow
ECF_PASS=%ECF_PASS%
ECF_NAME=%ECF_NAME%
ECF_TRYNO=%ECF_TRYNO%
ECF_TIMEOUT=300
ECF_HOSTFILE=$HOME/.ecfhostfile
ECF_JOBOUT=%ECF_JOBOUT%

set +a
###################################################################

ARCHWDIR=${ARCHWDIR:=""}

ECF_CLEAN() {
  set -x
  set +e
}
typeset -fx ECF_CLEAN

%include <rcp.h>

ERROR() {
  set -x
  set +e
  wait
  $ECFLOW_INSTALL_PATH/bin/ecflow_client --abort=trap  --host=%ECF_NODE% --port=%ECF_PORT%
  trap 0
  date
#  times
#  echo "environment was:"
#  printenv | sort
#
#  ECF_CLEAN
#
#  if [[ $$ARCH != "ibm_power*" ]] ; then
#    ECF_RCP || :
#  fi
  exit 1
}

case $ARCH in
 hpia64 ) export ECF_SIGNAL_LIST='1 2 3 4 5 6 7 8 10 12 13 15 24 30 33';;
 ibm_power* ) export ECF_SIGNAL_LIST='1 2 3 4 5 6 7 8 10 12 13 15 24 30';;
 linux ) export ECF_SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31';;
 *) export ECF_SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31';;
esac

trap ERROR 0

for sig in $ECF_SIGNAL_LIST
do
  trap "{ echo \"Signal $(kill -l $sig) ($sig) received\"; ERROR ; }" $sig
done
	
trap
set -exu

# [[ -d $TMPDIR ]] && cd $TMPDIR

$ECFLOW_INSTALL_PATH/bin/ecflow_client --init=$ECF_RID --host=%ECF_NODE% --port=%ECF_PORT%

date

