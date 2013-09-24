#!/bin/ksh

if [[ $# -ge 2 ]] ; then
    echo logsvr.sh port
    exit -1
fi

echo "logsvr pid $$"

HOST=`hostname | cut -c 1-4`

# LOGPORT=${1:-9318}
LOGPORT=${1:-9316}
USERS="$USER"

if [[ $HOST = hpc* ]] ; then
  LOGPATH=/$HOST/emos_dir:/emos_esuite:/emos_dir
  LOGMAP=/emos_esuite:/emos_esuite:/vol/emos/output:/emos_esuite:/vol/emos/output:/$MACHINE_NAME/emos_dir:/$MACHINE_NAME/emos_dir:/$MACHINE_NAME/emos_dir:/emos_esuite:/emos_esuite:/vol/emos/output:/emos_dir:/emos_dir:/emos_dir
  log=/$HOST/tmp/ma/emos/logsvr.log
else
# HP
  LOGPATH=/tmp/output:/pp2/log:/acq2/log:/eacq1/log:/eacq2/log
  LOGMAP=/pp2/log:/pp2/log:/acq2/log:/acq2/log:/eacq1/log:/eacq1/log:/eacq2/log:/eacq2/log
  log=/sms/logsvr.log
fi

if [[ -x /usr/local/lib/metaps/perl/logsvr.pl ]] ; then
    LOGSVR=/usr/local/lib/metaps/perl/logsvr.pl
elif [[ -x /usr/local/apps/sms/bin/logsvr.pl ]] ; then
    LOGSVR=/usr/local/apps/sms/bin/logsvr.pl
elif [[ -x $HOME/bin/logsvr.pl ]] ; then
    LOGSVR=$HOME/bin/logsvr.pl
elif [[ -x /sms/bin/logsvr.pl ]] ; then
  LOGSVR=/sms/bin/logsvr.pl
elif [[ -x ./logsvr.pl ]] ; then
  LOGSVR=`pwd`/logsvr.pl
else
    echo "logsvr.pl not found in expected location"
fi

echo "using: $LOGSVR"
export LOGPORT LOGPATH LOGMAP
n=0
while [[ $n -lt 1 ]]
do
	$LOGSVR > $log 2>&1 &
        echo "logsvr pid $!"
        wait
	tail $log | mail -s "logsvr.pl crashed" $USERS
        sleep 10
done
