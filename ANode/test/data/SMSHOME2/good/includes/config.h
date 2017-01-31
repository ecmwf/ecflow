## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.

#=======================================================================
#  config.h
#=======================================================================
banner config.h

if [[ "$ARCH" != "linux" ]] ;then
  typeset -l ARCH
fi

export SCRATCHDIR=${SCRATCHDIR:-UNDEFINED}
export ARCH=${ARCH:-sgimips}
export TMPDIR=${TMPDIR:-UNDEFINED}
export HOST=${HOST:-`hostname`}
export NPES=${NPES:-0}

if [[ $ARCH = linux ]] ; then
  export EMOS_TMPDIR=/var/tmp/tmpdir/%TASK%_%ECF_TRYNO%.$$
  export TMPDIR=$EMOS_TMPDIR
  [[ -d $TMPDIR ]] || mkdir -p $TMPDIR
  export USE_EMOS_TMP="true"
  cd $TMPDIR
else
  EMOS_TMPDIR=""
fi

if [[ "$TMPDIR" = "UNDEFINED" ]] ; then
   TMPDIR="$TEMP/NQS_tmpdir.$$"  export TMPDIR
   export USE_EMOS_TMP="true"
   [[ -d $TMPDIR ]] || mkdir $TMPDIR
   if [[ $? -ne 0 ]] ; then
      echo "####### CANNOT CREATE TMPDIR #######" >&2
      exit 1
   fi
fi

PS4="+ "

echo "\
  This is the config.h file and\n\
  System variables used were:\n\
  USER     $USER\n\
  TMPDIR   $TMPDIR\n\
  HOME     $HOME\n\
  HOST     $HOST\n\
  ARCH     $ARCH\n\
  PATH     $PATH\n\
"
set -xa

#=======================================================================
#  Experiment version and initial data origin (default values)
#=======================================================================

EXPVER=%VERSION:0001%
GRIBCLASS=1
BASEVER=0001
BASETIME=1996072200

#=======================================================================
#  Software configuration (library cycles etc)
#=======================================================================

IFSMASTER=ifsMASTER
#####################################
IFS_CYCLE=35r3
#####################################


SUITE_TYPE=%SUITE_TYPE:"undef"%
NEWSTREAM=none
EMOS_CYCLE=000370
VIEW_IFS=
VIEW_IFSAUX=
VIEW_OBSPROC=
VIEW_OBSORT=
VIEW_TOVSCODE=
VIEW_SSMICODE=
VIEW_BL=
VIEW_SSA=
VIEW_SST=
VIEW_WAM=
MAKE_LIBS=yes

GRIB_API_VERSION=1.8.0
GRIB_API_INCLUDE="-I/usr/local/lib/metaps/lib/grib_api/jasper/include -I/usr/local/lib/metaps/lib/grib_api/$GRIB_API_VERSION/include"
GRIB_API_LIB="-L/usr/local/lib/metaps/lib/grib_api/jasper/lib -ljasper -L/usr/local/lib/metaps/lib/grib_api/$GRIB_API_VERSION/lib -lgrib_api_f90 -lgrib_api"
export GRIB_API=/usr/local/lib/metaps/lib/grib_api/$GRIB_API_VERSION/bin   # change to fixed version
export PATH=$GRIB_API:$PATH

#=======================================================================
#  Model configuration
#=======================================================================

RESOL=799
LEVELS=91
GTYPE=l_2
FCLENGTH=%FCLENGTH:6%
FCLENGTH0N=$FCLENGTH
RUN_GFC=no

#=======================================================================
#  Analysis parameters
#=======================================================================

IFSMODE=early_delivery
PERIOD=6
TSTEP=720

if [[ "%STREAM%" = SCDA ]] && [[ %MXUP_TRAJ:3% = 1 ]] ; then
  RESOLINC_0=255 # HR:255, LR:159
  TSTEP_INC_0=1800
else
  RESOLINC_0=95
  TSTEP_INC_0=3600
fi

RESOLINC_1=159     # 255 32r1
RESOLINC_2=255     # 159 32r1
TSTEP_INC_1=1800
TSTEP_INC_2=1800

LFG=.false.

ITER_MIN_0=70
ITER_MIN_1=50
ITER_MIN_2=50
MXUP_TRAJ=%MXUP_TRAJ:3%
NMINSIMPHY=1
LINCNMI=false
LVERIFY_SCREEN=true
SCRANA=active

RESOLFCE=42
ITER_FCE=150
NWRIEVEC=25

OBPATH=ops
BLACK_DS=ops
BLACK_MM=ops
BLACK_EX=ops

LOZONE=true
LOZONECH=true
ANEMOM_HT=true
LISENTRPP=true
LESUITE=false

OBNLAT=90.00
OBSLAT=-90.00
OBWLON=-180.00
OBELON=180.00

MPLARGS="check=0"

#================================
# New for 25r3

LINRADEX=true
LREDUCOB=false
TSLOTSEC=1800
UPTRAJ_FCE=0
LAMV_REASSIGN=false

#================================
# New for 25r4

BRANCH=""
BRF=0
LAIRS=true
PARALLEL=4
PROJECTS="none"

#================================
# New for 26r3

if [[ "%SUITE%" = o || "%SUITE%" = e_* ]] ; then
  LARCHINCR=true
else
  LARCHINCR=false
fi

#================================
# New for 28r1

FPOSCLD=%FPOSCLD:false%  # should be true for 4dvar, false for 3d_fgat
PROFILE=%PROFILE:0%  # for diagnostic purposes
LANOBSENS=false

#================================
# New for 28r2
#==========================================================
# Variables configuring testOfAnalysis
#==========================================================
LTESTVAR=false
NTESTVAR=0
LCVTEST=false
LTLTEST=false
LADTEST=false
LGRTEST=false
LTESTINC=false    # should be set to false and will turn off other testing

#==========================================================
# New for 28r3
#==========================================================
REINIHOUR=%REINIHOUR:0%
ED_PERIOD=%ED_PERIOD:12%
ED_ONLY=%ED_ONLY:false%
REINICLASS=%REINICLASS:od%
REINI_NOT_ED=%REINI_NOT_ED:false%
OBSTREAM=%STREAM%

