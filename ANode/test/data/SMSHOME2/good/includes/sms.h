## Copyright 2009-2016 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.

#==========================================================================
#  Define environment variables and export them.
#==========================================================================

set -a

ECF_PASS=%ECF_PASS%
ECF_HOST=%ECF_HOST%
ECF_NAME=%ECF_NAME%
ECF_TRYNO=%ECF_TRYNO%
ECF_RID=$(echo $QSUB_REQID | cut -f1 -d.)
ECF_HOSTFILE=$HOME/.smshostfile
ECF_PORT=%ECF_PORT%
ECF_JOBOUT=%ECF_JOBOUT%

SUITE=%SUITE%
FAMILY=%FAMILY%
TASK=%TASK%

DEBUG=0
DISPLAY=0

#==========================================================================
#  Initialize sms.
#  Trap handling.
#  Modify the ERROR function what to do in case of any error.
#==========================================================================

#if [[ $ARCH = sgimips && -f /usr/local/ecfs/prodn/.ecfs_k_env ]] ; then
if [[ -f /usr/local/ecfs/prodn/.ecfs_k_env ]] ; then
  . /usr/local/ecfs/prodn/.ecfs_k_env
fi

smsinit $ECF_RID &

ERROR() {
  smsabort; printenv; trap 0; exit
}

trap ERROR 0
trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15

set -ex

if [[ $ARCH = sgimips && -f /usr/local/ecfs/prodn/.ecfs_k_env ]] ; then
  . /usr/local/ecfs/prodn/.ecfs_k_env
fi

