#!/bin/ksh

rdservers=/home/rd/rdx/ecflow/servers.config
serv=servers
cat servers.od > $serv
cat $rdservers | grep vecf | grep -v -E "^#.*" | awk '{ print $1"\t"$2"\t"$3;}' | sort >> $serv

loc=/usr/local/apps/ecflow/*/share/ecflow 
files=$serv
dests="ibis ablamor opensuse113 opensuse103"
set +x
for dest in $dests; do
  for locs in $(rsh $dest ls -d $loc | grep -v current); do
    rcp $files emos@$dest:$locs/. || echo $dest NOK
  done
done

dests="ecgb vsms1 vsms2 vsms3 lxab lxop opensuse131"
for dest in $(echo $dests); do
  for locs in $(ssh $dest ls -d $loc | grep -v current); do
    scp $files emos@$dest:$locs/. || echo $dest NOK
  done
done