#!/bin/bash
## Copyright 2009-2019 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 
# =====================================================================
# Show server load
# =====================================================================
set -x
ECF_PORT=$(($(id -u) + 1500))
ECF_HOST=$(uname -n)
ECF_LOG=${ECF_HOST}.${ECF_PORT}.log
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
       n) ECF_HOST=$OPTARG;;
       p) ECF_PORT=$OPTARG;;
       v) VIEWER=$OPTARG;;
       \? | *) echo $USAGE; exit 2;;
   esac
done

client="ecflow_client --host=$ECF_HOST --port=$ECF_PORT"
which ecflow_client || module load ecflow
if [[ -f $ECF_LOG ]]; then
    ecflow_client --server_load=$ECF_LOG
elif [[ "$ECF_LOG" = /* ]]; then
    $SSH $ECF_HOST $O -p $ECF_PORT -l $ECF_LOG -n $ECF_HOST -h $ECF_HOME
elif [[ -f $ECF_HOME/$ECF_LOG ]]; then
    ecflow_client --server_load=$ECF_HOME/$ECF_LOG
elif ! `$client --ping`; then
    echo "server is not responding"
    exit 1
elif [[ $ECF_HOST != $(uname -n) ]]; then # try remote
    $SSH $ECF_HOST $O -p $ECF_PORT -l $ECF_LOG -n $ECF_HOST -h $ECF_HOME
else
    $client --server_load || $client --server_load=$ECF_LOG || $client --server_load=$ECF_HOME/$ECF_LOG 
fi

$VIEWER ${ECF_HOST}.${ECF_PORT}.png

echoxx() {
echo """
"""
}