#==========================================================
# New for 29r1
#==========================================================
set -a
LOBREALTIME=%OBREALTIME:false% ; # false in catchup mode, true in rt
LCALC_PSBIAS=%CALC_PSBIAS:true%
LUSE_PSBIAS=%USE_PSBIAS:true%
INIPSBIAS=%INIPSBIAS:true%
INIVARBC=%INIVARBC:false%

NTYPE_MODERR=2
NCOMP_MODERR=1
LFGMODERR=true
ALPHAQ=0.3         # 31r2
NSERR=-1
LINITCV=true
LMODERRFC=false
LBGMODERR=false

LPROPTL=false
LINTEST=false

SAVINI=false

EPSROTPATH=""

#==========================================================
# New for 29r2
#==========================================================
set -a
LTWINTRUTH=false
LTWINEXP=false
TRUTHEXP="none"
LCO2=false
LCO2ANER=false
INICO2=false
LERA40_PREODB=false
Q2CREATE_NEWSTREAM=false
Q2USE=false
Q2CHANGELIST=0

# EPS
NSVET=50
NITERLET=120
EPSBASIS=50 # number of tropical singular vectors used in
            # Gaussian sampling
EPSBASIST=5 # number of tropical singular vectors used in

            # in order to startup emc suite this might be set to 25
            # for the first 2 way. Then make_sv would not be used.

EPSEVO_NORMALIZE=true
ICPEXPVER=%ICPEXPVER:0001%
EPSGAMMA_TC2INI=1.5
EPSGAMMA_EVO2INI=1.0 # 1.5 for 29r2
EPSGAMMA=0.014
EPSSIMPL=5     # Gaussian sampling for all singular vectors
#==========================================================
# New for 29r3
#==========================================================

LGPINNER=true
NFRDHP=3

#==========================================================
# New for 30r1
#==========================================================
FILTERRESOL=255
FILTERFACTOR=5
FILTEREXPONENT=4
EC_FILL_NANS=false
USE_SMT=%USE_SMT:true%

if [[ "%EC_SMT:yes%" = yes ]] ; then
  USE_SMT=true
else
  USE_SMT=false
fi

#==========================================================
# New for 30r2
#==========================================================
LUSE_RSTBIAS=%USE_RSTBIAS:true%
ERA_MODE=false

#==========================================================
# New for 31r1
#==========================================================
DOFDBK=true # ie true
LCALC_RSTBIAS=%CALC_RSTBIAS:false%
VARBC_PATH=%VARBC_PATH:standard%
NCS_CONFIG=2  # 31r2
USE_SMT=%USE_SMT:true%
LPML=%PRODUCE_EPS_ML:0%

if [[ %EC_SMT:yes% = yes ]] && [[ %USE_SMT:true% = false ]] ; then
  echo "ECF_ variable EC_SMT and USE_SMT are not compatible"
  exit 1
elif [[ %EC_SMT:yes% = no ]] && [[ %USE_SMT:true% = true ]] ; then
  echo "ECF_ variable EC_SMT and USE_SMT are not compatible"
  exit 1
fi
#==========================================================
# New for 31r2
#==========================================================
LRAIN4D=true
LNMPHYS=true   #32r1
ENDANENS=0
ENDANENSODB=0

P4CLIENT=0     #32r1 not used in operations

#==========================================================
# New for 32r1
#==========================================================
LCONSTANT_VARBC=false

#==========================================================
# New for 32r2
#==========================================================
LIASI=true

INIMODISALB=true # if intial data was run with 32r1 or later
VARBC_ARCHIVE_EVOLVE=false

#==========================================================
# New for 32r3
#==========================================================
LFULL_IASI=%LFULL_IASI:true%   # true = use full iasi data and screen: false use screened data
LGEMS=false                    # run with gems parameters
GHGVAR=
GRGVAR=
AEROVAR=
TRACVAR=
INIGHG=false
INIGRG=false
INIAERO=false
INITRAC=false
LEVGEN=true          # Use van Genuchten hydrology
INIHTESSEL=true      # if starteding from experiment where LEVGEN=true
LCALC_RSTRHBIAS=%CALC_RSTRHBIAS:true%
LUSE_RSTRHBIAS=%LUSE_RSTRHBIAS:true%
LSSMI1D=true
LSSMIS1D=true
LAMSRE1D=true
LTMI1D=true
LAMSRE1D=true
ODB_IO_OPENMP=1

#==========================================================
# New for 33r2
#==========================================================
LOBSCOR=false

#==========================================================
# New for 35r2
#==========================================================

LECURR=false
INIECURR=false
LSCATT_NEUTRAL=false
D3GGFIELDSSTEND="0"
LFSOBS=false
LSEKF=false

#==========================================================
# New for 35r3
#==========================================================
# if EPS or associated then LEMISKF & LWEAK4DVAR=false
INIOZONE=true

WEAK4D_INTERV=0.0
LMODERR_PERIODIC=false
NSPLIT4DWIN=0
LBALSTRATO=false
AMSU_LAND=Dynamic_emis
EMISKF_PATH=standard
INIEMISKF=true
INIMODERR=true
LDUCTDIA=false
LASCATSM=true
if [[ %SUITE% = "e_*" ]] ; then LANOBSENS=true; fi

#==========================================================
# Variables configuring ifs_ctm
#==========================================================
LCOUPLO4=false
CTM_MODEL=mozart
NFRCOUPLO4=1
NFRCOUPLO4_FB=1
LGLOBALGP=true
LGLOBALSP=true
MPROCGPG=1
MPROCSPG=1
LOUT_SPC=false
LOUT_FLUX=true
LOUT_COOR=false
LIN_GRG=true
LIN_COOR=false
LCOUPLO4_CTM=true
LOUT_GRG=false
LOUT_CO=false
LOUT_SO2=false
LOUT_NOX=false
LOUT_NO2=false
LOUT_GO3=false
LOUT_HCHO=false
LNOX2NO2=false
INTERP2D=bilinear
PARA_SEARCH=global
IF_MASKED=novalue
LGRG_CYCLE=true
LDIFF_GRG=false
LCONV_GRG=false
LDIA_GRG=false
NPROC_CTM=8
NPROC_CTM_AN=8
NPROC_DRV=1
CTM_THREADS=4
CTM_THREADS_AN=4
DRV_THREADS=1
DRV_THREADS_AN=1
IFS_THREADS_AN=6
LSTATS=true

#==========================================================
# Variables configuring Gems
#==========================================================
LGEMS=false
GHGVAR=""
INIGHG=false
GRGVAR=""
INIGRG=false
AEROVAR=""
INIAERO=false
TRACVAR=""
INITRAC=false

