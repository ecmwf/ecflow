#!/bin/ksh

rdservers=/home/rd/rdx/ecflow/servers.config
serv=servers
cat servers.od > $serv
cat $rdservers | grep vecf | grep -v -E "^#.*" | awk '{ print $1"\t"$2"\t"$3;}' | sort >> $serv

loc=/usr/local/apps/ecflow/*/share/ecflow 
# loc=/usr/local/apps/ecflow/4.0.6/share/ecflow 
files=$serv # files=ecflowview.menu
dests="eurus ablamor opensuse113 opensuse103"
dests=""
set +x
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow

if [[ "$1" = "-i" ]]; then
for dest in $dests; do
  list="$(rsh $dest ls -d $loc | grep -v current)"
  for locs in $list; do
    ssh emos@$dest mkdir -p $locs
    echo scp $files emos@$dest:$locs/. 
    scp $files emos@$dest:$locs/. || echo $dest NOK
  done
done

dests="ecgb vsms1 vsms2 vsms3 lxab lxop opensuse131 pikachu cpc2vb1  cpc3vb2 cpc3vb2 consolevm3 consolevm2 consolevm1 achilles cpc3vb1 cpc1vb2 cpc1vb1 cpc2vb3 cpc2vb2 cpc3vb3 "
dests=" cpc2vb1  cpc3vb2 cpc3vb2 consolevm3 consolevm2 consolevm1 achilles cpc3vb1 cpc1vb2 cpc1vb1 cpc2vb3 cpc2vb2 cpc3vb3 "
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow
for dest in $(echo $dests); do
  list="$(ssh $dest ls -d $loc | grep -v current)"
  for locs in $list; do
    ssh emos@$dest mkdir -p $locs
    echo scp $files emos@$dest:$locs/.
    ssh -l emos $dest chmod 644 $locs/servers || ssh -l deploy $dest chmod 644 $dest:$locs/servers || :
    scp $files emos@$dest:$locs/. || \
    scp $files deploy@$dest:$locs/. || echo $dest NOK
  done
done

else
  echo "use -i to install"
fi

exit 0
 cd /tmp/map/work/git/ecflow/view/
# ssh mordred -l emos ( for f in /var/tmp/tmpdir/emos/*; do rmdir $f; done)

