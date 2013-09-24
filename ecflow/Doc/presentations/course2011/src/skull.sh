#!/bin/ksh
DEBUG_DEF2ECF=1
exec 3> expanded.tmp
. ${TOP:=.}/libgen.sh

PROJECT=skull
SELECTION=$PROJECT
# set SELECTION e${PROJECT}
case $SELECTION in 
    skull ) FIRST_DAY=0;;
    eskull) FIRST_DAY=1;;
esac
function def_fam {
    defstatus suspended
}
GROUP=$(id -gn)
SCHOST=c1a
WSHOST=$(uname -n)
CLHOST=lxa
case $USER in
    map) WSQUEUE=serial
         WS_QUEUE_EPILOG=ns
         SCQUEUE=ns
         SC_QUEUE_EPILOG=ns
         ACCOUNT=ecodmdma;;
    *)   WSQUEUE=serial
         WS_QUEUE_EPILOG=ns
         SCQUEUE=ns
         SC_QUEUE_EPILOG=ns
         ACCOUNT=decmsaf;;
esac

WSLOGDIR=$SCRATCH/output
mkdir -p $WSLOGDIR/$SELECTION # call shell command
SCLOGDIR=/s1a/ms_dir/$GROUP/$USER/smsjoboutput
WDIR=/s1a/ms_dir/$GROUP/$USER/wdir
VERSION="0040"
# CONV: set FIRST_DATE `date +%Y%m%d` # date is a keyword for play
FIRST_DATE=`\date +%Y%m%d`
LAST_DATE=$(newdate -D $FIRST_DATE 1)
QUEUE_EPILOG=ns
onws() {
    edit QUEUE   $WSQUEUE
    edit QUEUE_EPILOG $WS_QUEUE_EPILOG
    edit LOGDIR  $WSLOGDIR
    edit SMSOUT  $WSLOGDIR
    edit WSHOST  $HOST
}
oncl() {
    edit QUEUE   $WSQUEUE
    edit QUEUE_EPILOG $WS_QUEUE_EPILOG
    edit LOGDIR  $WSLOGDIR
    edit SMSOUT  $WSLOGDIR
    edit WSHOST  linux_cluster
    edit WSHOST  $CLHOST
}
onsc() {
    edit QUEUE   $SCQUEUE
    edit QUEUE_EPILOG $SC_QUEUE_EPILOG
    edit LOGDIR  $SCLOGDIR
    edit SMSOUT  $SCLOGDIR
    edit WSHOST  $SCHOST
}
onecg() {
    edit QUEUE   normal
    edit QUEUE_EPILOG normal
    edit LOGDIR  $WSLOGDIR
    edit SMSOUT  $WSLOGDIR
    edit WSHOST ecgate
}
day() {
# edit WEEKDAY $*; label weekday $* # CONV
    edit WEEKDAY "\"$*\""
    label weekday "$*"
}
#
# always better to include external files
# out from the suite definition
#
. inc_${PROJECT}.sh
# < inc_skull.def
. dinner.sh
. barber.sh
# . weekly.sh
USE_SMS=1
USE_ECF=1
if (( !$USE_SMS && !$USE_ECF )); then
    echo "it cannot be"
    abort
fi
suite $SELECTION
defstatus suspended
edit USER             $USER
edit SCHOST           $SCHOST
edit WSHOST           $WSHOST
edit CLHOST           $CLHOST
edit WSLOGDIR         $WSLOGDIR
edit SCLOGDIR         $SCLOGDIR
edit DATEMASK         *.*.*
edit QUEUE_EPILOG     $QUEUE_EPILOG
# setenv -i PWD
HOMEDIR=$HOME/sms_server/$SELECTION
HOMEDIR=$PWD
if [[ -n USE_SMS ]]; then
    edit SMSLOGHOST       ${SCHOST}
    edit SMSLOGPORT       9316
    edit SMSTRIES         1
    edit SMSHOME          $SCRATCH/output
    edit SMSFILES         $HOMEDIR/smsfiles
    edit SMSINCLUDE       $HOMEDIR/include
    edit SMSCMD  "\"/home/ma/emos/bin/smssubmit %USER% %WSHOST% %SMSJOB% %SMSJOBOUT%\""
    edit SMSKILL "\"/home/ma/emos/bin/smskill %USER% %WSHOST% %SMSRID% %SMSJOBOUT%\""
