## Copyright 2009-2016 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.

# EXPVER=et34
#==========================================================
# Variables configuring emptyNamelist
#==========================================================
ENAMELIST=general
#
#==========================================================
# Variables configuring wsconfig
#==========================================================

# WSHOST=metis
# SCRATCH="/scratch/rd/rdx"
#
#==========================================================
# Variables configuring ppgeneral
#==========================================================

IFSMODE=fc
#PERIOD=6
#PERIOD_4D=12

FULL_POS=yes
LPPL=1
LPSU=1
LPML=0
LPPV=0
LPTH=0
LPGZ=0
#PPFRQ=24
PPSTEPS="0"
#BRF=0
ERF=408
FRQRF=-1
#
#==========================================================
# Variables configuring Ozone
#==========================================================
LOZONE=false
LOZONECH=false
#
#==========================================================
# Variables configuring clearcase
#==========================================================
#PROJECTS="none"
DATE_BRANCH=2005112409
USEDATE_BRANCH=no
VIEW=""
VIEW_HOPE="hope_h2e20"
#
#==========================================================
# Variables configuring eps_svect
#==========================================================
OCEPS=no
OCUSESV=no
EPSLROT=no
EPSNLEGS=%EPSNLEGS:3%
EPSFCLEV_A=40
EPSTSTEP_A=1800
EPSFCLEV_B=40
EPSTSTEP_B=%TSTEP:2700% # 2700
EPSFCLEV_C=40
EPSTSTEP_C=%TSTEP:2700% # 2700

#
#==========================================================
# Variables configuring Dr_Hook
#==========================================================
#DR_HOOK=true
#DR_HOOK_OPT="none"
#DR_HOOK_PROFILE_LIMIT=-10
#DR_HOOK_HASHBITS=15
#DR_HOOK_CATCH_SIGNALS=0
#DR_HOOK_IGNORE_SIGNALS=0
#
#==========================================================
# Variables configuring wavgeneral
#==========================================================
#WAVE=yes
#INIWAVE=yes
#WAM2WAY=yes
#WAMALT=no
#WAMSAR=no
#WAMSARASS=false
#WGRIBIN=yes
#WAMNSTPW=1
#WAMRESOL=global100
#WAMCOLDLENGTH=240
WAMNFRE=%EPSWAMNFRE:25%
WAMNANG=%EPSWAMNANG:12%
#WAMFCPARAM="229/230/231/232/220/221/244/233/245"
#
#==========================================================
# Variables configuring compile
#==========================================================
OCPATM=yes
#LD="mpxlf90_r"
#LDCC="xlc_r"
#USE=standard
#USECC=vac_6000
#PARALLEL=4
#NATIVE=true
#F90_DEBUG=0
#RUN_PARALLEL=true
#EC_FILL_NANS=false
#
#==========================================================
# Variables configuring system
#==========================================================
# ABO all comments
#FSROOT="/RDX/prepIFS"
#FSOROOT="/emos"
#ABO FSXDATA="/ocx/data"
#ABO FSXBINS="/ocx/$ARCH/bin"
#ACCOUNT=ecrmoc
#USER=ocx
#GROUP=rd
#FDB_GROUP=ecmwf
#BASEVER=0001
#CLASS=rd
#GRIBCLASS=2
#NOW=2005112409
#MEMBERSTATE=no
#
#==========================================================
# Variables configuring submit
#==========================================================
#SMS_NAME=ocx-prod
#
#==========================================================
# Variables configuring general
#==========================================================
OCFCRESOL=255
OCFCLEVELS=62
OCFCGTYPE=l_2
OCFCTSTEP=%TSTEP:1800%.0
FCCHUNKTOT=1
VERSION="fc"
LESUITE=false
#
#==========================================================
# Variables configuring libraries
#==========================================================
# IFS_CYCLE="31r1"
# Q2USE=true
# EMOS_CYCLE="000281"
# Q2CREATE_NEWSTREAM=false
# NEWSTREAM=none
NEWSTREAMLIB=none
# LIBRESOL=".R64.D64.I32"
SAVLIB=false
#GETLIB=false
# MAKE_LIBS=yes
# Q2CHANGELIST=0
OCEAN_BRANCH="neh_h2e20_enfo"
OCEAN_CYCLE="h2e20"
#
#==========================================================
# Variables configuring inidata
#==========================================================
#OCNINDAT=$(substring $BASETIME 1 8) # 19900101
#OCIAUFR=2
#if [[ $RUNHINDCAST = 1 ]] ; then
#OCINI="seas_0001_01_NN_${OCNINDAT}_od_02"
#else
#OCINI="ocea_0001_01_NN_$(substring $BASETIME 1 8)_od_03"
## "seas_0001_01_NN_${OCNINDAT}_od_02"
#fi
#OCNEWREST=false
#OCNMINI="00"
#INIORIG=mars
#CLASSORIG=e4_ops
#INISCHEME=lslag
SVRF=no
IFRF=no
#
#==========================================================
# Variables configuring ocgeneral
#==========================================================
OCRUNCTRLAN=no
OCRUNCTRLAC=no
OCRUNCTRLFC=no
OCRUNASSIMAN=no
OCRUNASSIMAC=yes
OCRUNASSIMFC=yes
#
MEMBERSTATE=no
#if [ $RUNVARFC -eq 1 ] ; then
#  OCSUITE=vareps
#fi

