
if [[ ${LIST_VARDIR:=no} = yes ]] ; then
  ls -l || true
fi

#=======================================================================
#  End task.
#  Clean up and exit
#=======================================================================

# put some information about pes and threads into the logfile:
threads=%THREADS:1%
npes=${NPES:-1}
if [[ $threads -gt 1 || $npes -gt 1 ]] ; then
   $ECFLOW_INSTALL_PATH/bin/ecflow_client --msg "NPES-THREADS:%ECF_NAME%: $npes - $threads ." --host=%ECF_NODE% --port=%ECF_PORT%
fi

wait
# TASK_DURATION=$(($(date +%%s) - $TASK_START_TIME))
# ECFLOW_INSTALL_PATH/bin/ecflow_client --msg "$ECF_NAME: duration $TASK_DURATION" --host=%ECF_NODE% --port=%ECF_PORT%
$ECFLOW_INSTALL_PATH/bin/ecflow_client --complete  --host=%ECF_NODE% --port=%ECF_PORT%

trap 0

set +ex

if [[ $ARCH != ibm_power* ]] ; then
ECF_RCP
fi

#sleep for a few seconds to allow next task to get into queue
sleep %EMOS_SLEEP:0%
etmp=${EMOS_TMPDIR:-0}
if [[ $etmp = 0 ]] ; then
  echo "EMOS_TMPDIR not defined"
elif [[ $etmp = /var/tmp/tmpdir/%TASK%_%ECF_TRYNO%.$$ ]] ; then
  # clean temporary directory on local workstation
  rm -rf  $etmp || :
else
  rmdir $etmp || :
fi

ECF_CLEAN

if [[ $ARCH = linux ]] ; then
  exit # in comment to enable step2.h rcp
fi
# ECF_JOB_CMD is %ECF_JOB_CMD%
# WSHOST is %WSHOST%
# SCHOST is %SCHOST%
