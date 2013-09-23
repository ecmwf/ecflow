
if [[ ${LIST_VARDIR:=no} = yes ]] ; then
  ls -l || true
fi

#=======================================================================
#  End task.
#  Clean up and exit
#=======================================================================

wait
# TASK_DURATION=$(($(date +%%s) - $TASK_START_TIME))
$ECFLOW_INSTALL_PATH/bin/ecflow_client --complete  --host=%ECF_NODE% --port=%ECF_PORT%

trap 0

set +ex

if [[ $ARCH != ibm_power* ]] ; then
ECF_RCP
fi

ECF_CLEAN

# ECF_JOB_CMD is %ECF_JOB_CMD%
# WSHOST is %WSHOST%
# SCHOST is %SCHOST%
