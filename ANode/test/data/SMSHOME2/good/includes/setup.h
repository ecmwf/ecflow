banner setup.h
#=======================================================================
## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.

USER=${USER:=$LOGNAME}

set -a

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
#=======================================================================
#
#  Distinguish between OD and RD
#
#=======================================================================

STREAM=%STREAM:DA%
if [[ $STREAM = DA ]] ; then
  stream=""
else
  stream="/${STREAM}"
fi

  # OD_MODE is set to false (default) for rd, e4 projects
  #                   true for od and all od projects
  OD_MODE=true
  CLASS=%OD_PROJ:od%
  OD_PROJ=%OD_PROJ:od%

  #---------------------------------------------------------------------
  #  EMOS specific setup
  #---------------------------------------------------------------------

  EMOS_YEAR=`substring $BASETIME 1 4`
  EMOS_MONTH=`substring $BASETIME 5 6`
  EMOS_DAY=`substring $BASETIME 7 8`
  EMOS_BASE=`substring $BASETIME 9 10`
  EMOS_VERSION=$EXPVER
  EMOS_TIME_STEP_H=%EMOS_TIME_STEP_H%
  EMOS_STREAM=$STREAM

  if [[ $EXPVER -ne 1 && $EXPVER -ne 2 ]] ; then
    alias -x gksplot="gksplot -u $EPLOT_USER"
    alias -x magplot="magplot -u $EPLOT_USER"
  fi

#=======================================================================
#
#  Derive the disk variables
#
#=======================================================================

export FWROOT=$FSROOT/${EXPVER} # this variable for similar path WITHOUT stream
export FWBASE=$FSROOT/${EXPVER} # this variable for similar path WITHOUT stream

if [[ %OD_PROJ:od% != od ]] ; then
  BASE=$TROOT/$CLASS/$EXPVER
  WORK=$WROOT/$CLASS/$EXPVER
  LIBS=$BASE/lib
  BINS=$BASE/bin
  DATA=$BASE/data

elif [[ $STREAM = MAED ]] ; then
  TROOT=${TROOT}/${STREAM}
  WROOT=${WROOT}/${STREAM}
  FSROOT=${FSROOT}/${STREAM}
  BASE=$TROOT/$EXPVER
  WORK=$WROOT/$EXPVER
  LIBS=$BASE/lib
  BINS=$BASE/bin
  DATA=$BASE/data

elif [[ $STREAM = DCDA ]] ; then
  # TBD TBD TBD ODIR configuration for operational mode

  BASE=$TROOT/$EXPVER
  LIBS=$BASE/lib
  BINS=$BASE/bin
  DATA=$BASE/data
  TROOT=${TROOT}/${STREAM}
  BASE=$TROOT/$EXPVER
  WROOT=${WROOT}/${STREAM}
  WORK=$WROOT/$EXPVER
  FSROOT=${FSROOT}/${STREAM}
  FSODB=${FSODB}/${STREAM}

else
  BASE=$TROOT/$EXPVER
  WORK=$WROOT/$EXPVER
  LIBS=$BASE/lib
  BINS=$BASE/bin
  DATA=$BASE/data
fi


if [[ $OD_MODE = true ]] ; then
  LOGS=%LOGDIR%/$SUITE/$FAMILY
else
  LOGS=$TROOT/log/$SUITE/$FAMILY
fi

if [ "$FSFAMILY" != "" ] ; then
  PATH=${XBINS}/${ARCH}/${IFS_CYCLE}/${FSFAMILY}:${XBINS}/${ARCH}/${IFS_CYCLE}:${XBINS}/${ARCH}/${IFS_CYCLE}/an:/${BINS}/${FSFAMILY}:${HOME}/bin/${FSFAMILY}:${XBINS}/${IFS_CYCLE}:${XBINS}/an:$PATH:$emosbin:${HOME}/bin/${ARCH}
  FSFAMILY=/$FSFAMILY
fi

SUBFSFAMILY=%SUBFSFAMILY:""%

XDIR=$XROOT/bin
ADIR=$BASE/${BASETIME}${FSFAMILY}$SUBFSFAMILY
WDIR=$WORK/${BASETIME}${FSFAMILY}$SUBFSFAMILY
ODIR=$OROOT/0001/${BASETIME}


if [[ "$STREAM" = MAED ]] ; then
 if [[ "$FSFAMILY" = "/fc" ]] ; then
   if [[ $INIMACONS -eq 1 ]] ; then
    ADIR="${ADIR}_cons"
    WDIR="${WDIR}_cons"
   elif [[ "$INIMA" != "" ]] ; then
     ADIR="${ADIR}_$INIMA"
     WDIR="${WDIR}_$INIMA"
   fi
 fi
