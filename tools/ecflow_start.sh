#!/bin/sh
#set -x
#set -u
#==========================================================================
##.TITLE   ECMWF utility for ECFLOW
##.NAME    ecf_start
##.SECTION ECF
##.AUTHOR  Avi
## Revision    : $Revision: #19 $ 
##
## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 
##
##.FILE    ecflow_start. 
###        Will start the ecflow_server in the background, using user id
###        to make a unique port number.

#==========================================================================
PATH=/usr/local/bin:/usr/bin:$PATH
export TZ=GMT LANG= # en_GB.UTF-8 unset, use locale -a to list available locales
host=$(hostname)
force=true
backup_server=false
verbose=false
rerun=false

#==========================================================================
# Syntax
# ecflow_start [-b] [-d ecf_home_directory] [-f] [-h] [-p port_number ]
#==========================================================================
# get command line options if any.
while getopts hfbd:vp:r option
do
case $option in
f)
force=true
;;
b)
backup_server=true
;;
v)
verbose=true
;;
d)
ecf_home_directory=$OPTARG
;;
p)
ecf_port=$OPTARG
;;
r)
rerun=true
;;
h)
echo "Usage: $0 [-b] [-d ecf_home directory] [-f] [-h]"
echo "       -b        start ECF for backup server or e-suite"
echo "       -d <dir>  specify the ECF_HOME directory - default $HOME/ecflow_server"
echo "       -f        forces the ECF to be restarted"
echo "       -v        verbose mode"
echo "       -h        print this help page"
echo "       -p <num>  specify server port number(ECF_PORT number)  - default 1000+<UID> | 500+<UID> for backup server"
exit 0
;;
*)
echo "Usage: $0 [-b] [-d ecf_home directory] [-f] [-h]"
echo "       -b        start ECF for backup server or e-suite"
echo "       -d <dir>  specify the ECF_HOME directory - default $HOME/ecflow_server"
echo "       -f        forces the ECF to be restarted"
echo "       -v        verbose mode"
echo "       -h        print this help page"
echo "       -p <num>  specify server port number(ECF_PORT number)  - default 1500+<UID> | 1000+<UID> for backup server"
exit 1
;;
esac
done
 

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
# if working directory not set then set to current directory  

export ECF_HOME=${ecf_home_directory:-$HOME/ecflow_server}
export ECF_LISTS=${ECF_LISTS:-$ECF_HOME/ecf.lists}

# ===============================================================================
# Update kill and status command for ecgate

if [ $host = ecga* ] ; then 
    # ECF_JOB_CMD='%ECF_JOB% 1> %ECF_JOBOUT% 2>&1 &'
    # ECF_JOB_CMD='${ECF_JOB:=/home/ma/emos/bin/ecfsubmit} %USER% %HOST% %ECF_JOB% %ECF_JOBOUT% > %ECF_JOB%.subm 2>&1'
    ECF_KILL_CMD='${ECF_KILL:=/home/ma/emos/bin/ecfkill} %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1'
    ECF_STATUS_CMD='${ECF_STAT:=/home/ma/emos/bin/ecfstatus} %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1'
    export ECF_KILL_CMD ECF_STATUS_CMD
fi


# ===============================================================================
# Update host and create hosts file

if [ $host = ecga* ] ; then 
  host=ecgate

  file=$HOME/.ecfhostfile
  grep ecga00 $file || ( cat >> $file <<EOF
ecga00
ecga01
ecga02
ecga03
ecga04
ecga05
EOF

  if [ "$USER" != emos ] ; then
    rcp $file $USER@c2a:~/.ecfhostfile || :
    rcp $file $USER@c2b:~/.ecfhostfile || :
  fi
) 

  file=$HOME/.ecflowrc/servers
  nick=ecgate1
  grep "^ecgate " $file || echo "ecgate  ecgate  $ECF_PORT" >> $file
  grep "^$nick  " $file || echo "$nick $nick $ECF_PORT" >> $file
fi
 
date -u

# set up default environment variables
export ECF_NODE=$host
export ECF_LOG=$host.$ECF_PORT.ecf.log
export ECF_CHECK=$host.$ECF_PORT.check
export ECF_CHECKOLD=$host.$ECF_PORT.check.b
if [ "$verbose" = "false" ]; then
     export ECF_OUT=/dev/null
else
     export ECF_OUT=$host.$ECF_PORT.ecf.out
fi

echo 
echo User \"$username\" attempting to start ecf server on \"$host\" using ECF_PORT \"$ECF_PORT\" and with:
echo "ECF_HOME     : \"$ECF_HOME\""
echo "ECF_LOG      : \"$ECF_LOG\""
echo "ECF_CHECK    : \"$ECF_CHECK\""
echo "ECF_CHECKOLD : \"$ECF_CHECKOLD\""
if [ "$verbose" = "false" ]; then
     echo "ECF_OUT      : \"/dev/null\""
else
     echo "ECF_OUT      : \"$host.$ECF_PORT.ecf.out\""
fi
echo 

#==========================================================================

echo "client version is $(ecflow_client --version)"
# echo "server version is $(ecflow_server --version)"
echo "Checking if the server is already running on $host and port $ECF_PORT"
ecflow_client --ping 
if [ $? -eq 0 ]; then
  echo "... The server on $host:$ECF_PORT is already running. Use 'netstat -lnptu' for listing active port" 
  exit 1
fi


#==========================================================================

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
cp $ECF_LOG      log/ 2>/dev/null

if [ -f $host.$ECF_PORT.ecf.out ]; then
   cp $host.$ECF_PORT.ecf.out log/ 2>/dev/null
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

nohup ecflow_server > $ECF_OUT 2>&1 < /dev/null &

# the sleep allows time for server to start
if [ "$force" = "true" ]; then
   echo "Placing server into RESTART mode..."
   sleep 5  
   ecflow_client --restart || { echo "restart of server failed" ; exit 1; }
fi


echo 
echo "To view server on ecflowview - goto Edit/Preferences/Servers and enter"
echo "Name        : <unique ecFlow server name>"
echo "Host        : $host"
echo "Port Number : $ECF_PORT"
echo

exit 0
