#!/bin/bash
set -x
ECF_PORT=$(($(id -u) + 1500))
ECF_NODE=$(uname -n)
ECF_LOG=${ECF_NODE}.${ECF_PORT}.log
ECF_HOME=
USAGE="$0 -p <ecf_port> -n <ecf_node> -h <ecf_home> -l <ecf_log> -v <viewer>"
SSH="ssh"
# VIEWER="eog"; VIEWER="viewnior"; VIEWER="xv"; VIEWER=gwenview; VIEWER=ristretto
VIEWER="xdg-open"

while getopts h:l:n:p:v:? option
do
   case $option in
       h) ECF_HOME=$OPTARG;;
       l) ECF_LOG=$OPTARG;;
       n) ECF_NODE=$OPTARG;;
       p) ECF_PORT=$OPTARG;;
       v) VIEWER=$OPTARG;;
       \? | *) echo $USAGE; exit 2;;
   esac
done

client="ecflow_client --host=$ECF_NODE --port=$ECF_PORT"
which ecflow_client || module load ecflow
if [[ -f $ECF_LOG ]]; then
    ecflow_client --server_load=$ECF_LOG
elif [[ "$ECF_LOG" = /* ]]; then
    $SSH $ECF_NODE $O -p $ECF_PORT -l $ECF_LOG -n $ECF_NODE -h $ECF_HOME
elif [[ -f $ECF_HOME/$ECF_LOG ]]; then
    ecflow_client --server_load=$ECF_HOME/$ECF_LOG
elif ! `$client --ping`; then
    echo "server is not responding"
    exit 1
elif [[ $ECF_NODE != $(uname -n) ]]; then # try remote
    $SSH $ECF_NODE $O -p $ECF_PORT -l $ECF_LOG -n $ECF_NODE -h $ECF_HOME
else
    $client --server_load || $client --server_load=$ECF_LOG || $client --server_load=$ECF_HOME/$ECF_LOG 
fi

$VIEWER ${ECF_NODE}.${ECF_PORT}.png

echoxx() {
echo """
  ./ecflow_load.sh -l /tmp/emos/sms/vsms1.ecf.3.log -p 43333 -n vsms2 
  ./ecflow_load.sh -l /home/mo/moy/ecflow_server/ligarius.3320.ecf.log -p 3320 -n ligarius
./ecflow_load.sh -l /tmp/emos/sms/vsms1.ecf.1.log -p 32112 -n vsms1
./ecflow_load.sh -l /tmp/emos/sms/vsms1.ecf.2.log -p 32222 -n vsms2
./ecflow_load.sh -l /var/tmp/emos/ecflow/vali.21801.log -p 21801 -n vali
./ecflow_load.sh -l /tmp/emos/sms/vsms1.ecf.3.log -p 31415 -n vsms2
"""
}
