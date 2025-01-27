#!/bin/bash
#set -x
#set -u
#==========================================================================
##.TITLE   ECMWF utility for ECFLOW
##.NAME    ecflow_start.sh
##.SECTION ECF
##.AUTHOR  Avi
## Revision    : $Revision: #19 $ 
##
## Copyright 2009- ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 
##
##.FILE    ecflow_start.sh
###        Will start the ecflow_server in the background, using user id
###        to make a unique port number.

if [[ $(hostname) = a[a-d]?-*.bullx && -z ${ECFLOW_ALLOW_LOCAL+x} ]]; then
    echo "To use an ecFlow server on the Atos HPC at ECMWF, please read instructions: https://confluence.ecmwf.int/display/UDOC/HPC2020%3A+Using+ecFlow"
    exit 0
fi

#===============================================================================
# Get the absolute path THIS script. use it to locate ecflow_client and ecflow_server
# This avoids mixing 4/5 version of ecflow.
# and need absolute since we change dir later on.
#
ECFLOW_BINDIR="$( cd "$(dirname "$0")" ; pwd -P )" 
#echo "-----> ${ECFLOW_BINDIR} <-------"

#==========================================================================
export TZ=GMT LANG= # en_GB.UTF-8 unset, use locale -a to list available locales
host=$(hostname)
force=true
backup_server=false
verbose=false
rerun=false
check=false
ssl=false
halted=false

usage()
{
    echo "Usage: $0 [-b] [-c] [-d ecf_home_dir] [-f] [-H] [-v] [-s] [-p port]"
    echo "       -b        start ECF for backup server or e-suite"
    echo "       -c        test check point file for errors"
    echo "       -d <dir>  specify the ECF_HOME directory - default $HOME/ecflow_server"
    echo "       -f        forces the ECF to be restarted"
    echo "       -H        Halted mode (manual restart), overrides -f option"
    echo "       -v        verbose mode"
    echo "       -s        enable ssl server. Requires client/server built with openssl libs"
    echo "       -h        print this help page"
    echo "       -p <num>  specify server port number(ECF_PORT number)  - default 1500+<UID> | 1000+<UID> for backup server"
}

#==========================================================================
# Syntax
# ecflow_start [-b] [-d ecf_home_directory] [-f] [-h] [-s] [-p port_number]
#==========================================================================
# get command line options if any.
while getopts chHfbd:vsp:r option
do
case $option in
    c) check=true;;
    f) force=true;;
    H) halted=true;;
    b) backup_server=true;;
    v) verbose=true;;
    d) ecf_home_directory=$OPTARG;;
    p) ecf_port=$OPTARG;;
    r) rerun=true;;
    s) ssl=true;;
    h)
    usage
    exit 0
    ;;
    *)
    usage
    exit 1
    ;;
esac
done

# ================================================================================
# SSL
if [ "$ssl" = "true" ]; then
     export ECF_SSL=1  # required for both client and server
fi

# =================================================================================
# port_number is set based on the unique users numeric uid.

username=`id -u`

if [ -z "$ecf_port" ] ; then

   if [ $backup_server = "true" ]; then
     base=1000
   else
     base=1500
   fi
   port_number=$((base+username))

else
   port_number=$ecf_port
fi

export ECF_PORT=$port_number

#===============================================================================
# Setup ECF_HOME 

export ECF_HOME=${ecf_home_directory:-$HOME/ecflow_server}
export ECF_LISTS=${ECF_LISTS:-$ECF_HOME/ecf.lists}

# ===============================================================================
# If server is already started then exit

rcdir=$HOME/.ecflowrc
fname=$rcdir/$(echo $host | cut -c1-4).$USER.$ECF_PORT 
# cut is useful when the server may be moved from node to node 
# 4 is common string here, so that the same file is used for all nodes

if [ -f $fname ]; then host=$(cat $fname); fi

mkdir -p $rcdir
THERE=KO
${ECFLOW_BINDIR}/ecflow_client --port=$ECF_PORT --host=$host --ping && THERE=OK
if [[ $THERE == OK ]]; then
  echo "server is already started"
  res="$(ps -lf -u $USER | grep ecflow_server | grep -v grep)"
  # which netstat && res="$(netstat -lnptu 2>/dev/null | grep ecflow | grep $ECF_PORT)"
  echo "$res $(${ECFLOW_BINDIR}/ecflow_client --stats)"
  if [ "$res" == "" ] ; then
    mail $USER -s "server is already started - server hijack?" <<EOF
Hello.

there was an attempt to start the ecFlow server while port is already in use
by another user, see the ecflow stats output below.

$(${ECFLOW_BINDIR}/ecflow_client --stats)
EOF
    exit 1
  fi
  exit 0 || :
fi