fi

PATH=$HOME/bin/ppdi:$TMPDIR:$BINS:$BINS/$WAMRESOL:$XROOT/bin/$EMOS_CYCLE:$PATH

if [[ "$FSFAMILY" = /mc ]] && [[ $RUNHINDCAST != 1 ]] ; then
  EMOS_STREAM=EF
  STREAM=EF
fi

#=======================================================================
#
#  Suite specific fdb settings
#
#=======================================================================
#=======================================================================
#
#  Derive the FileServer variables
#
#=======================================================================

if [[ %OD_PROJ:od% = od ]] ; then
  FSBASE=${FSROOT}${FSFAMILY}/${EXPVER}
else
  FSBASE=${FSROOT}${FSFAMILY}/${CLASS}/${EXPVER}
fi

FDBASE=$FSBASE

FSHOST=${FSBASE}/${ARCH}

FSLIBS=${FSHOST}/lib
FSBINS=${FSHOST}/bin
FSDATA=${FSHOST}/data
FSODIR=${FSROOT}/0001/$(substring $BASETIME 1 6)/$(substring $BASETIME 7 10)

DHSWAVE=${FSROOT}/${EXPVER}/wave

if [[ "$FSFAMILY" = /euroshelf ]] ; then
 DHSWAVE=${FSROOT}/${EXPVER}/euroshelf
fi

if [[ $OD_MODE = true ]] ; then
  FSDIR=${FSBASE}/${BASETIME}
  FSLOG=${FSBASE}/log

  DHSTIME=$(substring $BASETIME 1 6)/$(substring $BASETIME 7 10)

  FSBDIR=$FSOROOT/$EXPVER/$DHSTIME
else
  FSDIR=$FSROOT/${EXPVER}/${BASETIME}
  FSLOG=$FSROOT/${EXPVER}/log

  DHSTIME=${BASETIME}
fi
FODBDIR=${FSODB}${FSFAMILY}/${EXPVER}${stream}/${BASETIME}

PSDIR=/ws/scratch/ma/emos/ps

#=======================================================================
# Set variable to run metview macros on a different fs from normal users
# Please see comments in web.h if you want to change it.
#
# On linux cluster we use the default one
#=======================================================================



#=======================================================================
#  Model variables that depend on configuration
#=======================================================================

VPP_MBX_SIZE=1000000
MBX_SIZE=1000000
MPP_TYPE=2
if [[ $ARCH = @(ibm_power*) ]] ; then
  MBX_SIZE=64000000
fi

[[ $RESOL = 213 || $RESOL = 319 ]] && NRESOL=160
[[ $RESOL = 106 || $RESOL = 159 ]] && NRESOL=80
[[ $RESOL = 511 ]] && NRESOL=256
[[ $RESOL = 799 ]] && NRESOL=400
[[ $RESOL = 255 ]] && NRESOL=160
[[ $RESOL = 399 ]] && NRESOL=200


if [[ $ARCH = @(ibm_power*) ]] ; then
  CC=xlc_r
  FRT=xlf90
  FRT77=xlf
  LD=mpxlf90_r
  LDCC=xlc_r
  BUILDWS=true
  WSUSE=xlf_7112
  PARALLEL=4
  NATIVE=true
  F90_DEBUG=0
  RUN_PARALLEL=true
fi
#==========================================================
# New for 33r1
#==========================================================

if [[ $ARCH = ibm_power4 ]] ; then
  USE=xlf_11102
elif [[ $ARCH = @(ibm_power6) ]] ; then
  USE=standard
fi

if [[ $TASK != exp_setup ]] ; then
  USE=${USE:-standard}

  if [[ $USE != standard ]] && [[ $ARCH = ibm_power* ]] ; then
    use $USE
#    addon1=`echo $PATH | cut -d: -f1`
#    addon2=`echo $PATH | cut -d: -f2`
#    REDUCED_PATH=$addon1:$addon2:$REDUCED_PATH
  fi
fi

set +a

#=======================================================================
#
#  Functions to manipulate the variables
#
#=======================================================================