fi
if [[ -n USE_ECF ]]; then
    edit ECF_LOGHOST       ${SCHOST}
    edit ECF_LOGPORT       9316
    edit ECF_TRIES         1
    edit ECF_HOME          $SCRATCH/output
    edit ECF_FILES         $HOMEDIR/smsfiles
    edit ECF_INCLUDE       $HOMEDIR/include
    edit ECF_JOB_CMD  "\"/home/ma/emos/bin/smssubmit %USER% %WSHOST% %ECF_JOB% %ECF_JOBOUT% \""
    edit ECF_KILL_CMD "\"/home/ma/emos/bin/smskill %USER% %WSHOST% %ECF_RID% %ECF_JOBOUT% \""
fi
edit ACCOUNT          $ACCOUNT
edit VERSION          $VERSION
edit EPSVERSION       0001
edit RFVERSION        0001
edit DELTA_DAY        0
edit EMOS_BASE        00
edit USE_YMD          true
edit FSFAMILY         / # $HOME/
edit BINS             $HOME/bin
edit XBINS            /home/ma/emos/bin
edit STREAM           undef
edit EPSTYPE          undef
edit MEMBER           0
edit FIRST_DAY        $FIRST_DAY
edit EMOS_TIME_STEP_H 00
# repeat date YMD    $FIRST_DATE $LAST_DATE
edit YMD    $FIRST_DATE
edit   SUITE_START $FIRST_DATE
family limits
defstatus complete
limit lim 3
endfamily
# generate binaries
family make 
complete /$SELECTION:YMD ne /$SELECTION:SUITE_START
family wst
onws
call_skull_make
endfamily
family ecg
onecg
call_skull_make
endfamily
family hpc
onsc
call_skull_make
endfamily
family lcl
oncl
if [[ -n USE_SMS ]]; then
    edit SMSCMD "\"/home/ma/emos/bin/smssubmit.new %USER% %WSHOST% %SMSJOB% %SMSJOBOUT% \""
fi
if [[ -n USE_ECF ]]; then
    edit ECF_JOB_CMD  "\"/home/ma/emos/bin/smssubmit.new %USER% %WSHOST% %ECF_JOB% %ECF_JOBOUT% \""
fi
call_skull_make
endfamily
endfamily

family $PROJECT
  def_fam
  families="00 12"

  edit ENSEMBLES 50
  trigger "(./make==complete)"
  for fam in  $families ; do
    family $fam # onsc
    if (( $fam == 00 )); then
	edit DELTA_DAY 1
    else
	edit DELTA_DAY 0
    fi
    edit EMOS_BASE $fam

    call_skull

    family pop;    oncl
    trigger "(./${PROJECT}==complete)"
    call_pop_skull
    endfamily

    family ws;    onws
    trigger "(./${PROJECT}==complete)"
    call_pop_skull
    endfamily

    endfamily #fam
done

endfamily
family consumer
def_fam
. consumer.sh
endfamily
onws
edit SLEEP 20
call_dinner
call_barber_shop
# call_weekly
task perl
defstatus suspended
if [[ -n USE_SMS ]]; then
    edit SMSMICRO ^
    edit SMSCMD "\"^SMSJOB^ > ^SMSJOBOUT^ 2>&1 &\""
fi
if [[ -n USE_ECF ]]; then
    edit ECF_MICRO ^
    edit ECF_JOB_CMD "\"^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1 &\""
fi
meter step -1 100
event ev
label into "none"
task python
defstatus suspended
if [[ -n USE_SMS ]]; then
    edit SMSMICRO ^
    edit SMSCMD "\"^SMSJOB^ > ^SMSJOBOUT^ 2>&1 &\""
fi
if [[ -n USE_ECF ]]; then
    edit ECF_MICRO ^
    edit ECF_JOB_CMD "\"^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1 &\""
fi
meter step -1 100
event ev
label into "none"
endsuite
exit 0
