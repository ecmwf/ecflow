#!/bin/sh
#==========================================================================
##.TITLE   ECMWF utility for ecFlow
##.NAME    ecf_stop
##.SECTION ECFLOW
##.AUTHOR  Avi
## Revision    : $Revision: #10 $ 
##
## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 
##
##.FILE    ecf_stop
##.INFO    this file is expected to be located in /usr/local/share
##         it is to be used on ecgate by member states users  
##         one ecf server occurence will be generated on ecgate
#==========================================================================

#set -eux

PATH=/usr/local/bin:/usr/bin:$PATH
export TZ=GMT LANG=en_GB.UTG-8
host=$(hostname)
backup_server=false

case $host in 
sappa*) host=sappa;;
sappb*) host=sappb;;
ecga*)  host=ecgate;;
esac

#==========================================================================
# Syntax
# ecf_stop [-b] [-p port_number ] [-h]
#==========================================================================
# get commane line options if any.
while getopts b:p: option
do
case $option in
b)
backup_server=true
;;
p)
ecf_port=$OPTARG
;;
h)
echo "Usage: $0 [-b] [-p port_number ] [-h]"
echo "       -b        stop ECF backup server"
echo "       -p <num>  specify the ECF_PORT number  - default 1000+<UID> | 500+<UID> for backup server"
echo "       -h        print this help page"
exit 0
;;
*)
echo "Usage: $0 [-b] [-p port_number ] [-h]"
echo "       -b        stop ECF backup server"
echo "       -p <num>  specify the ECF_PORT number  - default 1500+<UID> | 1000+<UID> for backup server"
echo "       -h        print this help page"
exit 1
;;
esac
done

#==========================================================================
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

date -u
echo ""
echo "User \"$username\" attempting to stop ecf server on $host:$port_number" 
 
#==========================================================================
echo "";
echo "Checking if the server is already running on $host:$port_number" 

export ECF_PORT=$port_number
export ECF_NODE=$host
set -x
rcdir=$HOME/.ecflowrc
fname=$rcdir/$(echo $ECF_NODE | cut -c1-5).$USER.$ECF_PORT # OK as long as ecgate node is under 10
if [[ -f $fname ]]; then host=$(cat $fname); fi

ecflow_client --host $host --ping 
if [ $? -eq 1 ]; then
  echo "";
  echo "... The server on $host:$port_number has already been stopped" 
  exit 1
fi

#==========================================================================
echo "";
echo Halting, check pointing and terminating the server

ecflow_client  --host $host --halt=yes
ecflow_client  --host $host --check_pt
ecflow_client  --host $host --terminate=yes

exit 0
