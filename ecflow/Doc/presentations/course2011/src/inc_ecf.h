export ECF_PORT=%ECF_PORT:0%; ECF_FOLLOW=%ECF_FOLLOW:NO%; set -a
case $ECF_PORT in
0 ) echo "sms is leading"
    if [[ $ECF_FOLLOW = ECF ]] ; then 
    ECF_NAME=$SMSNAME; ECF_NODE=$SMSNODE; ECF_PASS=$SMSPASS; ECF_PORT=%ECF_PORT:0%
    fi
;;
* ) echo "welcome into ecflow world"
   PATH=/usr/local/apps/current/bin:$PATH # or call "use ecflow"
   ECF_NAME=%ECF_NAME:0%; ECF_NODE=%ECF_NODE:0%; ECF_PASS=%ECF_PASS:0%
   ECF_TRYNO=%ECF_TRYNO:0% 
   ECF_HOSTFILE=$HOME/.ecfhostfile 
   ECF_JOBOUT=%ECF_JOBOUT:0%
   svr=ecflow_client; # svr=/usr/local/apps/ecflow/current/bin/ecflow_client
   export SMSNAME=$ECF_NAME # ifs requests SMSNAME to be defined for meter-event posting
   if [[ $ECF_FOLLOW != SMS ]] ; then 
     export NO_SMS=1 NOSMS=1; 
   else
     SMSNODE=$ECF_NODE SMSPASS=ECF_PASS SMS_PROG=314159
   fi
   # export ECF_DEBUG_CLIENT=1
function  smsabort { 
  $svr --abort $*
}
function  smscomplete { 
  $svr --complete 
}
function  smsevent { 
  $svr --event $*
}
function  smsinit { 
  $svr --init $* 
}
function  smslabel { 
$svr --label $* 
}
alias  smsmail="echo 'not implemented'\; exit 1"
function  smsmeter { 
  $svr --meter $*
}
function  smsmsg { 
  $svr --msg $*
}
function  smsping { 
  $svr --ping $*
}
alias  smsrand="echo 'unimplemented'\; exit 1"
alias  smsstatus="echo 'unimplemented'\; exit 1"
alias  smsvariable="echo 'unimplemented'\; exit 1"
function  smswait { 
  $svr --wait $* 
}
;;
esac
set +a