#=======================================================================
# DR_HOOK
#=======================================================================
DR_HOOK=true # 30r1 was false
DR_HOOK_OPT="none" # 30r1 was "prof"
DR_HOOK_PROFILE_LIMIT=-10
DR_HOOK_HASHBITS=15
DR_HOOK_CATCH_SIGNALS=false
DR_HOOK_IGNORE_SIGNALS=false

LIBHPM=""
if [[ $DR_HOOK = true ]] ; then
  DR_HOOK_OPT=`echo $DR_HOOK_OPT | sed -e 's/\// /g'`
  if [[ $ARCH = ibm_power4 ]] ; then
    LIBHPM="-L/usr/pmapi/lib -lpmapi"
  fi
fi

#=======================================================================
#  BC project setup for analysis (only those that are different)
#=======================================================================

if [[ "%STREAM%" = SCDA ]] ; then
  PERIOD_4D=6 ;
  WINDOW_LENGTH_4D=%WINDOW_LENGTH_4D:12%
  WINDOW_OFFSET_4D=%WINDOW_OFFSET_4D:9%

  if [[ %MXUP_TRAJ:3% = 3 ]] ; then

  IFSMODE=4d_inc

  else

  IFSMODE=3d_fgat
  ITER_MIN=70
  ITER_MIN_1=0
  SIMUL_MIN=80
  SIMUL_MIN_1=0
  LISENTRPP=false
  LVERIFY_SCREEN=false

  fi
else
  # PERIOD_4D=6 ; # 20040423 should stay 12 by default (for SCDA reinit), then SMS variable is set to 12, this variable is considered for reinit = false (vardata.sms)
if [[ `echo %FAMILY:NOT_DEF% | cut -d / -f 1` = "main" ]] ; then
  ## RD behaviour : default is constant 12,
  ## if SMS variable is changed, task should consider it specifically
  ## some tasks (vardata, fetcherr) may want this default and not SMS
  ## variable for REINITIALIZE state
  PERIOD_4D=12
  WINDOW_LENGTH_4D=12
  WINDOW_OFFSET_4D=3

else  # SMS variable IS the environment variable for other families
  PERIOD_4D=%PERIOD_4D:12%
  WINDOW_LENGTH_4D=%WINDOW_LENGTH_4D:12%
  WINDOW_OFFSET_4D=%WINDOW_OFFSET_4D:3%
fi
fi

#=================================================
#  Simplified Kalman filter parameters
#=================================================

TSTEP_SV=1200
RESOLSV=42
NLANTYPE=6
NJDSTOP=48
NITERL=60
NINNER=30
NEIGEVO=60
HESSTYPE=3d
SVPPFRQ=6


################### New for 23r1 #############
LATE4DSTART=true
FPOSINC=false
INICLOUDAN=true


#=======================================================================
#  Satellite parameters
#=======================================================================

LTOVS=.false.
LATOVS=true
LAIRS=true # 31r1
LSSMI=true # 33r1
LSCAT=.true.
LGEOS=true
LRAIN1D=false    # 35r2
LSSMIRAIN=true   # 33r2
LSSMISRAIN=true  # 35r3
LTMIRAIN=true    # 35r3
LAMSRERAIN=true  # 33r2
LPRERAD1C=false
LREO3=true
LAEOLUS=false # 31r1
LSSMIS=true   # 31r2
LAMSRE=true   # 31r2
LMERIS=true # 20080714 until which was false  # 33r2
LTMI=true     # 31r2
LGPSRO=true   # 31r2

BIAS_PATH_AMV=none
BIAS_PATH_REO3=none
LREO3_BCOR=false

BIASCOLD=yes
BIASDAYS=14
BIASMISS=7
BIAS_ARCHIVE=1

if [[ "%SUITE%" = o || "%SUITE%" = e_* ]] ; then
  export FDB_NOF_BUFF=4
  export FDB_BUF_SECT_SIZE=25165824
elif [[ "%SUITE%" = bc || "%SUITE%" = ebc_* ]] ; then
  export FDB_NOF_BUFF=3
  export FDB_BUF_SECT_SIZE=33554432
else
  export FDB_NOF_BUFF=4
  export FDB_BUF_SECT_SIZE=4194304
fi

# 23r4
RSTBIAS=true

# 24r2
ERAFS=false

#  SATMON parameters
SMON_OPT_DIR=""
SMON_OPT_GEOS=ops
SMON_OPT_NOAA=ops
SMON_OPT_DMSP=ops
SMON_OPT_O3=ops
SMON_OPT_O3KNMI=ops
SMON_OPT_O3MSG=ops
SMON_OPT_TEMP=ops
SMON_OPT_PWC=ops
SMON_OPT_AMV=ops
SMON_OPT_DBAMV=ops       # 33r1
SMON_OPT_AIRS=ops
SMON_OPT_IASI=ops        # 32r3
SMON_OPT_EARS=ops
SMON_OPT_PACRARS=ops     # 33r1
SMON_OPT_MERIS=ops       # 33r2
SMON_OPT_SSMIS=ops       # 31r2
SMON_OPT_AMSRE=ops       # 31r2
SMON_OPT_TMI=ops         # 31r2
SMON_OPT_GPSRO=ops       # 31r2
SMON_OPT_SCATT=ops       # 35r3
SMON_OPT_SLMOIST=ops     # 35r3


SATROOT=/od_archive/data/satmon

#==========================================================
# Variables configuring Reanalysis
#==========================================================
MEANS=true
MOMENTS=true
INTEGRALS=true
INCREMENTS=true
PLOTS=true
RSTBIAS_TS=false
RSTBIAS_SE=true
ERAPRODUCTS="/era/intprod/data/monitor"

#=======================================================================
#  Coupled wave model parameters
#=======================================================================

WAVE=yes
WAM2WAY=yes
WAMNSTPW=1
  if [[ "%FSFAMILY%" = @(mc|refc|assim) ]] ; then
WAMRESOL=global100
else
WAMRESOL=global36 # HR global50
fi
WAMALT=yes
WAMSAR=yes
WAMSARASS=true

if [[ "%FSFAMILY%" = "euroshelf" || $SUITE_TYPE = "law" ]] ; then  # law suites
  WAMNFRE=30  # 36 future 0044
  WAMNANG=24  # 36 future 0044
