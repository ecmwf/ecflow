#!/bin/ksh

PORT=3199
CLIENT="../Client/bin/gcc-4.5/debug/ecflow_client --port $PORT"
DEF=tool/test.def
SUITE=/gui

flags="force_aborted  user_edit  task_aborted  edit_failed 
       ecfcmd_failed  no_script  killed  migrated  late 
       message  queue_limit complete task_waiting  locked  zombie" 
# u(o) e(g) a e
# s xx k triangle 
# msg c xx xx xx Z

$CLIENT --delete $SUITE
$CLIENT --load $DEF
$CLIENT --begin $SUITE

# for cmd  in set_flag clear_flag; do
for cmd  in set_flag; do
for node in / $SUITE $SUITE/a $SUITE/a/b/cc; do
for flag in $flags; do
   $CLIENT --alter $cmd $flag $node
done
done
sleep 5
done
