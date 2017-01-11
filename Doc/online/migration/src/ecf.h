#!/bin/ksh
export ECF_PORT=%ECF_PORT:0%
set -a
case $ECF_PORT in
0 ) echo sms world 
;;
* ) echo "ecf world"
   PATH=/usr/local/apps/ecflow/current/bin:$PATH
     
   ECF_NAME=%ECF_NAME:0% 
   ECF_PASS=%ECF_PASS:0% 
   ECF_HOST=%ECF_HOST:0%
   ECF_TRYNO=%ECF_TRYNO:0% 
   ECF_HOSTFILE=$HOME/.ecfhostfile 
   ECF_JOBOUT=%ECF_JOBOUT:0%

   export SMSNAME=$ECF_NAME # ifs requests SMSNAME to be defined for meter-event posting
   NO_SMS=1 NOSMS=1; 
   # export ECF_DEBUG_CLIENT=1
;;
esac
set +a