else
  WAMNFRE=30
  WAMNANG=24
fi

date_ers1_wave=1994010100

if [[ $BASETIME -lt $date_ers1_wave || $WAMALT != yes ]] ; then
  WAMANPARAM_DYN="215/216/220/221/222/223/224/225/226/227/228/229/230/231/232/233/234/235/236/237/238/239/244/245/249"
else
  WAMANPARAM_DYN="215/216/217/218/220/221/222/223/224/225/226/227/228/229/230/231/232/233/234/235/236/237/238/239/244/245/246/247/248/249/252/253/254"
fi

WAMANPARAM_STAT="219"
WAMANPARAM=$WAMANPARAM_STAT/$WAMANPARAM_DYN

WAMFCPARAM_STAT="219"
WAMFCPARAM_DYN="215/216/217/218/220/221/222/223/224/225/226/227/228/229/230/231/232/233/234/235/236/237/238/239/244/245/249/252/253/254"
WAMFCPARAM=$WAMFCPARAM_STAT/$WAMFCPARAM_DYN

INIWAVE=yes
WAMCOLDLENGTH=240
WAMSTREAM=0
# Many of these values are overridden for
# local area model in law.h

if [[ "%STREAM%" = SCDA ]] ; then
  WAMSTREAM=1027
elif  [[ "%STREAM%" = DCDA ]] ; then
  WAMSTREAM=1029
fi

#=======================================================================
#  Initial data parameters
#=======================================================================


INITIME=%INITIME:2001042900%
INICLASS=%INICLASS:od%
INISTREAM=%INISTREAM:da%
# da : early delivery suite initialised from standard suite
# dcda : early delivery suite initialised from another delayed cutoff suite
INITYPE=4v
INIEXPVER=%INIEXPVER:0001%
INILEVELS=%INILEVELS:60%
INISTEP=%INISTEP:0%
# size of the 4dvar window

# offset from the beginning of the 4V for FC
INIOFFSET_4D=%INIOFFSET_4D:3%
# 9 : early delivery suite initialised from standard suite
# 3 : early delivery suite initialised from another ealry delivery suite
INITILES=true
INIRESOL=799 # was 511
INICI=true
FORCE_FP=no

if [[ "%FAMILY1:NOT_DEF%" = sv || "%FAMILY1:NOT_DEF%" = m2_12 || "%FAMILY1:NOT_DEF%" = m2_00 || "%FAMILY1:NOT_DEF%" = m1_12 || "%FAMILY1:NOT_DEF%" = m1_00 ]] ; then
  INITYPE=fc
#  ------- vv
#  INISTEP=12   # for 00,12 UTC forecasts
#  ------- ^^
  INISTEP=6     # for 06,18 UTC forecasts
  INISTREAM=dcda
  INISUFFIX="_sv"
else
  INISUFFIX=
fi

if [[ "$ARCH" != "linux" ]];then
  typeset -l REINITIALIZE
fi
REINITIALIZE=%REINITIALIZE:false%
REINIEXPVER=%REINIEXPVER:0001%
REINISTREAM=%REINISTREAM:da%
REINIPERIOD_4D=%REINIPERIOD_4D:12%
REINIOFFSET=%REINIOFFSET:12%
ED_CUTOFF=%ED_CUTOFF:4%

#=================================================
# PE settings
#=================================================
NPES_MKCMA=%MKCMANPES:1%
NPES_AN=%ANNPES:1%
NPES_FC=%FCNPES:1%
NPES_SV=%FCNPES:1%
#=================================================
#  PREPAN parameters
#=================================================

OWNER=emos
AMASTER=/ws/home/ma/emos/amaster/xxxx
RUN_PARALLEL=true

#=======================================================================
#  Sami's variables + ODB
#=======================================================================

CONCAT=1
PMETHOD=2
FSODB=/emos_backup

ODB_CTX_DEBUG=0
ODB_MAXHANDLE=5
ODB_STATIC_LINKING=1
ODB_UNDEF=2146959359
ODB_CCMA_CREATE_DIRECT=1
ODBSAVE_ODBCMP=true     #31r2
ODB_FROM_FB=false
if [[ "%STREAM%" = SCDA ]] ; then
  ODBSAVE_CCMA=false
  ODBSAVE_ECMA=false
else
  ODBSAVE_CCMA=true
  ODBSAVE_ECMA=true
fi
ODB_CATCH_SIGNALS=0
ODB_CCMA_IGNORE_TSLOTS=0
ODB_CCMA_OBS_RANDOMIZE=0
ODB_CCMA_WTMETHOD=107
ODB_ERRTRA=1
ODB_FLPCHECK=0
ODB_INTCHECK=0
ODB_IO_METHOD=4
ODB_IO_FILESIZE=128
ODB_PACKING=-1
ODB_REPRODUCIBLE_SEQNO=4
ODB_TEST_INDEX_RANGE=0
ODB_WRITE_EMPTY_FILES=0

MANPATH=""
#=======================================================================
#  Coupled wave variables
#=======================================================================

WGRIBIN=yes

#================================
# New for 19r1, should be put in correct place.

HESS3D=3d
PPFRQ=12
LINCNMI=false
NEIGEVO=60
#================================
# New for 24r4

LMAPSOP=false
MPP_TYPE=1
if [[ "%STREAM%" = SCDA ]] && [[ %MXUP_TRAJ:3% = 1 ]] ; then
  LTRAJHR=false
else
  LTRAJHR=true
fi

#================================
# New for 26r3

LMODERR=false

#=======================================================================
#  Plot control
#=======================================================================

PLOT_RMS=true

#=======================================================================
#  Standard libraries
#=======================================================================

SCHOST=%SCHOST%
SCHOST_BKUP=%SCHOST_BKUP:not_set%
# storage host
STHOST=%STHOST:%
STHOST_BKUP=%STHOST_BKUP%
WSHOST=%WSHOST%

LIBRESOL=.R64.D64.I32
ELIB=/usr/local/lib/metaps/lib/${EMOS_CYCLE}

if [[ $ARCH = ibm_power* ]] ; then
  SLIB=/usr/local/lib
###########################################
#temporary setting: A.Hofstadler, 200200821
  EMOSLIB="-lemos.R64.D64.I32"
  ECLIB="-L $SLIB "
  FDBLIB="-L $SLIB -lfdb"