NEWADIR() {
  set -xe
  if [ $# -lt 1 ] ; then echo $0: error; return 1 ; fi
    echo $BASE/$(newdate $BASETIME $1)/$(basename $ADIR)
}

NEWWDIR() {
  set -xe
  if [ $# -lt 1 ] ; then echo $0: error; return 1 ; fi
    # echo $WORK/$(newdate $BASETIME $1)/$(basename $WDIR) # 29r2 TBD
    echo `echo $WDIR | sed -e "s/$BASETIME/$(newdate $BASETIME $1)/g"`
}

NEWFSBDIR() {
  set -x
    if [ $# -lt 1 ] ; then echo $0: error; return 1 ; fi;
    XX=`newdate ${BASETIME} $1`;
    # used to define the location for PS_BIAS file
    # shall contain DCDA stream within the path _but_ this would mean
    # modifying FSBDIR variable. However, too many script use already
    # this variable.
      echo $FSOROOT/$EXPVER/$(substring $XX 1 6)/$(substring $XX 7 10);
# ${FSBASE}/$STREAM/$(substring $XX 1 6)/$(substring $XX 7 10)
}

NEWFDIR() {
  set -xe
    if [ $# -lt 1 ] ; then echo $0: error; return 1 ; fi;
    XX=`newdate ${BASETIME} $1`;
    echo $FSBASE/$(substring $XX 1 6)/$(substring $XX 7 10)
}

CHECK_DIR(){
  if [[ $ARCH = ibm_power* ]]  ; then
    trap '{ echo "Error in function"; exit 1; }' 0 $SMS_SIGNAL_LIST
  fi
  set -xeu
  if [[ ! "$ARCH" = "linux" ]] ; then
    trap '{ echo "Error in function"; exit 1; }' 0 $SMS_SIGNAL_LIST
  fi
  if [[ $# -eq 0 ]]; then
    return 1
  fi

 if [[ $ARCH = ibm_power* ]] then
   (umask 022;pmkdir $1 || exit )
 else
    mkdir -p -m 755 $1 || exit
 fi
  if [[ ! "$ARCH" = "linux" ]] ; then
    trap 0
  fi

  return 0
}


PUT() {
  set -xe
  Ecp -o $1 ec:$FSDIR/$1 || return 1
}

GET() {
  set -xe
  Ecp ec:$FSDIR/$1 $1 || return 1
}

if [[ $OD_MODE = true ]] ; then

  GETSV() {
    set -xe
    file=$(basename $2)
    Ecp ec:$1/$file $2 || return 1
  }

else
  GETSV() {
       set -xe
       Ecp ec:$1/$2 $2 || return 1
  }

fi

typeset -fx NEWADIR NEWWDIR NEWFDIR CHECK_DIR PUT GET GETSV NEWFSBDIR

if [[ $ARCH != fujitsu ]] ; then
  function libselect { echo WARNING - dummy command : libselect $1 ;}
  typeset -fx libselect
fi

export OLDADIR=`NEWADIR -$PERIOD`
export OLDWDIR=`NEWWDIR -$PERIOD`
export YESTERDIR=`NEWWDIR -24`

#==========================================================================
#  Copy postscript file into a scratch directory;
#
#  PSFILE [filename [countem [rename]]]
#
#  Default filename is "ps"
#
#  countem (set but can contain anything) means that there is more than
#  one file from this task in which case the files are numbered.
#
#  If only one file is produced it is copied as
#    ${_PSDIR}/${SUITE}${EMOS_BASE}${TASK}.ps
#  otherwise it is copied as
#    ${_PSDIR}/${SUITE}${EMOS_BASE}${TASK}.fileno.ps
#  where fileno is counted automatically
#
#  If the third parameter is given the naming convention is not followed
#  except that the file numbering is still being done (stupid if there's
#  only one file, but what the hell...)
#
#  Old file is not saved
#==========================================================================

PSFILE() {
  set +exv
  if [ $# -ge 1 ] ; then _psfile="$1" ; else _psfile="ps" ; fi
  if [ ! -f $_psfile ] ; then return 1; fi
  if [ $# -ge 2 ] ; then _fileno=`expr ${_fileno:-0} + 1`; else _fileno=0; fi
  if [ $# -ge 3 ] ; then _name=$3; else _name=${SUITE}${EMOS_BASE}${TASK}; fi

  if [ ${_fileno} -gt 0 ] ; then
    /bin/rm -f ${PSDIR}/${_name}.${_fileno}.ps
    cp $_psfile ${PSDIR}/${_name}.${_fileno}.ps
  else
    /bin/rm -f ${PSDIR}/${_name}.ps
    cp $_psfile ${PSDIR}/${_name}.ps
  fi

  set -ex
}

typeset -fx PSFILE

#=======================================================================
#  For FDB we need a different stream. Oh why can they not be the same?
#=======================================================================

set -a
[[ $STREAM = DA   ]] && { WSTREAM=WV;   FDB_STREAM=oper; FDB_WSTREAM=wave; }
[[ $STREAM = EF   ]] && { WSTREAM=WAEF; FDB_STREAM=enfo; FDB_WSTREAM=waef; }
[[ $STREAM = SCDA ]] && { WSTREAM=SCWV; FDB_STREAM=scda; FDB_WSTREAM=scwv; }
[[ $STREAM = MAED ]] && { WSTREAM=MAWV; FDB_STREAM=maed; FDB_WSTREAM=mawv; }
[[ $STREAM = sens ]] && { WSTREAM=sens; FDB_STREAM=sens; FDB_WSTREAM=sens; }
[[ $STREAM = SCDA ]] && unset FDB_SERVER_HOST || true
[[ $STREAM = DCDA ]] && { WSTREAM=DCWV ; FDB_STREAM=dcda ;  FDB_WSTREAM=dcwv; }
[[ $STREAM = EFHC ]] && { WSTREAM=EWHC ; FDB_STREAM=efhc ;  FDB_WSTREAM=ewhc; } # MC HINDCAST
[[ $STREAM = ENFH ]] && { WSTREAM=ENWH; FDB_STREAM=enfh ;  FDB_WSTREAM=enwh; } # MC HINDCAST
[[ $STREAM = DAHC ]] && { WSTREAM=WVHC ; FDB_STREAM=dahc ;  FDB_WSTREAM=wvhc; } # MC HINDCAST

set +a

#=======================================================================
#  Check for day and date mask
#=======================================================================

echo checking for date mask

datemask=$(echo '%DATEMASK%' | cut -f1 -d.)
day=$(substring $BASETIME 7 8)
weekday=%WEEKDAY:none%

if [[ $weekday != "none" ]] ; then
  jday=$(newdate -d $BASETIME)
  jday=$(expr $jday %% 7) || true
  if [[ $jday -eq 0 ]] ; then
    dow=monday
  elif [[ $jday -eq 1 ]] ; then
    dow=tuesday
  elif [[ $jday -eq 2 ]] ; then
    dow=wednesday
  elif [[ $jday -eq 3 ]] ; then
    dow=thursday
  elif [[ $jday -eq 4 ]] ; then
    dow=friday
  elif [[ $jday -eq 5 ]] ; then
    dow=saturday
  elif [[ $jday -eq 6 ]] ; then
    dow=sunday
  fi
fi

if [[ $datemask != "*" && $day -ne $datemask ]] || [[ $weekday != "none" && $weekday != $dow ]] ; then
echo job should not run today ... setting it complete
%include <endt.h>
fi

#=======================================================================
#
#  Check the directories
#
#=======================================================================

CHECK_DIR $ADIR
CHECK_DIR $WDIR

if [[ $ARCH = ibm_power* ]];then
  export PBIO_BUFSIZE=1048576
fi

#=======================================================================
#
#  Suite stopper
#
#=======================================================================

if [[ %SUITE% = "e_35r3" && $BASETIME = "2012120500" ]] ; then
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR"
  echo "Suite e_35r3 has been set up to stop now"
  echo "Operators  You can suspend this suite and requeue failed tasks if you like!"
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR"
  exit 1
elif [[ %SUITE% = "emc_35r3" && $BASETIME = "2012022600" ]] ; then
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR"
  echo "Suite emc_35r3 has been set up to stop now"
  echo "Operators  You can suspend this suite and requeue failed tasks if you like!"
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR"
  exit 1
fi

#=======================================================================
# We use an operation only Magics version to be on the safe side.
# This is of course only on the servers
#=======================================================================
MAGICS=%MAGICS:magics_emos%

#=======================================================================
# We use an operation only Metview version to be on the safe side.
#=======================================================================

if [[ $ARCH = linux ]] ; then
  metview_cmd=%METVIEW:metview_emos%

elif [[ $ARCH = ibm_power4 ]] ; then
  METVIEW_TMPDIR=/home/ma/emos/metview/metview_tmpdir
  metview_cmd=%METVIEW:/home/ma/emos/metview/MvRun_3.9.3%
elif [[ $ARCH = ibm_power6 ]] ; then
  METVIEW_TMPDIR=/home/ma/emos/metview/metview_tmpdir
  metview_cmd=%METVIEW:/home/ma/emos/metview/MvRun_3.10%
fi

#=======================================================================
# We use an operation only metpy version to be on the safe side.
#=======================================================================
metpy_cmd=%METPY:metpy_emos%

echo setup complete

# 20040720 add /home/rd/rdx/bin/filter/bufr_filter from hpcd
#
%include <law.h>

export LEDFAMILY=%EDFAMILY:false%

banner end setup.h
