banner trap.h 
#==========================================================================
#  Trap handling and ECF_ initialisation
#
#  Write into the ERROR function what to do in case of any error.
#==========================================================================


#=======================================================================
#  Get the prompt PS4 to be ok with -u option
#=======================================================================
typeset -Z2 _h _m _s
if [[ $ARCH != linux ]] ; then
SECONDS="$(date '+3600*%%H+60*%%M+%%S')"
_x=""
_time='${_x[(_h=(SECONDS/3600)%%24)==(_m=SECONDS/60%%60)==(_s=SECONDS%%60)]}$_h:$_m:$_s'
PS4="$_time + "
elif [[ $ARCH = linux ]] ; then
  set +e
  LKSH93=`isksh93 && echo "true" || echo "false"`
  set -e
  if [[ $ARCH = linux ]] && [[ "$LKSH93" = "true" ]] ; then
# ksh93 syntax
    _time='$((_h=(SECONDS/3600)%%24)):$((_m=(SECONDS/60)%%60)):$((_s=SECONDS%%60))'
    PS4="$_time + "
  fi
fi
unset _hh _mm || true



typeset -l ARCH 
TASK_START_TIME=$(date +%%s)

###########################################################################
export BOOST_ROOT=%BOOST_DIR%/%BOOST_VERSION%
echo $BOOST_ROOT

# Set PATH in case job files reference ecflow_client
# NOTE: We DONT hard wire path to python interpreter.
#       This can be changed in site-config.jam
ECFLOW_INSTALL_PATH=%ECFLOW_LAST_INSTALLED_VERSION%
export PATH=$PATH:/usr/local/bin:$ECFLOW_INSTALL_PATH/bin

# print out python version: Used to check we have the right paths
python --version

# do not export ECF_HOME unless there is good reason
ECF_HOME=%ECF_HOME%


###########################################################################
# set -a means export all variables until closing set +a
set -a
LOADL_STEP_ID=${LOADL_STEP_ID:=NOT_SET}
QSUB_REQID=${QSUB_REQID:=NOT_SET}
PBS_JOBID=${PBS_JOBID:=NOT_SET}

if [[ $LOADL_STEP_ID != NOT_SET ]] ; then
  ECF_RID=$(echo $LOADL_STEP_ID | cut -f1 -d.)
  ECF_RID=$(substring $ECF_RID 6 8  )
  ECF_RID=$(echo $LOADL_STEP_ID | cut -f2 -d.)${ECF_RID}
  JOB_ID=$LOADL_STEP_ID
elif [[ $QSUB_REQID != NOT_SET ]] ; then
  ECF_RID=$(echo $QSUB_REQID | cut -f1 -d.)
  JOB_ID=$QSUB_REQID
elif [[ $PBS_JOBID != NOT_SET ]] ; then
  ECF_RID=$(echo $PBS_JOBID | cut -f1 -d.)
  JOB_ID=$PBS_JOBID

  TOPATH=%TOPATH:/tmp/output%
  link_name=`~emos/bin/subs_path.pl -f %ECF_OUT% -t $TOPATH -n %ECF_JOBOUT%`
  mkdir -p `dirname $link_name` || : 
  ln -sf /var/spool/PBS/spool/${PBS_JOBID}.OU $link_name || :
  rm -f %ECF_JOBOUT% || :
  ln -sf /var/spool/PBS/spool/${PBS_JOBID}.OU %ECF_JOBOUT%
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
  STREAM=${STREAM:=""}
  if [[ "$STREAM" = @(SEAS|MOFC|OCEA|mnfc|mnfh) ]] ; then
    if [[ "%TASK%" = logfiles ]] ; then
      n=0
      while [[ -d $WDIR ]] ; do
        # Forked processes could still be around, and 'rm -rf' fails
        rm -rf $WDIR || true
        ls -l $WDIR && sleep 2 || true
        n=$((n+1))
        [[ $n -gt 5 ]] && break
      done || true
    fi
  fi
  FSFAMILY=${FSFAMILY:=""}
  if [[ "$FSFAMILY" = /mars || "%FAMILY1:%" = archive ]] ; then
    cd $TMPDIR
    n=0
    while [[ -d ${WDIR}$ECF_NAME ]] ; do
      # Forked processes could still be around, and 'rm -rf' fails
      rm -rf ${WDIR}$ECF_NAME || true
      ls -l ${WDIR}$ECF_NAME && sleep 2 || true
      n=$((n+1))
      [[ $n -gt 5 ]] && break
    done || true
  fi

  if [[ $ARCH = linux ]] ; then
    . /usr/local/share/ecmwf/share/.epilog
  fi

  if [[ %CLEAN_ON_EXIT:1% = 1 ]] ; then
  if [[ $EMOS_TMPDIR = /var/tmp/tmpdir/%TASK%_%ECF_TRYNO%.$$ ]] ; then
    # clean temporary directory on local workstation
    rm -rf  $EMOS_TMPDIR || :
  else
    rmdir $EMOS_TMPDIR || :
  fi
  fi
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
  times
  echo "environment was:"
  printenv | sort

  ECF_CLEAN

  if [[ $$ARCH != "ibm_power*" ]] ; then
    ECF_RCP || :
  fi
  exit 1
}

if [[ "$ARCH" != rs6000 ]] ; then 
  chmod 644 %LOGDIR%%ECF_NAME%.%ECF_TRYNO%
fi

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


[[ -d $TMPDIR ]] && cd $TMPDIR

#=======================================================================
# Tell ecflow we have that job has started
#=======================================================================
$ECFLOW_INSTALL_PATH/bin/ecflow_client --init=$ECF_RID --host=%ECF_NODE% --port=%ECF_PORT%

date