#temporary setting:
###########################################
  EMOSLIB="-L $ELIB $EMOSLIB"
  OBJECT_MODE=64
  XLFRTEOPTS=err_recovery=no
else
###########################################
#temporary setting: A.Hofstadler, 200200821
#  EMOSLIB="-lemos.R64.D64.I32"
  EMOSLIB="-lemos.R32.D64.I32" # or single???
#temporary setting:
###########################################
  SLIB=/usr/local/lib
  EMOSLIB="-L $ELIB $EMOSLIB"
fi

emosbin=/home/ma/emos/bin/${EMOS_CYCLE}/$ARCH
#=======================================================================
#  Singular vector configuration
#  NSVHEM=1 for SVs over NH only
#         2      "       SH only
#         3      "       NH+SH
#=======================================================================

EPSSVPATH=
EPSSVROTPATH=
EPSFCRES=%EPSFCRES:399% # HR 255

# VAREPS 30r2
EPSFCRES_A=%EPSFCRES_A:399% # HR 255
EPSFCRES_B=%EPSFCRES_B:255% # HR 255
EPSFCRES_C=%EPSFCRES_C:255% # HR 255

EPSFCLENGTH_A=%EPSFCLENGTH_A:240%
EPSFCLENGTH_B=%EPSFCLENGTH_B:144%
EPSFCLENGTH_C=%EPSFCLENGTH_C:408%

EPSINISTEP_B=%EPSINISTEP_B:216%
EPSINISTEP_C=%EPSINISTEP_C:360%

EPSFCGTYPE=%EPSFCGTYPE:l_2%
EPSFCGTYPE_A=%EPSFCGTYPE_A:l_2%
EPSFCGTYPE_B=%EPSFCGTYPE_B:l_2%
EPSFCGTYPE_C=%EPSFCGTYPE_C:l_2%

EPSVARHDIF=false
EPSVARHDIFT=24

EPS_USE_ICP=0
# /VAREPS

EPSFCLEV=%EPSFCLEV:62%  # HR 40
EPSSVRES=%EPSSVRES:42%
EPSSVGTYPE=_full
EPSSVLEV=%EPSSVLEV:62%  # HR 40
EPSMEMBERS=`echo "%ENSEMBLES:50% + 1" | bc`
EPSNENS=%ENSEMBLES:50%
EPSEVO=1
EPSHEM=%EPSHEM:3%
EPSEVOTIME=48

EPSWAMRESOL=%EPSWAMRESOL:global100%
EPSWAMNSTPW=%EPSWAMSTPW:1%
EPSTSTEP=%TSTEP:1800% # HR 2700
EPSSVTSTEP=%EPSSVTSTEP:900%

if [[ %STREAM% = MAED ]] ; then
  INIMODISALB=false
TSTEP=$EPSTSTEP
fi

EPSTSTEP_SV=900 # HR 1200
EPSWAMALT=no
EPSTYPE=%EPSTYPE%
EPSMEMBER=%MEMBER%
FCTOTAL=$EPSMEMBERS
NTOTAL=$EPSNENS
EPSSVDIAB=%EPSSVDIAB:false%
EPSSVNUM=%EPSSVNUM:0%

# 28r3
DELTAHH_TC_TRACKS=12 # 12h lag from previous TC used in targets task
EPSSV_TOPT=48        # optimization time for the tropical singular vectors
                     # targets

TC_TRACKS_EXPVER=%VERSION:0001%    # ???TDB

if [[ %VERSION:0001% = 0020 ]] ; then
  TC_TRACKS_EXPVER=0001
fi

OD_PROJ=%OD_PROJ:od%

if [[ "%SUITE%" = emc* ]] && [[ %IS_REAL_TIME:false% = false ]] ; then
  TC_TRACKS_PATH=/emos/tc/0001
elif [[ $REINITIALIZE = true ]] ; then
  TC_TRACKS_PATH=/emos/tc/$REINIEXPVER
elif [[ %FIRST_DAY% = 1 ]] ; then
  TC_TRACKS_PATH=/emos/tc/$INIEXPVER
  TC_TRACKS_EXPVER=0001
elif [[ ! "$OD_PROJ" = "od" ]] ; then
# || [[ %IS_REAL_TIME:false% = false ]] ; then
  ## MC_NO and other MS suites
  ## and catchup mode running 12 cycle only
  TC_TRACKS_PATH=/emos/tc/0001
else
  TC_TRACKS_PATH=/emos/tc/%VERSION%
fi

TC_TRACKS_FROM_MARS=true
TC_TRACKS_CLASS=od

EPSSVTCSUB=%EPSSVTCSUB:0%
EPSTCBB=%EPSTCBB:1%       # use TC tracks to define optimization regions
EPSSVOP=%EPSSVOP:0%       # not required to set sms variable in OD
EPSSVTC_ORTHONORM=1       # ortho-normalize tropical SVs

RUNHINDCAST=%RUNHINDCAST:0% # 1 for back loop and 0 for realtime

#=======================================================================
# EFI PATHS:
#=======================================================================
EFI_WS_CACHE=/efi_clim/data/cache
EFI_WS_SCRATCH=/efi_clim/data/scratch
EFI_ECFS_PATH=ec:/emos/efi_clim
EFI_HPCF_PATH=/home/ma/emos/data/efi_clim
#=======================================================================
#  Ocean model configuration
#=======================================================================

OCVER=h2e2
ASVER=cy2r3

#=======================================================================
#  Configuration for sensitivity suite
#=======================================================================

SENSFCRES=255
SENSFCLEV=60
SENSADJRES=63
SENSADJLEV=60
SENSADJGTYPE=_2
SENSFCGTYPE=l_2
SENSADJSTEP=48
SENSADJTSTEP=600.0
SENSRUNFC=yes
INISPQ=no

#new for 25r1
SENSADJADVEC=euler
SENSFORCE=false
SENSDIAB=true
SENSITER=6

#=======================================================================
#  The disk configuration
#  Look at e-suite configuration below also
#=======================================================================

