#  rcp.h

ECF_RCP() {
  set -x
  set +e
  if [[ "${ARCH:=UNDEF}" = hppa || "${ARCH:=UNDEF}" = hpia64 ]] ; then
    if [[ $PBS_JOBID = NOT_SET ]] ; then
      rcp %LOGDIR%%ECF_NAME%.%ECF_TRYNO% \
          emos@%ECF_NODE%:%ECF_OUT%%ECF_NAME%.%ECF_TRYNO%
    else
      rm -f %ECF_JOBOUT% || : # remove link

      # hardcoded to hanfs2 then mordred to release load on ablamor
      rcp /var/spool/PBS/spool/${PBS_JOBID}.OU \
        emos@hanfs2:/hanfs%ECF_HOME%%ECF_NAME%.%ECF_TRYNO% || \
      rcp /var/spool/PBS/spool/${PBS_JOBID}.OU \
        emos@mordred:%ECF_HOME%%ECF_NAME%.%ECF_TRYNO% || \
      rcp /var/spool/PBS/spool/${PBS_JOBID}.OU \
        emos@%ECF_NODE%:%ECF_HOME%%ECF_NAME%.%ECF_TRYNO%

      link_name=`~emos/bin/subs_path.pl -f %ECF_OUT% -t $TOPATH -n %ECF_JOBOUT%`
      rm -f $link_name || :

    fi
  fi

  if [[ ${ARCH:=UNDEF} = ibm_power* ]] ; then
    rcp %LOGDIR%%ECF_NAME%.%ECF_TRYNO% %ECF_NODE%:%ECF_HOME%%ECF_NAME%.%ECF_TRYNO%
  fi

}
typeset -fx ECF_RCP

#  end of rcp.h
