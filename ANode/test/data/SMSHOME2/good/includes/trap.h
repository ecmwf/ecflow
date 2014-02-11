banner trap.h

## Copyright 2009-2012 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.


#==========================================================================
#  Trap handling and ECF_ initialisation
#
#  Write into the ERROR function what to do in case of any error.
#==========================================================================

typeset -l ARCH

if [[ "$ARCH" = hpia64 ]] ; then
  rcp /home/ma/emos/data/dummy.output emos@%ECF_NODE%:%ECF_JOBOUT% || true
fi

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

ECF_PASS=%ECF_PASS%
ECF_NODE=%ECF_NODE%
ECF_NAME=%ECF_NAME%
ECF_TRYNO=%ECF_TRYNO%
ECF_HOSTFILE=$HOME/.smshostfile
ECF_JOBOUT=%ECF_JOBOUT%

set +a
ARCHWDIR=${ARCHWDIR:=""}

SMSCLEAN() {
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
  if [[ "$FSFAMILY" = /mars || "%FAMILY1%" = archive ]] ; then
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
}
typeset -fx SMSCLEAN

%include <rcp.h>

ERROR() {
  set -x
  set +e
  wait
  smsabort
  trap 0
  date
  times
  echo "environment was:"
  printenv | sort

  SMSCLEAN

  if [[ $$ARCH != "ibm_power*" ]] ; then
    SMSRCP || :
  fi
  exit 1
}

if [[ $ARCH == hpia64 ]]; then
  export SMS_SIGNAL_LIST='1 2 3 4 5 6 7 8 10 12 13 15 24 30 33'
elif [[ $ARCH == ibm_power* ]]; then
  export SMS_SIGNAL_LIST='1 2 3 4 5 6 7 8 10 12 13 15 24 30'
elif [[ $ARCH == linux ]]; then
  export SMS_SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31'
else
  export SMS_SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31'
fi

%include <set_traps.h>
trap
set -exu

[[ -d $TMPDIR ]] && cd $TMPDIR

smsinit $ECF_RID &
smsmsg "#ID ECF_NAME=%ECF_NAME%;ECF_JOBOUT=%ECF_JOBOUT%;ECF_HOME=%ECF_HOME%;JOB_ID=${JOB_ID:-}"

date