if [[ $ARCH = ibm_power* ]] ; then
  if [[ "%SUITE%" = mc_no ]] ; then
    TROOT=$STHOST/ms_crit/teps
    WROOT=$STHOST/ms_crit/teps
    OROOT=$STHOST/emos_data
    FDB_ROOT=$STHOST/ms_crit/fdb
    FDB_IROOT=$STHOST/ms_crit/fdb
    WAVEEPS_ROOT=$FDB_ROOT
    WAVEEPS_IROOT=$FDB_IROOT
  elif [[ "%SUITE%" = e*35r3* || "%SUITE%" = "e_sync" || "%SUITE%" = "emc_no*" ]] ; then
    TROOT=$STHOST/emos_esuite/emos_data
    WROOT=$STHOST/emos_esuite/emos_data
    OROOT=$STHOST/emos_data
    WBASE=/
    FDB_ROOT=$STHOST/emos_esuite/ma_fdb
    FDB_ROOT_BKUP=$STHOST_BKUP/emos_esuite/ma_fdb
    FDB_IROOT=$STHOST/emos_esuite/ma_fdb
    WAVEEPS_ROOT=$FDB_ROOT
    WAVEEPS_IROOT=$FDB_IROOT
  else
    TROOT=$STHOST/emos_data
    WROOT=$STHOST/emos_data
    OROOT=$STHOST/emos_data
    FDB_ROOT=$STHOST/ma_fdb
    FDB_ROOT_BKUP=$STHOST_BKUP/ma_fdb
    FDB_IROOT=$STHOST/ma_fdb
    WAVEEPS_ROOT=$STHOST$FDB_ROOT
    WAVEEPS_IROOT=$STHOST$FDB_IROOT
  fi
  if [[ "%FSFAMILY%" = mars  ]] ; then
    WROOT=/od_archive/data
  fi
elif [[ $ARCH = linux ]] ; then
  TROOT=/var/tmp/tmpdir/emos
  WROOT=/var/tmp/tmpdir/emos
  OROOT=/var/tmp/tmpdir/emos
  FDB_ROOT=""
  FDB_IROOT=""
else
  TROOT=/var/tmp/emos
  WROOT=/var/tmp/emos
  OROOT=/var/tmp/emos
  FDB_ROOT=""
  FDB_IROOT=""
fi

WBASE=${WROOT}/${EXPVER}

XROOT=/home/ma/emos
XLIB=$XROOT/lib/${IFS_CYCLE}
XDATA=$XROOT/data
XBINS=$XROOT/bin
XMOD=/cc/rd/module/frt
XSRC=$XROOT/src
ODATA=/home/ma/emos/data
SCRATCH=/ws/scratch/ma/emos

FSROOT=/$USER
FSOROOT=/emos
FSFAMILY=""
FSXDATA=/emos/data
FSXBINS=/emos/$ARCH/bin

#=======================================================================
#
#  The following is the configuration filled in by the SMS
#
#=======================================================================

#EXPVER=%VERSION% # done further up...

if [[ "%USE_YMD:false%" = true ]] ; then
  BASETIME=%YMD%%EMOS_BASE%
else
  BASETIME=%YYYY%%MM%%DD%%EMOS_BASE%
fi

FSFAMILY=%FSFAMILY%

ECF_PASS=%ECF_PASS%
ECF_NODE=%ECF_NODE%
ECF_NAME=%ECF_NAME%
ECF_TRYNO=%ECF_TRYNO%

ECF_HOSTFILE=$HOME/.smshostfile
ECF_PORT=%ECF_PORT%
ECF_JOBOUT=%ECF_JOBOUT%


SUITE=%SUITE%
FAMILY=%FAMILY:NOT_DEF%
TASK=%TASK%

DEBUG=0
DISPLAY=0

# INFORM_EMOS_AUTORESOL=1

#
#  For e-suites only
#
if [[ $EXPVER -eq 2 ]] ; then
  BASEVER=0002
fi

#Multianalysis
INIMA="%INIMA:ecmwf%"
INIMACONS=%INIMACONS:0%
INIORIG=mars
MANNCEP=0
MANMFR=0
MANDWD=0
MANUKM=0

INIORIGIN=OFF
MAORIGINS="CONS"
MAMISSING="%MAMISSING:%"

if [[ %STREAM% = "MAED" || %STREAM% = MAWV ]] ; then
  WAMRESOL=global100
  WAMNFRE=30
  WAMNANG=24
  INIOZONE=false
  LOZONE=false
  LOZONECH=false
  RESOL=399
  LEVELS=62

  for xxx in $INIMA ; do
  if [[ "$xxx" = ncep ]] ; then
    MANNCEP=1
    MAORIGINS="$MAORIGINS KWBC"
  elif [[ "$xxx" = mfr ]] ; then
    MANMFR=1
    MAORIGINS="$MAORIGINS LFPW"
  elif [[ "$xxx" = ukm ]] ; then
    MANUKM=1
    MAORIGINS="$MAORIGINS EGRR"
  elif [[ "$xxx" = dwd ]] ; then
    MANDWD=1
    MAORIGINS="$MAORIGINS EDZW"
  fi
  done


  # Find WMO centre number. Do this more cleverly
  if [[ $INIMACONS -eq 1 ]] ; then
    INIORIGIN=CONS
  elif [[ $INIMA = mfr ]] ; then
   INIORIGIN=LFPW
  elif [[ $INIMA = ncep ]] ; then
   INIORIGIN=KWBC
  elif [[ $INIMA = ukm ]] ; then
   INIORIGIN=EGRR
  elif [[ $INIMA = dwd ]] ; then
   INIORIGIN=EDZW
  fi

fi

#=================================================
# For mars compute. This will use model number found.
# If not set, model number will be 255.
export MARS_COMPUTE_FLAG=0

# User for e-suite plots
EPLOT_USER=mos

set +a

if [[ %DELTA_DAY% -ne 0 ]] ; then
  delta=`expr %DELTA_DAY% \* 24`
  BASETIME=`newdate $BASETIME $delta`
fi

if [[ $FSFAMILY = "refc" ]] ; then
  HC_REFDATE=`echo $BASETIME | cut -c 1-8`
  EPSTYPE=fc

  export HC_REFDATE EPSTYPE

  if [[ %YEAR:0% -ne 0 ]] ; then
    YEAR=%YEAR:0%
    YYYY=`echo $BASETIME | cut -c 1-4`
    MM=`echo $BASETIME | cut -c 5-6`
    DDHH=`echo $BASETIME | cut -c 7-10`
    if [[ $MM$DDHH = 0229* ]] ; then
      BASETIME=`newdate $(( $YEAR$MM$DDHH - 100 )) 24 `
    else
      BASETIME=$YEAR$MM$DDHH
    fi
  fi
fi

