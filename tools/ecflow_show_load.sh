#!/bin/bash
USAGE="$0 -n <ecf_node> -p <ecf_port> -h <ecf_home> -l <ecf_log>
  -r [retrieve_log_file] 
  -z [run remotely, and retrieve plot file] 
  -g [debug]" 
OPT="[-author map]"
OPT="[-copyright?Copyright ECMWF]"
OPT="[+NAME?ecflow_show_load.sh]"
OPT="[+DESCRIPTION?retrieve server log file, call client server_load, and display load]"
OPT="[g][n:ecf_node][h:ecf_home][p:ecf_port][?][l:ecf_log][r][z]"
# OPT=":n:p:h:?"
ECF_PORT=$((1500 + $(id -u)))
ECF_NODE=$(uname -n) # ssh may not like localhost
ECF_HOME=
ECF_LOG=$ECF_NODE.$ECF_PORT.log
PNG=$ECF_NODE.$ECF_PORT.png
RETRIEVE=0
DEBUG=0
REMOTE=0
LOCALH=$(uname -n)

while getopts "$OPT" options
do
   [[ $DEBUG == 1 ]] && echo "options:" $options ${OPTARG:-}
  case $options in
  g) DEBUG=1; set -eux;;
  p) ECF_PORT=$OPTARG;;
  n) ECF_NODE=$OPTARG;;
  h) ECF_HOME=$OPTARG;;
  l) ECF_LOG=$OPTARG;;
  r) RETRIEVE=1;;
  z) REMOTE=1;;
  \?) echo $USAGE; exit 2;;
  * ) echo $USAGE; exit 2;;
  esac
done

which ecflow_client > /dev/null || module load ecflow
client=$(which ecflow_client)
test=/tmp/map/work/git/cmake_build_dir/ecflow/debug/bin/ecflow_client && \
  [[ -f $test ]] && client=$test
client="$client --host $ECF_NODE --port $ECF_PORT --server_load"

case $ECF_LOG in
./* ) if [[ ! -f $ECF_LOG ]]; then 
    echo "#WAR local file not found"; 
    exit 1; 
  fi
;;
/*  ) if [[ ! -f $ECF_LOG ]] && [[ $DEBUG == 1 ]]; then echo "#WAR absolute path"; fi
;;
*)    [[ -n "$ECF_HOME" ]] && ECF_LOG=$ECF_HOME/$ECF_LOG
;;
esac
echo "#MSG: ECF_LOG is $ECF_LOG "

rm -f $PNG || :
if [[ 1 == $REMOTE ]]; then
  ssh $ECF_NODE -l $USER ($client && scp $ECF_NODE.$ECF_PORT.png $USER:$LOCALH:$PNG)
  # ssh -l $USER $ECF_NODE $client; scp $USER@$ECF_NODE:$ECF_NODE.$ECF_PORT.png .
  [[ -f $PNG ]] && ${EOG:-eog} $PNG || echo "#ERR: could not display $PNG"
  exit 0

elif [[ 1 == $RETRIEVE ]]; then
  TMPDIR=/tmp/$USER
  mkdir -p $TMPDIR || :
  TMPLOG=$TMPDIR/$ECF_NODE.$ECF_PORT.log
  # NO: avoid scp log-file, it is better is issue the command remotely and retrieve the plot
  # scp $USER@$ECF_NODE:$ECF_LOG $TMPLOG || rcp $USER@$ECF_NODE:$ECF_LOG $TMPLOG 

  ECF_LOG=$TMPLOG
fi

if [[ -f $ECF_LOG ]]; then 
  echo "#MSG: direct access # $client $ECF_LOG"
  $client $ECF_LOG

else
  ssh $ECF_NODE -l $USER ($client && scp $ECF_NODE.$ECF_PORT.png $USER:$LOCALH:$(pwd))
  # leave remote png behind? clean?
fi

[[ -f $PNG ]] && ${EOG:-eog} $PNG || echo "#ERR: could not display $PNG"

exit 0
load=./ecflow_show_load.sh
$load -p $ECF_PORT -n $ECF_NODE -h $TMPDIR/. -l ${ECF_NODE}.ecf.${ECF_PORT}.log -z
