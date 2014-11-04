#!/bin/ksh

rdservers=/home/rd/rdx/ecflow/servers.config
serv=servers
cat servers.od > $serv
cat $rdservers | grep vecf | grep -v -E "^#.*" | awk '{ print $1"\t"$2"\t"$3;}' | sort >> $serv

loc=/usr/local/apps/ecflow/*/share/ecflow 
files=$serv
# files=ecflowview.menu
dests="ibis ablamor opensuse113 opensuse103"
set +x
list="$(rsh $dest ls -d $loc | grep -v current)"
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow
for dest in $dests; do
  for locs in $list; do
    rcp $files emos@$dest:$locs/. || echo $dest NOK
  done
done

dests="ecgb vsms1 vsms2 vsms3 lxab lxop opensuse131"
list="$(ssh $dest ls -d $loc | grep -v current)"
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow
for dest in $(echo $dests); do
  for locs in $list; do
    scp $files emos@$dest:$locs/. || echo $dest NOK
  done
done