DELTA_YEAR=%DELTA_YEAR:0%
YYYY=`echo $BASETIME | cut -c 1-4`
YYYY=$(( $DELTA_YEAR + $YYYY ))
if [[ $DELTA_YEAR -ne 0 ]] ; then
  YYYY=`echo $BASETIME | cut -c 1-4`
  DELTA_YEAR=%DELTA_YEAR:0%
  MMDDHH=`echo $BASETIME | cut -c 5-10`
  YYYY=$(( $DELTA_YEAR + $YYYY ))
  BASETIME=$YYYY$MMDDHH
  YMD=$(substring $BASETIME 1 8)
fi

DELTA_HOUR=%DELTA_HOUR:0%
if [[ $DELTA_HOUR -ne 0 ]] ; then
  BASETIME=`newdate $BASETIME $DELTA_HOUR`
fi

# remove when hpcf gone
if [[ $HOST = hpc* ]] ; then
  EMOSLIB="-qextname $EMOSLIB"
fi

if [[ $ARCH = linux ]] ; then
  export LINK_C="pgf90 -tp k8-32 -g"
  export LINK_F="pgf90 -tp k8-32 -g"
fi

if [[ "%OD_PROJ:od%" = no ]] ; then
  ### TEPS project
  EPSHEM=1
  EPSGAMMA=0.016
  EPSBASIS=10
  EPSMEMBERS=21
  EPSNENS=20
  GRIBCLASS=113 # from http://www.ecmwf.int/publications/manuals/libraries/gribex/gribClass.html
fi

#=====================================================================
# ocean model configuration
#======================================================================
export RUNVARFC=%RUNVARFC:0%
export EPSLEG=%EPSLEG:1%
if [[ $RUNHINDCAST = 1 ]] ; then
  # export HC_REFDATE=%YMD%
  export HC_REFDATE=$((YYYY -(DELTA_YEAR)))`echo $BASETIME | cut -c 5-8`

INIMODISALB=false # if intial data was run with 32r1 or later

#==========================================================
# Variables configuring inidata
#==========================================================
INIPATH="/net/fujitsu/ifs/initial/t95r/ic_19r1_%YMD%"
INISTREAM=DA
INITYPE=an
INISTEP=0
INISPQ=no
LFG=.true.
INIOFFSET=0
INISCHEME=lslag
SAVINI=""
SVRF=no
IFRF=no
FORCE_FP=no

INICLASS=`oper_model -b $BASETIME -c`
INIRESOL=`oper_model -b $BASETIME -r`
INILEVELS=`oper_model -b $BASETIME -l`
INIGTYPE=`oper_model -b $BASETIME -t`
INIEXPVER=`oper_model -b $BASETIME -e`
INITILES=`oper_model -b $BASETIME -T`
INICLOUDAN=`oper_model -b $BASETIME -C`

if [[ %RUNHINDCAST:0% = 0 ]] ; then
  INILEVELS=91
  INIRESOL=799

if [[ $BASETIME -le %ERA40END:2002073100% ]] ; then
  INICLASS=e4
  INILEVELS=60
  INIEXPVER=0001
  INITILES=true
  INICLOUDAN=true
  INIMODISALB=false
  EPSGAMMA=0.018
fi

else # HINDCAST mode
  INICLASS=ei
  INILEVELS=60

  if [[ $BASETIME -lt 2008010100 ]] ; then
    INIEXPVER=0001
  else
    INIEXPVER=1112 # temporary while Manuel is changing 1112 to 0001
  fi
  EPSGAMMA=0.016 # to be updated according to Martin Leutbecher
  INIRESOL=255
  INIMODISALB=true
  INIHTESSEL=false
  INITILES=true
fi

fi

export MARS_CMD='mars'

if [[ %RUNHINDCAST:0% = 1 ]] ; then
  INIHTESSEL=`oper_model -b $BASETIME -I ${CLASSORIG:="er_ops"} -H`
fi

### check if MOFC-VarEps is to run
  sim=`newdate -d -D $BASETIME`
  real=`date +'%%Y%%m%%d'`
  real=`newdate -d -D $real`
  dow=`date +'%%u'`
  res=$((($sim - $real + $dow) %% 7))

  if [[ $((res)) -lt 0 ]] ; then res=$((res+7)); fi

  RUN_VAREPS=0
  if [[ $res = 4 ]] ; then # Thursday MOFC
    RUN_VAREPS=1
  fi
  export EPSNLEGS=$((2+$RUN_VAREPS)) # do not refer to sms variables while this changes
  # 35r2
  export EPSCOUPL_A=no                    # 35r2
  export EPSCOUPLE_A=0                    # 35r2
  CC=`substring $BASETIME 9 10`           # later EMOS_BASE
  export EPSCOUPLE_B=$((CC == 00))        # 35r2
  export EPSCOUPLE_C=$RUN_VAREPS          # 35r2


export RUN_VAREPS
###########
export OCNINDAT=$(substring $BASETIME 1 8) # 19900101

if [[ $EPSLEG != 1 ]] || [[ $RUNVARFC = 1 ]] ; then
set -a


ENAMELIST=general
FULL_POS=yes
LPPL=1
LPSU=1
LPPV=0
LPTH=0
LPGZ=0
PPSTEPS="0"
ERF=408
FRQRF=-1
DATE_BRANCH=2005112409
USEDATE_BRANCH=no
VIEW=""
VIEW_HOPE="hope_h2e20"
OCEPS=no
OCUSESV=no
EPSLROT=no
OCPATM=yes
OCFCRESOL=255
OCFCLEVELS=62
OCFCGTYPE=l_2
OCFCTSTEP=%TSTEP:1800%.0
FCCHUNKTOT=1
NEWSTREAMLIB=none
SAVLIB=false
OCEAN_BRANCH="neh_h2e20_enfo"
OCEAN_CYCLE="h2e20"
OCIAUFR=2
if [[ $RUNHINDCAST = 1 ]] ; then

