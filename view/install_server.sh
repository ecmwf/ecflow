#!/bin/ksh
# set -eux
# set -ux
arg=$1
serv=servers
user=emos
RSH="ssh -o StrictHostKeyChecking=no -l $user "
SSH="ssh -o StrictHostKeyChecking=no -l $user"
SCP=scp

case $arg in
-h | -\?) 
USAGE="$0
  [-g] generate servers file locally
  [-i] install
  [-h] help,usage
"
echo $USAGE
exit 2;; 

-g) rdservers=/home/rd/rdx/ecflow/servers.config
cat > $serv <<EOF
1_od      vsms1 32112
1_sappa   sappa  3141
1_sappb   sappb  3141
2_bceps_mumo_tc ecgate 4409
2_harmonie_tc     ecgate   5573
3_od2     vsms2 32222
3_od4     vsms2 45555
3_dataset_public vali 21801
3_dataset_p vsms2     21801
5_od3     vsms2 43333
5_webapps ecflow-minos 3813
9_ode     vsms3 31415
rdcx          vsmsrdcx                314112
EOF

# cat servers.od > $serv
cat $rdservers | grep vecf | grep -v -E "^#.*" \
  | awk '{ print $1"\t"$2"\t"$3;}'\
  | sort >> $serv

;; -i) 
loc=/usr/local/apps/ecflow/*/share/ecflow 
# loc=/usr/local/apps/ecflow/4.0.6/share/ecflow 
files="$serv" # files=ecflowview.menu
dests="eurus ablamor opensuse113 opensuse103"
# dests=""
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow

if [[ 1 == 0 ]]; then
for dest in $dests; do
  list="$($RSH $dest ls -d $loc | grep -v current)"
  for locs in $list; do
    ruser=$($SSH $dest ls -la $locs/servers | awk '{print $3}' || echo emos)
    $SSH $dest "test -d $locs || mkdir -p $locs ; chmod 755 $locs; chmod 644 $locs/servers"
    CMD="$SCP $files $ruser@$dest:$locs/. "
    $CMD || echo -e "# $dest $locs NOK\n# $CMD"
  done
done
fi

opers=" cpc2vb1  cpc3vb2 cpc3vb2 consolevm3 consolevm2 consolevm1 achilles cpc3vb1 cpc1vb2 cpc1vb1 cpc2vb3 cpc2vb2 cpc3vb3 "
dests="ecgb vsms1 vsms2 vsms3 lxab lxop opensuse131 pikachu $opers"
set -
# list=/usr/local/apps/ecflow/4.0.4/share/ecflow
for dest in $(echo $dests); do
  list="$($SSH $dest ls -d $loc | grep -v current)"
  echo "########### $dest"
  for locs in $list; do
    ruser=$($SSH $dest ls -la $locs/servers | awk '{print $3}' || echo emos)
    $SSH $ruser@$dest "mkdir -p $locs; chmod 755 $locs; chmod 644 $locs/servers"
    CMD="$SCP $files $ruser@$dest:$locs/. "
    $CMD || echo -e "# $dest $locs NOK\n# $CMD"
  done
done

exit 0
cd /tmp/map/work/git/ecflow/view/
# ssh mordred -l emos ( for f in /var/tmp/tmpdir/emos/*; do rmdir $f; done)

;; esac
