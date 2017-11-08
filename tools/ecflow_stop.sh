#!/bin/sh
#==========================================================================
##.TITLE   ECMWF utility for ecFlow
##.NAME    ecflow_stop.sh
##.SECTION ECFLOW
##.AUTHOR  Avi
## Revision    : $Revision: #10 $ 
##
## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 
##
##.FILE    ecflow_stop.sh
##.INFO    this file is expected to be located in /usr/local/share
##         it is to be used on ecgate by member states users  
##         one ecf server occurrence will be generated on ecgate
#==========================================================================

#set -eux

PATH=/usr/local/bin:/usr/bin:$PATH
export TZ=GMT LANG=en_GB.UTG-8
host=$(hostname)
backup_server=false

# =========================================================================
# Update host, ecflow_site.sh is configured from CMAKE at install time
# =========================================================================
if [ -f ecflow_site.sh ] ; then
$(source ./ecflow_site.sh)
fi

#==========================================================================
# Syntax
# ecf_stop [-b] [-p port_number ] [-h]
# get command line options if any.
#==========================================================================
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
# PORT NUMBER is set based on the unique users numeric uid.
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

#==========================================================================
# HOST
date -u

rcdir=$HOME/.ecflowrc
fname=$rcdir/$(echo $host | cut -c1-4).$USER.$ECF_PORT 
# cut is useful when the server may be moved from node to node 
# 4 is common string here, so that the same file is used for all nodes

if [ -f $fname ]; then host=$(cat $fname); fi

echo ""
echo "User \"$username\" attempting to stop ecf server on $host:$port_number"
echo "";
echo "Checking if the server is running on $host:$port_number" 

export ECF_HOST=$host

ecflow_client --ping 
if [ $? -eq 1 ]; then
  echo "";
  echo "... The server on $host:$port_number has already been stopped" 
  exit 1
fi

#==========================================================================
echo "";
echo Halting, check pointing and terminating the server

ecflow_client  --halt=yes
ecflow_client  --check_pt
ecflow_client  --terminate=yes

exit 0
