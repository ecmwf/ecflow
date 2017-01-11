#!/bin/bash
exec 3> /dev/stdout
commands="   autocancel   
clock        complete     cron         date         day          defstatus    
edit         endfamily    endsuite     endtask      event        extern       
family       inlimit      label        late         limit        meter        
repeat       suite        task              time    today        trigger      "

# enable -n time || : # shell builtin removed
for fname in $commands; do
source /dev/stdin <<EOF
function $fname()
{
  echo $fname "\${*}" >&3
}
EOF
done

alias time="echo time \"\${*}\" >&3"
init="ECF_NAME=%ECF_NAME% ECF_PASS=%ECF_PASS% ecflow_client --init;"
complete="ECF_NAME=%ECF_NAME% ECF_PASS=%ECF_PASS% ecflow_client --complete;"

ensemble() {
num=0
tot=10
family ensemble
limit  lim 5
inlimit ensemble:lim
while (( num <= tot)); do 
  family $(printf "%02d" $num)
    task model; edit MEMBER $num; (( num += 1))
  endfamily
done
endfamily

family process
  for param in "z" "u" "v" "t" "q"; do 
    task $param; edit PARAM $param; 
  done
endfamily # process

family seq
  for num in $(seq 1 9); do
    family $(printf "%02d" $num)
      task model; edit MEMBER $num; (( num += 1))
    endfamily  
  done
endfamily
}

producer() {
    family producer
      repeat date YMD 20160101 20321212
      family produce
        edit PRODUCE 1
        edit CONSUME 0
        task generic
	  event p
	  event c
	  meter step -1 100 90
      endfamily # produce

      family consume
        edit PRODUCE 0
        complete produce:CONSUME eq 1
        trigger produce/generic:p or produce==complete
	task generic
      endfamily # consume

      family consume2
        repeat integer STEP 0 100
        edit PRODUCE 0
        complete produce:CONSUME eq 1
        trigger produce/generic:step gt consume2:STEP or produce==complete
	task generic
      endfamily # consume2
    endfamily # producer
}

test_suite() {
extern /limits:tasks

suite $SUITE_NAME
  defstatus suspended
  autocancel +10
  clock real

  family limits
    limit tasks 10
     defstatus complete
  endfamily
  inlimit /limits:tasks

  producer # function defined upper

  ensemble

  task cron
    cron 00:00 23:59 01:00
    late -s 00:05 -c 00:10

  task day
    day monday
  
  family fam
    time 12:00
    date "1.*.*"
    task t1
      event 1
      meter step -1 100 90
      label info nop
}

usage() {
echo <<EOF
$0 -p <ECF_PORT> -n <ECF_HOST> -s <suite_name> -r <path>
   -l: load
   -r: replace path
   -t: test suite generated
EOF
}

LOAD=0
REPL=0

while getopts dln:p:r:s:t option
do
case $option in
    d) DEBUG=1; set -eux;;
    l) LOAD=1;;
    n) ECF_HOST=$OPTARG;; # NODE HOST
    p) ECF_PORT=$OPTARG;; # PORT
    r) REPL=$OPTARG;;
    s) SUITE_NAME=$OPTARG;;
    t) TEST=1;;
    ?|*) usage; exit 2 ;;
esac
done

sdef=${SUITE_NAME:=test}.exp
exec 3> $sdef
if [[ $TEST == 1 ]]; then
 test_suite
else
  echo "#ERR: insert your suite definition HERE $0 $LINENO"
  exit 2
fi

CLIENT="ecflow_client --port ${ECF_PORT:=31415} --host ${ECF_HOST:=localhost}"
if [[ $REPL != 0 ]]; then
  $CLIENT --replace $REPL $sdef
  echo "#MSG: node $REPL was replaced as defined in $sdef" \
       " on ${ECF_HOST:=localhost} ${ECF_PORT:=31415}"
elif [[ $LOAD != 0 ]]; then
  $CLIENT --load $sdef
else
  cat $sdef
  echo "#MSG: suite was created in file $sdef"
fi