# =========================================================================
# Update host, ecflow_site.sh is configured from CMAKE at install time
# =========================================================================
if [ -f ${ECFLOW_DIR:=/usr/local}/bin/ecflow_site.sh ] ; then
  source ${ECFLOW_DIR}/bin/ecflow_site.sh
fi


date -u

# ======================================================================================
# set up default environment variables
#
export ECF_HOST=$host
export ECF_LOG=$ECF_HOST.$ECF_PORT.ecf.log
export ECF_CHECK=$ECF_HOST.$ECF_PORT.check
export ECF_CHECKOLD=$ECF_HOST.$ECF_PORT.check.b
if [ "$verbose" = "false" ]; then
     export ECF_OUT=/dev/null
else
     export ECF_OUT=$ECF_HOST.$ECF_PORT.ecf.out
fi

echo 
echo User \"$username\" attempting to start ecf server on \"$ECF_HOST\" using ECF_PORT \"$ECF_PORT\" and with:
echo "ECF_HOME     : \"$ECF_HOME\""
echo "ECF_LOG      : \"$ECF_LOG\""
echo "ECF_CHECK    : \"$ECF_CHECK\""
echo "ECF_CHECKOLD : \"$ECF_CHECKOLD\""
if [ "$verbose" = "false" ]; then
     echo "ECF_OUT      : \"/dev/null\""
else
     echo "ECF_OUT      : \"$ECF_HOST.$ECF_PORT.ecf.out\""
fi
echo 

#==========================================================================

echo "client version is $(${ECFLOW_BINDIR}/ecflow_client --version)"
echo "Checking if the server is already running on $ECF_HOST and port $ECF_PORT"
${ECFLOW_BINDIR}/ecflow_client --ping 
if [ $? -eq 0 ]; then
  echo "... The server on $ECF_HOST:$ECF_PORT is already running. Use 'netstat -lnptu' for listing active port" 
  exit 1
fi

#==========================================================================
#
echo "";
echo Backing up check point and log files

if [ ! -d $ECF_HOME ] ;then
  mkdir $ECF_HOME
fi
cd $ECF_HOME

if [ ! -d log ] ;then
  mkdir log
fi

set +e

cp $ECF_CHECK    log/ 2>/dev/null
cp $ECF_CHECKOLD log/ 2>/dev/null
if [ -f $ECF_LOG ]; then 
    STAMP=$(date +%Y%m%d.%H%M)
    SIZE=$(du -Hm $ECF_LOG | awk '{print $1}') || SIZE=0
    if [ $SIZE -gt 100 ]; then
	     echo "Moving, compressing logfile ${SIZE}mb ${ECF_LOG}.${STAMP}.log"
	     mv $ECF_LOG log/${ECF_LOG}.${STAMP}.log 2>/dev/null
	     gzip -f log/${ECF_LOG}.${STAMP}.log 2>/dev/null
    fi
fi
cp $ECF_LOG log/ 2>/dev/null  # allow logfile append in case of multiple restart

if [ -f $ECF_HOST.$ECF_PORT.ecf.out ]; then
   cp $ECF_HOST.$ECF_PORT.ecf.out log/ 2>/dev/null
fi

set -e

# =============================================================================
# ecFlow server start in the background.
#
# o/ nohup is a POSIX command to ignore the HUP (hangup) signal, enabling the command to 
#    keep running after the user who issues the command has logged out. 
#    The HUP (hangup) signal is by convention the way a terminal warns depending processes of logout. 
#
#    Note that these methods prevent the process from being sent a 'stop' signal on logout,
#    but if input/output is being received for these standard IO files (stdin, stdout, or stderr), 
#    they will still hang the terminal
#    This problem can also be overcome by redirecting all three I/O streams:
#
# o/ ecflow_server will by default attempt to recover from a check point file if it is there
#    otherwise it will look for the backup check point file
#
echo "";
echo "OK starting ecFlow server..."
echo "";

if [ $check == true ]; then
  ${ECFLOW_BINDIR}/ecflow_client --load $ECF_CHECK check_only
fi

nohup ${ECFLOW_BINDIR}/ecflow_server > $ECF_OUT 2>&1 < /dev/null &

# the sleep allows time for server to start
if [ "$halted" = "true" ]; then
   echo "Server is in Halted mode..."  
elif [ "$force" = "true" ]; then
   echo "Placing server into RESTART mode..."
   sleep 5  
   ${ECFLOW_BINDIR}/ecflow_client --restart || { echo "restart of server failed" ; exit 1; }
fi


echo 
echo "To view server on ecflow_ui - goto Servers/Manage Servers... and enter"
echo "Name        : <unique ecFlow server name>"
echo "Host        : $ECF_HOST"
echo "Port Number : $ECF_PORT"
echo

exit 0
