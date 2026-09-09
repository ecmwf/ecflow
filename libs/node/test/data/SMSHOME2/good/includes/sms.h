# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

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

