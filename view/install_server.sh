#!/bin/ksh

rdservers=/home/rd/rdx/ecflow/servers.config
serv=servers
cat servers.od > $serv
cat $rdservers | grep vecf | grep -v -E "^#.*" | awk '{ print $1"\t"$2"\t"$3;}' | sort >> $serv

loc=/usr/local/apps/ecflow/*/share/ecflow 
# loc=/usr/local/apps/ecflow/4.0.6/share/ecflow 
files=$serv # files=ecflowview.menu
dests="eurus ablamor opensuse113 opensuse103"
set +x
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow

if [[ "$1" = "-i" ]]; then
for dest in $dests; do
  list="$(rsh $dest ls -d $loc | grep -v current)"
  for locs in $list; do
    ssh emos@$dest mkdir -p $locs
    scp $files emos@$dest:$locs/. || echo $dest NOK
  done
done

dests="ecgb vsms1 vsms2 vsms3 lxab lxop opensuse131"
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow
for dest in $(echo $dests); do
  list="$(ssh $dest ls -d $loc | grep -v current)"
  for locs in $list; do
    ssh emos@$dest mkdir -p $locs
    scp $files emos@$dest:$locs/. || echo $dest NOK
  done
done
fi

exit 0
# ssh mordred -l emos ( for f in /var/tmp/tmpdir/emos/*; do rmdir $f; done)
