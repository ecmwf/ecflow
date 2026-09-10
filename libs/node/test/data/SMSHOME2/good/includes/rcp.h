# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#  rcp.h

SMSRCP() {
  set -x
  set +e
  if [[ "${ARCH:=UNDEF}" = hppa || "${ARCH:=UNDEF}" = hpia64 ]] ; then
    if [[ $PBS_JOBID = NOT_SET ]] ; then
      rcp %LOGDIR%%SMSNAME%.%SMSTRYNO% \
          emos@%SMSNODE%:%SMSOUT%%SMSNAME%.%SMSTRYNO%
    else
      rm -f %SMSJOBOUT% || : # remove link

      # hardcoded to hanfs2 then mordred to release load on ablamor
      rcp /var/spool/PBS/spool/${PBS_JOBID}.OU \
        emos@hanfs2:/hanfs%SMSHOME%%SMSNAME%.%SMSTRYNO% || \
      rcp /var/spool/PBS/spool/${PBS_JOBID}.OU \
        emos@mordred:%SMSHOME%%SMSNAME%.%SMSTRYNO% || \
      rcp /var/spool/PBS/spool/${PBS_JOBID}.OU \
        emos@%SMSNODE%:%SMSHOME%%SMSNAME%.%SMSTRYNO%

      link_name=`~emos/bin/subs_path.pl -f %SMSOUT% -t $TOPATH -n %SMSJOBOUT%`
      rm -f $link_name || :

    fi
  fi

  if [[ ${ARCH:=UNDEF} = ibm_power* ]] ; then
    rcp %LOGDIR%%SMSNAME%.%SMSTRYNO% %SMSNODE%:%SMSHOME%%SMSNAME%.%SMSTRYNO%
  fi

}
typeset -fx SMSRCP

#  end of rcp.h