GRIB_API_VERSION=1.7.0
GRIB_API_INCLUDE="-I/usr/local/lib/metaps/lib/grib_api/jasper/include -I/usr/local/lib/metaps/lib/grib_api/$GRIB_API_VERSION/include"
GRIB_API_LIB="-L/usr/local/lib/metaps/lib/grib_api/jasper/lib -ljasper -L/usr/local/lib/metaps/lib/grib_api/$GRIB_API_VERSION/lib -lgrib_api_f90 -lgrib_api"
export GRIB_API=/usr/local/lib/metaps/lib/grib_api/$GRIB_API_VERSION/bin   # change to fixed version
export PATH=$GRIB_API:$PATH

  if [[ $HC_REFDATE -lt 20080605 ]] ; then
    OCINI="seas_0001_01_NN_${OCNINDAT}_od_02"
  else
    OCINI="ocea_0001_01_NN_$(substring $BASETIME 1 8)_od_03"
  fi
else
  OCINI="ocea_0001_01_NN_$(substring $BASETIME 1 8)_od_03"
fi
OCNEWREST=false
OCNMINI="00"
CLASSORIG=e4_ops

OCRUNCTRLAN=no
OCRUNCTRLAC=no
OCRUNCTRLFC=no
OCRUNASSIMAN=no
OCRUNASSIMAC=yes
OCRUNASSIMFC=yes
MEMBERSTATE=no
%include <config.oc.h>

 OCINPUT=mars
 OCSUITE=vareps
 if [[ -n $OCINI ]] ;then
    OCINPUT=$(substring $OCINI 1 4)
 fi

 if [[ $OCINPUT != 'ecfs' ]] ; then
   OCSTREAM_INI=$OCINPUT
    if [[ $OCINPUT = 'mars' ]];then
      OCSTREAM_INI='ocea' # 'seas'
    fi
    if [[ $OCINPUT = 'seas' ]];then
      OCINPUT='mars'
    fi
    if [[ $OCINPUT = 'mofc' ]];then
     OCINPUT='mars'
    fi
   OCEXPVER_INI=0001 # $(substring $OCINI 6 9 )
   OCMETHOD_INI=$(substring $OCINI 11 12 )
   OCNUMBER_INI=$(substring $OCINI 14 15 )
   OCDATE_INI=$(substring $OCINI 17 24 )
   OCCLASS_INI=$(substring $OCINI 26 27 )
   OCSYST_INI=$(substring $OCINI 29 30 )
 fi

# For EPS runs only. Change of EPSNORM
#
   LASTERA40=%ERA40END:2000070100%

  if [[ $OCEPS = "yes" ]] ; then
     if [[ $BASETIME  -le 2001031400 ]] || [[ $BASETIME  -le $LASTERA40 ]] ;
     then
        EPSNORM=2.0
        EPSNORMT=2.0
     fi
     EPSTROP=%EPSTROP:$EPSTROP%
     EPSSVDIAB=%EPSSVDIAB:$EPSSVDIAB%
     EPSSVNUM=%EPSSVNUM:$EPSSVNUM%
  fi

#=======================================================================
#  special mofc variables
#=======================================================================

# pop/web/mofc

MOFC_PROCESS=$WROOT/data/%SUITE%_process
MOFC_PROCESS=`echo $MOFC_PROCESS | sed -e 's/var\/tmp\/tmpdir\/emos\/data/emos_data/'`
Y=`echo $BASETIME | cut -c 1-4`
STREAM=EF
CLIMYEAR1=$((Y + - %NH_YEARS:18%))
CLIMYEAR2=$((Y - 1))
OCUTILS=$XBINS/$STREAM/$EXPVER

export NH_ENSEMBLES=%NH_ENSEMBLES:4%
export MOFC_PROCESS

if [[ %RUNHINDCAST:0% = 1 ]] ; then
  MOFCCLIM_VERIF=$WROOT/data/%SUITE%_clim
  [[ -d $MOFCCLIM_VERIF ]] || mkdir -p $MOFCCLIM_VERIF
  MOFC_VERIFY=/gpfs1/emos_verify/data/%SUITE%_verify
  export PATH=$PATH:$HOME/bin/EF/verify
  [[ -d $MOFC_VERIFY ]] || mkdir -p $MOFC_VERIFY

  INIHTESSEL=`oper_model -b $BASETIME -I ${CLASSORIG:="er_ops"} -H`
fi

fi

set -a

if [[ %EPSTYPE:cf% = sv ]] ; then
   INIMODISALB=false
fi

if [[ "%STREAM%" = EF ]] ; then
  INIPERIOD_4D=%INIPERIOD_4D:12%
else
  INIPERIOD_4D=12
fi

if [[ "%FSFAMILY:0%" != refc ]] && [[ %RUNHINDCAST:0% != 1 ]] ; then
  LVARBC=true
else
  LVARBC=false
  INIMODISALB=false # if intial data was run with 32r1 or later

  date4v12h=2000091200
  # CHANGE date_dcda=2004062900
  date_dcda=2004062912
  if [ $BASETIME -ge $date4v12h ] ; then
     INIPERIOD_4D=12
  fi
  if [ $BASETIME -ge $date_dcda ] ; then
    INIPERIOD_4D=6
  fi
fi

inidate=`echo $BASETIME | cut -c 1-8`
if [ ${inidate} -ge 19971125 ] && [ $INICLASS = od ] && [ $INIPERIOD_4D = 6 ] ; then
    INIOFFSET_4D=3
elif [ $INICLASS = ei ] ; then
    INIOFFSET_4D=9
elif [ $INICLASS = e4 ] ; then
    INIOFFSET_4D=3
elif [ $INIPERIOD_4D = 12 ] ; then
  if [[ %RUNHINDCAST:0% = 1 ]] || [[ %STREAM:UNDEF% = EF ]] ; then
    INIOFFSET_4D=9
  else
    INIOFFSET_4D=3
  fi
elif [ $INIPERIOD_4D = 6 ] ; then
    INIOFFSET_4D=3
else
    INIOFFSET_4D=0
fi

### SST ###
CLMSST=no
PERSST=%PERSST:false%
FORCE_SST=no
SSTLEN=%SSTLEN:240%
SSTINT=%SSTINT:1%
LOSTIA=%LOSTIA:true%              # 33r2

NUMBER=%FCTOTAL:NOT_SET%
VAR=%VAR:NOT_SET%
CNUMBER=5

if [[ "%SUITE%" = @(mc*|emc*|mofc*|emofc*|ma|tparc*|dts*) || $SUITE_TYPE = "eps" ]] ; then
  LWEAK4DVAR=false
  LEMISKF=false
  LOZONE=false
  INIOZONE=false
  LOZONECH=false
  LVARBC=false
else
  LWEAK4DVAR=true
  LEMISKF=true
fi


set +a


