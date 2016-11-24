#!/usr/bin/env python
# -*- coding= UTF-8 -*-
from __future__ import with_statement
import sys, os, pwd, getopt
sys.path.append('/home/ma/emos/def/o/def')
import inc_emos as ic
from ecf import *
import ecf
from inc_emos import Family, Task, Variables, Time, Meter, Event, Label, Late
import sms2ecf

# ecf.USE_TRIGGER = 0 # DEBUG MODE: do not load triggers

def get_username():
    return pwd.getpwuid( os.getuid() )[ 0 ]
def get_uid():
    return pwd.getpwnam(get_username()).pw_uid

def create_wrapper(name, content):
    print "#MSG: creating file %s/" % wdir + name
    wrapper = open(wdir + "/%s" % name, 'w')
    print >>wrapper, content
    wrapper.close()    
##############################################

# ic.SUBM = "/home/ma/emos/bin/smssubmit.cray"
# ic.KILL = "/home/ma/emos/bin/smssubmit.cray"
# ic.STAT = "/home/ma/emos/bin/smssubmit.cray"
# sms2ecf.subm = "/home/ma/emos/bin/smssubmit.cray"
# sms2ecf.kill = "/home/ma/emos/bin/smssubmit.cray"
# sms2ecf.stat = "/home/ma/emos/bin/smssubmit.cray"

def create_task():
    ### PRODUCER/CONSUMER TASK
    produce = '''#!/bin/ksh
%manual
  start - check - stop logsvr
%end
%include <qsub.h>
%include <trap.h>

step=%BEG:0%
while [ $step -le %FIN:48% ]; do
  if [[ %PRODUCE:1% = yes ]]; then
    xevent p
  elif [[ %CALL_WAITER:0% != 0 ]]; then
case %ECF_PROG:0% in 
0) smswait %TRIGGER:1==1%;;
*) ecflow_client --wait "%TRIGGER:1==1%";;
esac
  fi

  if [[ %CONSUME:no% = yes ]]; then
    xevent c
  fi

  xmeter step $step
  step=$((step + %BY:1%))
done
 
%include <endt.h>
'''
    create_wrapper("produce.sms", produce)

    create_wrapper("consume.sms", produce)
    ### PURE PYTHON

    head = """import os, time, signal
import ecflow
  
print "PYTHONPATH====================================================="
print os.environ['PYTHONPATH'].split(os.pathsep)
 
class Client(object):
    ''' communication with the ecflow server. This will automatically call
       the child command init()/complete(), for job start/finish. It will also
       handle exceptions and signals, by calling the abort child command.
       *ONLY* one instance of this class, should be used. Otherwise zombies will be created.
    '''
    def __init__(self):
      print "Creating Client"
      self.ci = ecflow.Client()
      self.ci.set_host_port("$ECF_HOST$","$ECF_PORT$")
      self.ci.set_child_pid(os.getpid())
      self.ci.set_child_path("$ECF_NAME$")
      self.ci.set_child_password("$ECF_PASS$")
      self.ci.set_child_try_no($ECF_TRYNO$)
    
      print "Only wait 20 seconds, if the server cannot be contacted (note default is 24 hours) before failing"
      self.ci.set_child_timeout(20)
     
      # Abort the task for the following signals
      signal.signal(signal.SIGINT,  self.signal_handler)
      signal.signal(signal.SIGHUP,  self.signal_handler)
      signal.signal(signal.SIGQUIT, self.signal_handler)
      signal.signal(signal.SIGILL,  self.signal_handler)
      signal.signal(signal.SIGTRAP, self.signal_handler)
      signal.signal(signal.SIGIOT,  self.signal_handler)
      signal.signal(signal.SIGBUS,  self.signal_handler)
      signal.signal(signal.SIGFPE,  self.signal_handler)
      signal.signal(signal.SIGUSR1, self.signal_handler)
      signal.signal(signal.SIGUSR2, self.signal_handler)
      signal.signal(signal.SIGPIPE, self.signal_handler)
      signal.signal(signal.SIGTERM, self.signal_handler)
      signal.signal(signal.SIGXCPU, self.signal_handler)
      signal.signal(signal.SIGPWR,  self.signal_handler)
     
    def signal_handler(self,signum, frame):
      print 'Aborting: Signal handler called with signal ', signum
      self.ci.child_abort("Signal handler called with signal " + str(signum));
     
    def __enter__(self):
      self.ci.child_init()
      return self.ci
     
    def __exit__(self,ex_type,value,tb):
      print "Client:__exit__: ex_type:" + str(ex_type) + " value:" + str(value) + "\\\n" + str(tb)
      if ex_type != None:
         self.ci.child_abort("Aborted with exception type " + str(ex_type) + ":" + str(value))
         return False
      self.ci.child_complete()
      return False
"""
    body = """#!/usr/bin/env python
$include <head.py>
 
if __name__ == "__main__":
# This will also handle call to sys.exit(), i.e Client.__exit__ will still be called.
   with Client() as ci:
   # *******************************************************************************
   # This is where the main work is done.
   # *******************************************************************************
      for i in range(1,100):
         ci.child_meter('step',i)
         ci.child_label('info', "value_" + str(i))
         time.sleep(1)
       
      ci.child_event('1')
      print "Finished event,meter and label child commands"
 
$manual
   This is the manual section. Instead of calling python from the ECF_JOB_CMD we could alternatively place,
       #!/bin/env/python
   on the first line of this file.
$end
 
$comment
   Note: We do not need a include a tail.py, the head.py does it all.
$end

"""
    create_wrapper("head.py", head)
    create_wrapper("pure_python.sms", body)
    ### PYTHON TASK
    content = """$include <python_header.h>
# header files are located in the same directory as wrapper (quotes)
for step in range(0,101):
    print step
    xmeter("step", step)
else:
    print 'the loop is over'
xevent("1")
xlabel("info", "news from pure python world")

$include <python_endt.h>
"""
    create_wrapper("python.sms", content)

    content = """#!/usr/bin/env python
import os
import sys
import signal 

ECF_PORT=$ECF_PORT:0$
XECF="/usr/local/apps/ecflow/current/bin/ecflow_client ";
# --port=$ECF_PORT:0$ --host=$ECF_HOST:0$ ";
def SigHandler(signum, frame):
   print "caught signal " + signum
   xabort()
   sys.exit(0);
   return

print ECF_PORT
# set -eux ?
# trap 0 ?
# time stamp per executed line

import atexit
early_exit = True
@atexit.register
def goodbye():
  if early_exit: 
    print "too early"
    xabort()
  else: 
    xcomplete()

# TIME_STAMP
# http://shop.oreilly.com/product/9780596007973.do # recipie p436
import syslog, time

class FunctionFileLikeWrapper():
    def __init__(self, func): self.func = func
    def write(self, msg): self.func(msg)
    def flush(self): pass

class TimeStamper(object):
    msg_format = "[%y%m%d %H:%M:%S]", time.gmtime, "%s: %s"
    msg_format = "+ %H:%M:%S", time.gmtime, "%s %s"

    def __call__(self, msg):
        tfmt, tfun, gfmt = self.msg_format
        return "%s %s\\\n" % (time.strftime(tfmt, tfun()), msg)

class TeeFileLikeWrapper():
    def __init__(self, *files): self.files = files
    def write(self, msg): 
        for f in self.files: f.write(timestamp(msg.strip()))

class FlushingWrapper:
    def __init__(self, *files): self.files = files
    def write(self, msg):
        for f in self.files:
            f.write(timestamp(msg))
            # f.write(timestamp(msg.strip()))
            f.flush()

def logto(*files):
    # sys.stdout = TeeFileLikeWrapper(*files)
    sys.stdout = FlushingWrapper(*files)

syslogger = syslog.syslog
syslogfile = FunctionFileLikeWrapper(syslogger)
timestamp = TimeStamper()
logto(sys.stdout, syslogfile, open("log.tmp", "w"))
# end time stamp

if ECF_PORT > 0:
  os.environ['ECF_PORT'] = "$ECF_PORT:0$"
  os.environ['ECF_NAME'] = "$ECF_NAME:0$"
  os.environ['ECF_HOST'] = "$ECF_HOST:0$"
  os.environ['ECF_PASS'] = "$ECF_PASS:0$"

  def xinit():
    os.system(XECF + " --init=" + str(os.getpid()))  
    print "init"
  def xabort():
    os.system(XECF + " --abort")  
  def xcomplete():
    os.system(XECF + " --complete")  
  def xmeter(name, step):
    os.system(XECF + " --meter=" + name + " " + str(step))  
  def xevent(name):
    os.system(XECF + " --event=" + name)  
  def xlabel(name, msg):
    os.system(XECF + " --label=" + name + " '%s'" % msg)  
else:
  os.environ['SMS_PROG'] = "$SMS_PROG:0$"
  os.environ['SMSNAME'] = "$SMSNAME:0$"
  os.environ['SMSNODE'] = "$SMSNODE:0$"
  os.environ['SMSPASS'] = "$SMSPASS:0$"

  def xinit():
    os.system('smsinit ' + str(os.getpid()))
  def xabort():
    os.system('smsabort')
  def xcomplete():
    os.system('smscomplete')
  def xmeter(name, step):
    os.system("smsmeter " + name + " " + str(step))
  def xevent(name):
    os.system("smsevent " + name)
  def xlabel(name, msg):
    os.system("smslabel " + name + " '%s'" % msg)

signal.signal (signal.SIGHUP,  SigHandler)
signal.signal (signal.SIGINT,  SigHandler)
signal.signal (signal.SIGQUIT, SigHandler)
signal.signal (signal.SIGILL,  SigHandler)
signal.signal (signal.SIGTRAP, SigHandler)
signal.signal (signal.SIGIOT,  SigHandler)
signal.signal (signal.SIGBUS,  SigHandler)
signal.signal (signal.SIGFPE,  SigHandler)

"""
    create_wrapper("python_header.h", content)

    content = """# os.system('smscomplete')
early_exit = False
# xcomplete() # managed with atexit
# os.system('/usr/local/apps/sms/bin/ecflow/bin/ecf_client --complete')
"""
    create_wrapper("python_endt.h", content)

    ### PERL TASK
    create_wrapper("perl.sms", """#!/usr/bin/perl -w
^include <perl_header.h>
# header files are located in the same directory as wrapper (quotes)
print "Pure perl SMS task";
for ( my $step=1; $step <= 100 ; $step++ ) {
   print "this is the number $step\\\n";
   xmeter("step", $step);
}
xevent("1");
            
xlabel("info", "news from pure perl world");      
^include <perl_endt.h>
""")
    # create_wrapper("perl.sms", content)

    source = """use strict;

my $xmeter = "smsmeter"; my $arg_m = "";
my $xlabel = "smslabel"; my $arg_l = "";
my $xevent = "smsevent"; my $arg_e = "";
my $xcomplete = "smscomplete"; my $arg_c = "";
my $xabort = "smsabort";

if (^ECF_PORT:0^ != 0) {
$ENV{'ECF_PORT'}  = "^ECF_PORT:0^" ;  # ecFlow port number
$ENV{'ECF_HOST'}  = "^ECF_HOST:0^"  ; # ecFlow host
$ENV{'ECF_NAME'}  = "^ECF_NAME:0^"  ; # task path into the suite
$ENV{'ECF_PASS'}  = "^ECF_PASS:0^"  ; # password for the job
$ENV{'ECF_TRYNO'} = "^ECF_TRYNO:0^" ; # job occurence number
my $client = "/usr/local/apps/ecflow/current/bin/ecflow_client";
$xmeter = $client; $arg_m = "--meter";
$xlabel = $client; $arg_l = "--label";
$xevent = $client; $arg_e = "--event";
$xcomplete = $client; $arg_c = "--complete";
$xabort = $client; 

system($client, "--init", "$$");
} else {
  $ENV{'SMS_PROG'} = "^SMS_PROG:0^" ; # SMS Program Number
  $ENV{'SMSNODE'}  = "^SMSNODE:0^"  ; # SMS host
  $ENV{'SMSNAME'}  = "^SMSNAME:0^"  ; # task path into the suite
  $ENV{'SMSPASS'}  = "^SMSPASS:0^"  ; # password for the job occurence
  $ENV{'SMSTRYNO'} = "^SMSTRYNO:0^" ; # job occurence number
}

sub xmeter($$){ my ($name, $step) = @_; 
  system($xmeter, $arg_m, $name, $step); }
sub xevent($){ my ($name) = @_; 
  system($xevent, $arg_e, $name); }
sub xlabel($$){ my ($name, $msg) = @_; 
  system($xlabel, $arg_l, $name, $msg); }
sub xabort(){ system($xabort); }
sub xcomplete(){ system($xcomplete, $arg_c, "$$"); }

print "start";
eval '
"""
    create_wrapper("perl_header.h", source)

    source="""';
if ($@){
    print "caught signal: $@\n";
    xabort();
    exit;
  }
print "the job is now complete\n";
xcomplete();
exit;"""
    create_wrapper("perl_endt.h", source)

    ### LOGER TASK
    content = """#!/bin/ksh
%manual
  start - check - stop logsvr
%end
%include <qsub.h>
%include <trap.h>
SLEEP=%SLEEP:0%
KIND=%KIND:start%
type rsh && RSH=rsh || RSH=ssh

case $HOST in 
c2a*) rhost=c2a;;
c2b*) rhost=c2b;;
xxcct*) rhost=cctdtn1 ; RSH=ssh;;
cct*) rhost=cct-log ; RSH=ssh;;
cca*) rhost=cca-log ; RSH=ssh ;;
xxcca*) rhost=cca-il2 ; RSH=ssh ;;
xxccb*) rhost=ccb-il2 ; RSH=ssh ;;
ccb*) rhost=ccb-log ; RSH=ssh ;;
esac

case $KIND in
start)

ps -elf | grep logsvr | grep -v grep | grep $USER && \\\
ps -elf | grep logsvr | grep $USER | grep -v grep | \\\
awk '{print $1}' | xargs kill -9

case $ARCH in
cray)  
  case $HOST in
  cct*) logsvr=/usr/local/apps/sms/bin/logsvr.sh
        # /usr/local/apps/emos/bin/logsvr.sh
        # $RSH $rhost 
        nohup $logsvr > /tmp/emos_logsvr.tmp 2>&1 &
        rhost=cct # start on both nodes cctdtn1  + cct (Avi serial jobs)
        trap 0; xcomplete; exit 0
  ;;
  cca*|ccb*) logsvr=/usr/local/apps/emos/bin/logsvr.sh;;
  *)    exit 1;;
  esac

  $RSH $rhost nohup $logsvr > /tmp/emos_logsvr.tmp 2>&1 &
;;
ibm*) $RSH $rhost nohup $HOME/bin/boot/logsvr.sh > /tmp/emos_logsvr.tmp 2>&1 &
;;
*)    nohup logsvr.sh > /tmp/emos_logsvr.tmp 2>&1 &
;;
esac
xlabel info "logserver is started"

;;
stop)
pid=$($RSH $rhost ps -elf | grep logsvr | grep $USER | grep -v grep | awk '{print $1}' )
kill -9 $pid
xlabel info "logserver is stoped"
;;

ping)
case $ARCH in
cray) ps -elf | grep logsvr | grep $USER # grep -v grep || exit 1
      xlabel info "???"
     trap 0; xcomplete; exit 0
;;
*) $RSH $rhost ps -elf | grep logsvr | grep -v grep | grep $USER || exit 1 
;;
esac

xlabel info "$($RSH $rhost ps -elf | grep logsvr | grep -v grep | grep $USER)"
;;
esac

%include <endt.h>
"""
    create_wrapper("logsvr.sms", content)

    ### task
    content = """#!/bin/ksh
%manual 
manual - this task is automatically created by cray.py
%end
%include <qsub.h>
if [[ $ARCH = hp* ]]; then export PATH=/usr/local/bin:$PATH; fi
if [[ $HOST = lxop* ]]; then export PATH=/usr/local/apps/ecflow/current:/usr/local/apps/sms/bin:$PATH; fi
%include <trap.h>
SLEEP=%SLEEP:0%
echo OK

case %ECF_PORT:0% in
0) base=900000; LOGPORT=%LOGPORT:0%;;
*) base=1000; LOGPORT=%LOGPORT:0%;;
esac
base=1000

case $HOST in
cc*)xlabel info $(printenv | grep -E '(SUBMIT_|EC_)') ;;
*) xlabel info OK
esac
  
printenv | sort
xevent 1
step=0
while (( $step <= 12 )); do
  xmeter step $step
  ((step = step + 1))
  sleep $SLEEP
done
%include <endt.h>
"""
    create_wrapper("test.sms", content)

    content = """#!/bin/ksh
%manual 
manual - this task is automatically created by cray.py

expected to run in real-time mode only

%end
#include <qsub.h>
#include <step1.h>
%include <trap.h>

KIND=%KIND:when%
NOW=$(date +%%H%%M)

case $KIND in 
when) TIME=0; 
for when in $(echo %WHENS:0%); do
  if [[ $((NOW - when)) -le 1 ]]; then break; fi
  TIME=$when
done
for when in $(echo %WHENS:0%); do
  if   [[ $when -le $NOW ]] then  xmeter time $when; sleep %SLEEP:5%; fi
  done;; 
time) TIME=%TIME:0%

main=/sapp/run/ext
case $TIME in 
1350) path=$main/DC1/00;;
0120) path=$main/DC1/12;;
0205) path=$main/DC1/18;;
1405) path=$main/DC1/06;;
1605) path=$main/DA1/12;;
2205) path=$main/DA1/18;;
0405) path=$main/DA1/00;;
1005) path=$main/DA1/06;;
*) xabort;;
esac

case %ECF_PORT:0% in
0) login="set SMS_PROG %SMS_PROG:0%; login %SMSNODE:0% $USER 1"
   cdp -c "$login; force -r complete $path";;
*) ecflow_client --force=complete recursive $path;;
esac

;;
esac
%include <endt.h>
#include <step2.h>
"""

    create_wrapper("sapp.sms", content)

    content = """
%manual 
manual - this task is automatically created by cray.py

expected to run in real-time mode only
%end
#include <qsub.h>
#include <step1.h>
%include <trap.h>

if [[ $USER == map ]]; then
cd /tmp/map/work/p4/metapps/suites/o/def
make etest | grep -v Node::addVariable:
make teste | grep -v Node::addVariable:

TEST="" make oe edae lawe lbce rd mce mofce
TEST= "" make e eeda elbc elaw 

fi

cd /home/ma/emos/def/o/def
# make etest| grep -v Node::addVariable:
# make teste| grep -v Node::addVariable:

TEST="" make oe edae lawe lbce rd mce mofce| grep -v Node::addVariable:
TEST="" make e eeda elbc elaw | grep -v Node::addVariable:

make doc
make covtest
make cov| grep -v Node::addVariable:

# e o rd

%include <endt.h>
#include <step2.h>
"""
    create_wrapper("maker.sms", content)

    content = """#!/bin/ksh
%manual 
manual - this task is automatically created by cray.py
expected to run in real-time mode only
%end
#include <qsub.h>
#include <step1.h>
%include <trap.h>

KIND=%KIND:when%
NOW=$(date +%%H%%M)

YMD=%YMD:20010101%
BASETIME=%YMD:20010101%%EMOS_BASE:00%
BASE=%EMOS_BASE:00%
STREAM=%STREAM:elda%
MEMBER_FROM=%MEMBER_FROM:%
MEMBER=%MEMBER:%
EXPVER_FROM=%EXPVER_FROM:9093%
EXPVER=%EXPVER:9963%

case $HOST in
c2*) FDB_ROOT=%STHOST:/s2o1%/ma_fdb;;
cc*) FDB_ROOT=%STHOST:/sc1%/tcwork/emos/ma_fdb;;
*) exit 1;;
esac

cd $FDB_ROOT
mkdir fix
cd fix
scp emos@%SCHOST_BKUP:cca:$FDB_ROOT/:od:$STREAM:g:$EXPVER_FROM:$YMD::/:${EMOS_BASE}00:$MEMBER_FROM:*: .

cat > rules.txt <<EOF
set expver="$EXPVER";
write;
EOF

for f in $(ls :${EMOS_BASE}00:$MEMBER_FROM:*:); do
  grib_filter -o $f.0001 rules.txt $f
done

echo "mkdir -p test; for f in $(ls :${EMOS_BASE}00:$MEMBER_FROM:*:); do FDB_ROOT=$(pwd)/test grib2fdb $f; done"
echo "mv test/*$STREAM:*/* $FDB_ROOT/:od:$STREAM:g:$EXPVER:$YMD::/."
done

case $KIND in 
VERSION) echo;;
MEMBER) echo;;
*) xabort;;
esac

;;
esac
%include <endt.h>
#include <step2.h>
"""

    create_wrapper("safety.sms", content)

def create_de():
    try:    os.stat(wdir + "/de")
    except: os.mkdir(wdir + "/de")       
    task = open(wdir + "/de/setup.ecf", 'w')
    print >>task, '''#!/bin/ksh
%include <head.h>
%ecfmicro ~
~includenopp <scripts/setup.run>
~ecfmicro %
%include <tail.h>
'''

    task = open(wdir + "/de/model.ecf", 'w')
    print >>task, '''#!/bin/ksh
# QSUB -lh  %THREADS:1%
# QSUB -lPv %NPES:1%
# @ tasks_per_node = 64
%include <head.h>
%ecfmicro ~
~includenopp <scripts/model.run>
~ecfmicro %
%include <tail.h>
'''

    logdir = "/s2o2/emos_dir"
    return Family("de").add(
        Defcomplete(),
        Repeat("YMD", 20100101, 20121212),
        Variables(ECF_EXTN= ".ecf",
                  ACCOUNT= "oesu",
                  SCHOST= "cca",
                  ECF_LOGHOST= "cca",
                  ECF_OUT= logdir,
                  LOGDIR= logdir,
                  QUEUE= "ns",
                  TURTLES= 1000*'I LIKE ',
                  ),
        Task("setup"),
        Task("obs").add(Trigger("setup==complete")),
        Task("namelist").add(Trigger("obs==complete")),
        Task("model").add(
            Variables(NPES= 1,
                      THREADS= 1,
                      QUEUE= "np",),
            Trigger("namelist==complete")),
        Task("postproc").add(Trigger("model==complete")),
        )

def create_smhi():
    submit = open(wdir + "/smhi_run.sh", 'w')
    print >>submit, '''#!/bin/ksh
set -eux
ECF_NAME=$1
ECF_PASS=$2
ECF_JOB=$3
ECF_JOBOUT=$4
ECF_HOST=$5
ECF_PORT=$6
export ECF_NAME ECF_PASS ECF_HOST ECF_PORT ECF_JOB ECF_JOBOUT
echo "#MSG: START USER SCRIPT"
. $ECF_JOB # >> $ECF_JOBOUT 2>&1
'''

    man = open(wdir + "/smhi_man.h", 'w')
    print >> man, '''
an example for include manual page
'''
    man.close()

    shell = open(wdir + "/smhi.sh", 'w')
    print >> shell, '''#!/bin/ksh
set -eux
# use ecflow || 
export PATH=/usr/local/apps/ecflow/current/bin:$PATH
export SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31'
ecflow_client --init=$$

ERROR() {
  ecflow_client --abort # raison:trap
  echo "#WAR: TRAPPING ERROR"
  trap 0  
  uname -a; date; times
  sleep 0
  exit 0
}
trap ERROR 0 $SIGNAL_LIST

ecflow_client --event=1
ecflow_client --meter=step 10

echo "simulates an error"; exit 1
echo "simulates an early exit: expect this to be trapped and reflected as error"
exit 0 # test bubbling up an error through ssh

'''
    wrapper = open(wdir + "/smhi_v.sms", 'w')
    print >>wrapper, '''
%manual
  example of minimalist script: only variables, wrapper is added at submission time
%end
#SET ECF_NAME=%ECF_NAME%
#SET ECF_HOST=%ECF_HOST%
#SET ECF_PORT=%ECF_PORT%
#SET ECF_PASS=%ECF_PASS%
#SET ECF_JOB=%ECF_JOB%
#SET ECF_JOBOUT=%ECF_JOBOUT%
#SET ECF_TRYNO=%ECF_TRYNO%
#SET SCRIPT_PATH=%SCRIPT_PATH%
#SET SCRIPT_NAME=%SCRIPT_NAME%
'''

    submit = open(wdir + "/smhisubmit.sh", 'w')
    print >>submit, '''#!/bin/ksh
set -eux
ECF_JOB=$1
grep "#SET " $ECF_JOB | sed -e "s:#SET ::" > $ECF_JOB.set
grep "#SET " $ECF_JOB > $ECF_JOB.var
. $ECF_JOB.set
echo "#!/bin/ksh" > $ECF_JOB
cat $ECF_JOB.set $ECF_JOB.var >> $ECF_JOB
cat >> $ECF_JOB <<\@@
set -eux
export ECF_NAME ECF_PASS ECF_HOST ECF_PORT
export PATH=/usr/local/apps/ecflow/current/bin:$PATH
export SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31'
ERROR() {
  ecflow_client --abort trap
  echo "#WAR: TRAPPING ERROR"
  trap 0 
  uname -a; date; times
  exit 1
}
trap ERROR 0 $SIGNAL_LIST

if [[ ${USE_SSH_SUB:=0} == 1 ]]; then
ssh $SCHOST mkdir -p $(dirname $ECF_JOBOUT)
ssh $SCHOST -R$ECF_PORT:$ECF_HOST:$ECF_PORT ''' + wdir + '''/smhi_run.sh \
  $ECF_NAME $ECF_PASS $SCRIPT_PATH/$SCRIPT_NAME $ECF_JOBOUT $ECF_HOST $ECF_PORT
echo "#MSG: ssh exits with $?"
else
ecflow_client --init=$$

i=0
while [[ $i -lt ${STEPS:=1} ]]; do
  ecflow_client --meter=step $i
  ((i=i+1))
done

ecflow_client --event=1

fi

wait
trap 0
ecflow_client --complete
uname -a; date; times
exit 0
@@

chmod 755 $ECF_JOB
$ECF_JOB > $ECF_JOBOUT 2>&1 &

'''

    os.system("chmod 755 " + wdir + "/smhisubmit.sh")
    wrapper = open(wdir + "/smhi.sms", 'w')
    os.system("chmod 755 " + wdir + "/smhi_run.sh")
    cmd = "ln -sf %s/test.sms " % wdir + wdir + "/"
    os.system(cmd + "a.sms")
    os.system(cmd + "a1.sms")
    os.system(cmd + "a11.sms")
    print "##cmd", cmd

    print >> wrapper, '''#!/bin/ksh
%manual 
%include <smhi_man.h>
%end
set -eux
# use ecflow ||
export PATH=/usr/local/apps/ecflow/current/bin:$PATH
export SIGNAL_LIST='1 2 3 4 5 6 7 8 13 15 24 31'
ECF_NAME=%ECF_NAME%
ECF_PASS=%ECF_PASS%
ECF_HOST=%ECF_HOST%
ECF_PORT=%ECF_PORT%
export ECF_NAME ECF_PASS ECF_HOST ECF_PORT

ERROR() {
  ecflow_client --abort # raise:trap
  echo "#WAR: TRAPPING ERROR"
  trap 0  
  uname -a; date; times; sleep 0
  exit 1
}

SLEEP=%SLEEP:0%
trap ERROR 0 $SIGNAL_LIST

if [[ %USE_SSH_SUB:0% == 1 ]]; then
ssh %SCHOST% mkdir -p $(dirname %ECF_JOBOUT%)
ssh %SCHOST% -R%ECF_PORT%:%ECF_HOST%:%ECF_PORT% ''' + wdir + '''/smhi_run.sh \
  %ECF_NAME% %ECF_PASS% %SCRIPT_PATH%/%SCRIPT_NAME% %ECF_JOBOUT% %ECF_HOST% %ECF_PORT%
  echo "#MSG: ssh exits with $?"
else
ecflow_client --init=$$

i=0
while [[ $i -lt ${STEPS:=1} ]]; do
  ecflow_client --meter=step $i
  ((i=i+1))
done

ecflow_client --event=1

fi

wait
trap 0
ecflow_client --complete
uname -a; date; times
exit 0
'''

def dummy(): return Task("test")

def logsvr(kind): return Task("logsvr").add(
    Label("man", "kind may be start, stop, ping"),
    Label("info", ""),
    Variables(KIND= kind))

def limits():
    return (Family("Test").add(Defcomplete(),
                               Label("info", "test find reg exp, case sens"),
                               Task("Ecmwf"),
                               Task("ECMWF"),
                               Task("ecmwf"),
                               Task("ecm")),
            Family("limits").add(
            Defcomplete(),
            Limit("lim", 10),
            Limit("test", 10),
            Limit("hpc", 10),
            Limit("cca", 10),
            Limit("ccb", 10),
            # Limit("cct", 10),
            # Limit("c2a", 10),
            # Limit("c2b", 10),
            Inlimit(ic.psel() + "/limits:lim")))

def repeat():
    return Family("repeat").add(
        Defcomplete(),
        Family("int").add(
            Repeat("YMD", 21000101, 21001212, 1, "integer"),
            dummy()),
        Family("date").add(
            Repeat("YMD", 21000101, 21001212, 1, "date"),
            dummy()),
        Family("string").add(
            Repeat("YMD", ["1", "2", "3", "4", "5"], kind="string"),
            dummy()),
        Family("enum").add(
            Repeat("YMD", "1 2 3 4 5".split(), kind="enum"),
            dummy()),
        )
        

def fam_ui():
    return Family("ui").add(
        Defcomplete(),
        repeat(),
        
        Variables(VAR= 1),

        Family("limit").add(Limit("mutex", 1),
                            InLimit("limit:mutex"),
                            Defstatus("complete"),
                            dummy()),

        Family("autocancel").add(AutoCancel("+01:00"),
                                 dummy()),

        dummy().add(Meter("step", -1, 100),
                          Event(1),
                          Label("info", ""),
                          Late("-s 00:10 -c 01:00"),
                          ),

        Family("complete").add(Complete("test:1"),
                               Trigger("test==complete")),

        Task("time").add(Time("10:00"),
                         Today("11:00"),
                         Cron("12:00 13:00 00:05"),
                         ),
        Task("date").add(Date("01.*.*"),)
        )

def smhi():
    return Family("smhi").add(
        Defcomplete(),

        Variables(ECF_FILES= wdir,
                  QUEUE= "none",
                  LOGDIR= jdir, # "/vol/lxop_emos_nc/output",
                  MANPATH= "%ECF_SCRIPT%",
                  ECF_JOB_CMD= "%ECF_JOB% > %ECF_JOBOUT% 2>&1",
                  ECF_HOME= jdir, # "/tmp/smhi",
                  ECF_INCLUDE= wdir,),

        acq(),

        Family("ex1").add(Cron("00:00 23:59 00:03"),
                          Task("scan").add(Label("info", "one min"),
                                           Event("1")),
                          Task("proc").add(Label("info", "one min"),
                                           Trigger("scan:1"),
                                           Complete("scan eq complete and not scan:1"))),
        Family("ex2").add(
            Task("scan").add(Label("info", "one min"),
                             Cron("00:00 23:59 00:03"),
                             Event("1")),
            Family("consume").add(
                Label("info", "event is set by scan task"),
                Event("1"),
                Cron("00:00 23:59 00:05"),
                Trigger("consume:1"),                
                Task("proc1").add(Trigger("1==0", Complete("1==1"))),
                Task("proc2").add(Trigger("1==0", Complete("1==1"))),
                )),
        Family("name").add(
            Variables(ECF_JOB_CMD= SUBMIT,
                      ECF_KILL_CMD= KILL,
                      ECF_STATUS_CMD= STATUS,
                      ECF_CHECK_CMD= CHECK,
                      # ECF_EXTN= ".sms",
                      ACCOUNT=  "UNSET",                      
                      ECF_INCLUDE= idir,
                      ECF_HOME= jdir,
                      ECF_FILES= wdir,
                      TOPATH= udir + "/logs",
                      SLEEP=    1,
                      SCHOST= "ibis",
                      ECF_TRIES= 1,),
            Task("a").add(Meter("step", -1, 100), Event("1")),
            Task("a1").add(Meter("step", -1, 100), Event("1")),
            Task("a11").add(Meter("step", -1, 100), Event("1")),
            ),
        
        Family("ssh").add(
            Variables(USE_SSH_SUB= 1,
                      SCRIPT_PATH= wdir,
                      SCRIPT_NAME= "smhi.sh"),            
            smhi_unit("ibis", "ibis", "none", jdir),
            smhi_unit("pikachu", "pikachu", "none", jdir),
            ),

        Family("alter").add(
            Variables(USE_SSH_SUB= 1,
                      ECF_JOB_CMD= "/home/ma/map/course/cray/smhisubmit.sh %ECF_JOB%",
                      SCRIPT_PATH= wdir,
                      SCRIPT_NAME= "smhi.sh"),            
            smhi_unit_alter("ibis", "ibis", "none", jdir),
            smhi_unit_alter("pikachu", "pikachu", "none", jdir),
                          )
        )        
                          
def acq():
    name = "acq_ex"

    fam = Family(name).add(
        Defcomplete(),
        Family("times").add(
            dummy().add(
                Time("10:00 20:00 00:30"))),
        Complete(name + "/data eq complete"),
        Task("data").add(
            Event("ready"),
            Trigger("rt/wait:data"),
            ),
        Family("rt").add(
            Complete("data:ready"),
            Task("wait").add(
                Event("data"),
                # Cron("10:00 12:00 00:05")
                )),
        Task("late").add(
            Time("11:00"),
                Trigger("not data:ready")))
    return fam


HCRAY="cct"

SUBMIT=ic.SUBM + " %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT% submit"
GSUB=SUBMIT.replace("%SCHOST%", "%SCHOST% %ECF_RID%")
KILL=GSUB.replace(" submit", " kill")
STATUS=GSUB.replace(" submit", " status")
CHECK=STATUS


def unit(name, schost, queue, rdir, account="", leaf=True, add=None):
    return Family(name).add(
        If(account != "",
           Variables(ACCOUNT= account)),
        Variables(QUEUE=    queue, 
                  SCHOST=   schost,
                  ECF_OUT=  rdir,
                  LOGDIR=   rdir,),        
        If (leaf, Task("test").add(
                add,
            Event(1),
            Meter("step", -1, 120, 100),
            Label("info", "nop"))))

def smhi_nit(name, schost, queue, rdir, account=""):
    return Family(name).add(
        If(account != "", Variables(ACCOUNT= account)),
        Variables(QUEUE=    queue, 
                  SCHOST=   schost,
                  ECF_OUT=  rdir,
                  LOGDIR=   rdir,),        
        Task("smhi").add(
            Event(1),
            Meter("step", -1, 120, 100),
            Label("info", "nop")))

def smhi_unit(name, schost, queue, rdir, account=""): return None

def smhi_unit_alter(name, schost, queue, rdir, account=""):
    return Family(name).add(
        If(account != "", Variables(ACCOUNT= account)),
        Variables(QUEUE=    queue, 
                  SCHOST=   schost,
                  ECF_OUT=  rdir,
                  LOGDIR=   rdir,),        
        Task("smhi_v").add(
            Event(1),
            Meter("step", -1, 120, 100),
            Label("info", "nop")))

def turkey_main(day, path):
    def runs(num, time):
        return Family('run%d' % num).add(
               Trigger(path + '/deps/%s eq complete' % time),
            
               Family('feed_meb').add(
                Task('strip_message'),                  

                Task('insert_meb').add(
                    Trigger('strip_message eq complete'),
                  ),
               ),               
               Family('feed_reb').add(
                  Task('decode_synop').add(
                     Trigger('../feed_meb/insert_meb eq complete'),
                  ),
                  Task('decode_temp').add(
                     Trigger('../feed_reb/decode_synop eq complete'),
                  ),
                  Task('decode_ship').add(
                     Trigger('../feed_reb/decode_temp eq complete'),
                  ),
                  Task('decode_metar').add(
                     Trigger('../feed_reb/decode_ship eq complete'),
                  ),
               ),               
               Family('rdb').add(
                  Task('rdb_synop').add(
                     Trigger('../feed_reb/decode_synop eq complete'),
                  ),
                  Task('rdb_temp').add(
                     Trigger('../feed_reb/decode_temp eq complete'),
                  ),
                  Task('rdb_ship').add(
                     Trigger('../feed_reb/decode_ship eq complete'),
                  ),
                  Task('rdb_metar').add(
                     Trigger('../feed_reb/decode_metar eq complete'),
                  ),
               ),               
            ),            

    def task_trigger(name, condition):
        return Task(name).add(Trigger(condition))

    def echos():
        echo = { 
            "cleandb" : "0131",
            "cleanerrdb": "0136",
            "silmeb": "0141",
            "silreb": "0146",
            "run1": "0221",
            "run2": "0421",
            "run3":"0621",
            "run4": '0821',
            'run5': '1021',
            'run6': '1321',
            'run7': '1421',
            'run8': '1621',
            'run9': '1821',
            'run10': '2121',
            'eksikveritamamla': '2201',
            'uydutsm': '0201',
            'model_all_gunluk_tsm': '0131',
            'radar_lokasyon_tip_tsm': '0501',
            'retObsAndFtp_for_00': '0231',
            'retObsAndFtp_for_18': '0241',
            'retObsAndFtp_for_12': '1541',
            'retObsAndFtp_for_06': '1101',
            'retObsAndFtp_for_06gungeri': '0801',
            'uydureport': '1655',
            'radarreport': '0701',
            'backuplog': '2301',
            'cleanup': '2316'
            }
        out = []

        for key in sorted(echo, key=echo.get, reverse=False): # order by value
            # for key in sorted(echo.keys()): # order by key
            out.append(task_trigger("echo_%s" % key, 
                                    path + "/deps/%s" % echo[key] + " eq complete"))
        return out
        
    return Family('main').add(
            Trigger('deps/0000 eq complete'),
         
            Family('clean').add(
               task_trigger('cleandb',
                            path + '/deps/0130 eq complete'),

               Task('clean_errdb').add(
                  Trigger(path + '/deps/0135 eq complete'),
               ),
               Task('silmeb').add(
                  Trigger(path + '/deps/0140 eq complete'),
               ),
               Task('silreb').add(
                  Trigger(path + '/deps/0145 eq complete'),
               ),
            ),         
            runs(1, "0220"),
            runs(2, "0420"),
            runs(3, "0620"),
            runs(4, "0820"),
            runs(5, "1020"),
            runs(6, "1320"),
            runs(7, "1420"),
            runs(8, "1620"),
            runs(9, "1820"),
            runs(10, "2120"),
            Family('model').add(
               Task('eksikveritamamla').add(
                  Trigger(path + '/deps/2200 eq complete'),
               ),
            ),            
            Family('tsmarc').add(
               Task('uydu_tsm').add(
                  Trigger(path + '/deps/0200 eq complete'),
               ),
               Task('model_all_gunluk_tsm').add(
                  Trigger(path + '/deps/0130 eq complete'),
               ),
               Task('radar_lokasyon_tip_tsm').add(
                  Trigger(path + '/deps/0500 eq complete'),
               ),
            ),            
            Family('ftpsenddata').add(
               Task('retObsAndFtp_for_00').add(
                  Trigger(path + '/deps/0230 eq complete'),
               ),
               Task('retObsAndFtp_for_18').add(
                  Trigger(path + '/deps/0240 eq complete'),
               ),
               Task('retObsAndFtp_for_12').add(
                  Trigger(path + '/deps/1540 eq complete'),
               ),
               Task('retObsAndFtp_for_06').add(
                  Trigger(path + '/deps/1100 eq complete'),
               ),
               Task('retObsAndFtp_for_06gungeri').add(
                  Trigger(path + '/deps/0800 eq complete'),
               ),
               Task('uydu_report').add(
                  Trigger(path + '/deps/1654 eq complete'),
               ),
               Task('radar_report').add(
                  Trigger(path + '/deps/0700 eq complete'),
               ),
            ),            
            Family('cleanup').add(
               Task('backuplog').add(
                  Trigger(path + '/deps/2300 eq complete'),
               ),
               Task('cleanup').add(
                  Trigger(path + '/deps/2315 eq complete'),
               ),
            ),            
            Family('echo').add(
               Variables(
                  ECF_FILES= '/pp1/bin/dpp_sms/echo',),

               echos(),           
         ))


def turkey_deps(num):
    def item(hhmm):
        return  Family(hhmm).add(
               Complete("./%s/compl eq complete and " % hhmm + 
                        './%s/dummy eq queued' % hhmm),
            
               Task('dummy').add(
                  Cron('-w %d' % num + ' ' + hhmm[:2] + ":" + hhmm[-2:]),
               ),
               Task('compl').add(
                  Complete("dummy eq active or " +
                           "dummy eq submitted or " +
                           'dummy eq complete'),
                  Trigger('1 eq 0'),
               ),
            )

    items = [ "0000",
"0130",
"0135",
"0140",
"0145",
"0220",
"0420",
"0620",
"0820",
"1020",
"1320",
"1420",
"1620",
"1820",
"2120",
"2200",
"0200",
"0500",
"0230",
"0240",
"1540",
"1100",
"0800",
"1654",
"0700",
"2300",
"2315",
"0131",
"0136",
"0141",
"0146",
"0221",
"0421",
"0621",
"0821",
"1021",
"1321",
"1421",
"1621",
"1821",
"2121",
"2201",
"0201",
"0501",
"0231",
"0241",
"1541",
"1101",
"0801",
"1655",
"0701",
"2301",
"2316", ]
    def loop_items(): 
        out = []
        for hhmm in items: 
            out.append(item(hhmm))
        return out 

    return Family('deps').add(
            Variables(
               ECF_FILES= '/pp1/bin/dpp_sms/haftalik',),
            loop_items())

def turkey_loop(suite_name):
    out = []
    days = [ "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi", "Pazar"]
    days = [ "mon", "tue", "wed", "thu", "fri", "sat", "sun"]
    start = 20120723; end=20301231 # beware addition below does not consider start as a date
    path = "/%s/weekly/" % suite_name

    for num in xrange(7): 
        out.append(Family(days[num]).add(
                Repeat(kind='date', name='YMD', 
                       start= start + num, end=end, step=7),
                turkey_deps(1),
                turkey_main(days[num], path=path + days[num]),))
    return out

def turkey(suite_name):
    return    Family('weekly').add(
        Label("repeat", "run one family per week day"),
        Label("time_trigger", "replace time dependencies with trigger"),
        Label("aborted", "one aborted task requests being fixed within one week"),
        Defcomplete(),
        turkey_loop(suite_name),)

##########################################################################
## PURE

class Languages(object):
    def __init__(self): pass 
    def task(self): return None
    def scripts(self): pass

class Python(Languages):
    def task(self):
        return (
            Task("pure_python").add(
                Variables(ECF_MICRO= "$",
                          ECF_INCLUDE= wdir,
                          SCHOST= "localhost",
                          ECF_JOB_CMD= "use ecflow; $ECF_JOB$ > $ECF_JOBOUT$ 2>&1 &"),
        Event("1"),
        Label("info", ""),
        Meter("step", -1, 100),
        # Defstatus("complete"),
        ),

        Task("python").add(
        Variables(ECF_MICRO= "$",
                  ECF_INCLUDE= wdir,
                  SCHOST= "localhost",
                  ECF_JOB_CMD= "$ECF_JOB$ > $ECF_JOBOUT$ 2>&1 &"),
        Event("1"),
        Label("info", ""),
        Meter("step", -1, 100),
        # Defstatus("complete"),
        ), )

class Perl(object):
    def task(self):
        return Task("perl").add(
            Label("todo", "time-stamp PS4, set eux, exit 0 1,"),
            Variables(ECF_MICRO= "^",
                      ECF_INCLUDE= wdir,
                      SCHOST= "localhost",
                      ECF_JOB_CMD= "^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1 &"),
            Event("1"),
            Label("info", ""),
            Meter("step", -1, 100),
            # Defstatus("complete"),
            )

class Ruby(object):
    def task(self):
        return Task("ruby").add(
            Label("todo", "time-stamp PS4, set eux, exit 0 1,"),
            Variables(ECF_MICRO= "^",
                      ECF_INCLUDE= wdir,
                      SCHOST= "localhost",
                      ECF_JOB_CMD= "^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1 &"),
            Event("1"),
            Label("info", ""),
            Meter("step", -1, 100),
            Defstatus("complete"),
            )

    def scripts(self):
        head = """#!/usr/bin/env ruby"""
        tail = """ """
        body = """ """
        create_wrapper("head.rb", head)
        create_wrapper("tail.rb", tail)
        create_wrapper("ruby.sms", body)

ruby = Ruby()
perl = Perl()
pyth = Python()

WDIR= "/scratch/ma/map/sms"
HLOG= "/home/ma/map/logs"

hosts = {"lxb" : { "COMPILER": "gcc-4.3", # linux64
                   "SCHOST": "lxb", 
                   "QUEUE_EPILOG": "serial",
                   "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                   "QUEUE": "serial",
                   "WDIR": WDIR, }, 
         "ibis" : { "COMPILER": "gcc",
                    "SCHOST": "ibis", 
                    "WDIR": WDIR, }, 
         "lxop" : {  "COMPILER": "gcc-4.3",
                    "SCHOST": "lxop",
                     "QUEUE": "test", 
                     "QUEUE_EPILOG": "test",
                     "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                     "COMPILE_DIR": "/gpfs/lxop/emos_data/sms", 
                    "WDIR": "/gpfs/lxop/emos_data/sms", 
                     }, 
         "opensuse103": { "COMPILER": "gcc-4.5",
                         "SCHOST": "opensuse103", 
                     "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                         "WDIR": WDIR, }, 
         "opensuse113": { "COMPILER": "gcc-4.5",
                         "SCHOST": "opensuse113", 
                     "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                         "WDIR": WDIR, }, 
         "opensuse131": { "COMPILER": "gcc-4.8",
                         "SCHOST": "opensuse131", 
                     "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                         "WDIR": WDIR, }, 
         "ecgb": { "COMPILER": "gcc-4.4.7", # redhat
                   "QUEUE": "normal", 
                   "QUEUE_EPILOG": "normal",
                   "SCHOST": "ecgb", 
                     "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                    "WDIR": WDIR, }, 
         "lxc": { "COMPILER": "gcc-4.4.7", # redhat
                   "QUEUE": "normal", 
                   "QUEUE_EPILOG": "normal",
                   "SCHOST": "lxc", 
                   "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                   "WDIR": WDIR, }, 
         "cct": { "COMPILER": "gcc",
                  "QUEUE_EPILOG": "ns",
                  "SCHOST": "cct", 
                  "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                  "COMPILE_DIR": "/home/ma/map/sms", 
                  "WDIR": "/home/ma/map/sms", }, 
         "cca": { "COMPILER": "gcc",
                  "QUEUE_EPILOG": "ns",
                      "SCHOST": "cca", 
                       "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                       "WDIR": "/home/ma/map/sms", }, 
         "ccb": { "COMPILER": "gcc",
                  "QUEUE_EPILOG": "ns",
                      "SCHOST": "ccb", 
                       "LOGDIR": HLOG, "ECF_OUT": HLOG, 
                      "WDIR": "/home/ma/map/sms", 
                  "COMPILE_DIR": "/home/ma/map/sms", 
                  },          }

class Compile(object):

    def __init__(self):
        pass

    def compile(self, host=None, add=None, but=None):
        version = "4.4.15"
        if host is None: dest = hosts.keys()
        else: dest = (host, )
        out = []
        for host in sorted(dest):
            if not but: pass
            elif host == but or host in but: continue
            out.append(Family(host).add(
                    add,
                    Variables(hosts[host]),
                    Variables(ECF_FILES= wdir,
                              VERSION= version,
                              ECF_INCLUDE= idir,         ),
                    Task("get"),
                    Task("make").add(Trigger(["get"])),
                    Task("install").add(Defcomplete(),
                                     Trigger(["make"])),
                    Task("testit").add(Event("cdp"),
                                       Event("sms"),
                                       Event("child"),
                                       Event("xcdp"),
                                       Trigger(["make"])),
                    Task("child").add(Defcomplete(),
                                      Variables(SMS_VERSION= "4.4.15",),
                                      Label("info", ""),
                                      Event("1"),
                                      Meter("step", -1, 100)),
                    ))
        return out

    def main(self):
        self.create_tasks()
        return Family("sms").add(Family("compile").add(
            Defstatus("suspended"),
            # onws("localhost"),
            Variables(SCHOST= "localhost"),
            Task("prepare"),
            self.compile("ibis", add=Trigger("prepare==complete")),
            Family("remote").add(Trigger("prepare==complete"),
                                 self.compile(but="ibis"),)))

    def wrap(self, content):
        return '''#!/bin/ksh
# generated by cray.py
%%manual
  start - check - stop logsvr
%%end
%%include <qsub.h>
%%include <step1.h>
%%include <trap.h>
%s 
%%include <endt.h>
%%include <step2.h>''' % content

    def create_tasks(self):
        source = "%SOURCE_DIR:/tmp/map/work/git/sms%"
        compil = "%COMPILE_DIR:/scratch/ma/map/sms/$ARCH/%/%SCHOST:$HOST%"
        task = self.wrap("cd %s/sms; make clean; cd ../; tar -czf sms.tgz sms; " % source)
        create_wrapper("prepare.sms", task)
        
        task = self.wrap("comp=%s; " % compil + 
                         "mkdir -p $comp; cd $comp; scp ibis:%s/sms.tgz .;" % source +
'''rm -rf sms.tar sms || :
gzip -d sms.tgz; tar -xf sms.tar''')
        create_wrapper("get.sms", task)

        task = self.wrap("comp=%s;" % compil + 
                         '''cd $comp/sms; 
if [[ $ARCH == cray ]]; then
module switch PrgEnv-cray PrgEnv-intel
export CRAYPE_LINK_TYPE=dynamic
fi
make %TARGET:linux%''')
        create_wrapper("make.sms", task)

        task = self.wrap("comp=%s;" % compil +  
                         '''cd $comp/sms; make tester; 
mkdir -p %SMSHOME:/tmp/$USER%
SMS_PROG=900001 sms &
sleep 10
xevent sms
SMS_PROG=900001 smsping localhost || :
./cdp/cdp -c 'set SMS_PROG 900001; login $HOST $USER 1; terminate -y; exit 0'
xevent cdp
DISPLAY=ibis:0.0 ./xcdp/xcdp &
xevent xcdp
ln -sf ./cdp/child smsevent
./smsevent child
rm -f smsevent''')
        create_wrapper("testit.sms", task)

        task = self.wrap("xevent 1; xlabel info test; xmeter step 0; xmeter step 100;")
        create_wrapper("child.sms", task)

##########################################################################
## BARBER

def call_barber_passerby():
    # family passby
    # limit to one at a time entering the
    # shop to check if there is room to
    # stay in there
    return Task("passby").add(
        Cron("00:00 23:59 00:01"),
    # this one is regular
        Variables(ID= 0),
        Inlimit("limit:passby"))
    # other frequent passers by can be added here
# called by client generator task

def call_barber_client(id):
    return Family(id).add(
    # to be called by passby to create a new client
        Task("cut").add(
            Inlimit("../limit:barbers")),
    # there may be many barbers in the shop
    # but one cashier only
        Task("pay").add(
            Trigger("./cut == complete"),
            Inlimit("../limit:barbers"),
            Inlimit("../limit:cashiers")),
        Task("leave").add(
            Trigger("cut==complete and pay==complete"),
            If(id % 2 == 0,
               Defcomplete(),
               Autocancel( "0" ))))

# main definition
def call_barber_shop():
    return Family("shop").add(
        Defstatus("suspended"),
        Variables(NB_CHAIRS= 4),

        Family("limit").add(
            Defcomplete(),
            Limit("passby", 1),
            Limit("barbers", 1),
            Limit("cashiers", 1)),

        call_barber_passerby(),)

##########################################################################
## dinner
total_number = 5
life_expectency = 100
def requeued():
    return Repeat("iter", 1, life_expectency, kind= "integer")
    # possibility to requeue the task without duration limit
    # provided 02:00 is below the task duration:
    # cron 00:00 23:59 02:00

def triggered(ref, tor=None, tand=None):
    trg = ""
    tr2 = ""
    if tand: trg = " and %s" % tand
    if tor:  tr2 = " or %s" % tor
    return Trigger("(%s != active and %s != submitted and %s != aborted" 
                % (ref, ref, ref) + trg + ")" + tr2)

def philosopher(id, s1, s2, version):
    return Family("%d" % id).add(
        Task("eat").add(
            Inlimit("../limit:"+str(s1)),
            Inlimit("../limit:"+str(s2)),
            Event("release")),
        If( version == "normal",
                # eat task a3ttribute
                # TBD triggered("think")
                ( requeued(),

                  Task("think").add(
                        requeued(),
                        triggered( "eat", tor="eat:release or eat==complete")))),
        If( version == "simple",
                (Task("release").add(
                        Trigger("eat:release or eat==complete"),),
                 requeued())))

def call_dinner():
    out = Family("dinner").add(
        Variables(SLEEP= 10),
        Defstatus("suspended"),
        )
    for version in "simple normal".split():
        # Xcdp would fold parent family when limits change
        # we prefer to store them in a dedicated family then
        lim = Family("limit").add(
                    Defcomplete(),)
# for num 1 $total_number do num 1 $total_number do
# $total_number do num 1 $total_number do
        num = 1
        while num <= total_number:
            lim.add(Limit(str(num), 1))
            num += 1; # endfor
        ver = Family(version).add(lim)
        out.add(ver)
# for num 1 $total_number do num 1 $total_number do
# $total_number do num 1 $total_number do
        num = 1
        while num <= total_number:
            stick1 = num
            stick2 = 1 + ((num) % total_number)
            ver.add(philosopher( num, stick1, stick2, version))
            num += 1; # endfor
    return out

##########################################################################
## CONSUMER
# /home/ma/map/course/course2011/src/consumer.py
beg =  0
fin = 48
by  =  3
    
def not_consumer(): return Variables(CONSUME= "no")

def not_producer(): return Variables(PRODUCE= "no")

def ev():return (Event("p"), Event("c"))

def call_task(name, start, stop, inc):
    meter = None
    if start != stop:
        meter = Meter( "step", -1, int(stop))

    return Task(name).add(
        ev(),
        Variables(BEG= start,
                  FIN= stop,
                  BY= inc),
        meter)

def call_consumer(SELECTION):
    # defstatus suspended # avoid early start
    def consume1(leap = 1, leap_nb = 3):
        out = []
        while leap <= leap_nb:
            leap_beg = beg + by * (leap - 1)
            leap_by = by * leap_nb
            out.append(Family("%d" % leap).add(
                    call_task( "consume", "'%STEP%'", "'%STEP%'", leap_by).add(
                        Repeat("STEP", leap_beg, fin, leap_by, kind="integer"),
                        # same trigger as consume0
                        # Trigger("(%s and (consume:STEP le %s1/produce:STEP)) or " % (lead, prod) + 
                        #         "(not %s and (consume:STEP le %s0/produce:step))" % (lead, prod))
                        Trigger("(consume:STEP le %s1/produce:STEP)" % prod),
                       )))
            leap += 1;
        return out

    def consume2(idx, fin):
        out = []
        while idx <= fin:  
            out.append(Family("%03d" % idx).add(
                    call_task( "consume", idx, idx, by).add(
                        Variables(STEP= idx),
                        # Same trigger as consume0
                        # Trigger("(%s and (consume:STEP le %s1/produce:STEP)) or " % (lead, prod) +
                        #         "(not %s and (consume:STEP le %s0/produce:step))" % (lead, prod))))
                        Trigger("consume:STEP le %s1/produce:STEP" % prod),
                       )))
            idx += by
        return out

    lead = "/%s/consumer/admin/leader:1" % SELECTION
    prod = "/%s/consumer/produce" % SELECTION
    if 1: return Family("consumer").add(Defcomplete()) # FIXME
    return Family("consumer").add(
        Defcomplete(),

        Variables(SLEEP= 10,
                  PRODUCE= "no",
                  CONSUME= "no"),

             Family("limit").add(
                Defcomplete(),
                Limit("consume", 7),),

             Family("admin").add(
                # set manually with Xcdp or alter the event 1 so
                # that producer 1 becomes leader
                # default is producer0 leads
                Task("leader").add(
                    Event("1"),
                    Defcomplete())),
             # text this task is dummy task not designed to run

             # default : task does both :
                 Variables(PRODUCE= "yes",
                           CONSUME= "yes"),
             # this task will do both, ie serial

             call_task( "produce", beg, fin, by).add(
            Label("info", "do both at once"),),
             # this will loop inside the task, reporting
             # its processed step as a meter indication

             Family("produce0").add(
                not_consumer(),
                call_task( "produce", beg, fin, by )),
             # here, choice is to push a new task for each step

             Family("produce1").add(
                not_consumer(),
                call_task( "produce", '%STEP%', "%STEP%", by ).add(
                Repeat("STEP", beg, fin, by , kind="integer"), )),
             # PRB edit FIN '$((%STEP% + %BY%))'

             Family("consume").add(
                not_producer(),
                Inlimit("limit:consume"),
                Variables(CALL_WAITER= 1,
                          # $step will be interpreted in the job!
                          TRIGGER= "../produce:step -gt consume:$step or ../produce eq complete"),
                call_task( "consume", beg, fin, by ).add(
                )),
             
             Family("consume0or1").add(
                not_producer(),
                Inlimit("limit:consume"),
                call_task( "consume", "%STEP%", "%STEP%", by, ),
                Repeat( "STEP", beg, fin, by, kind="integer"),
                Trigger("(%s and (consume0or1:STEP le %s1/produce:STEP)) or "
                        % (lead, prod) +
                        "(not %s and (consume0or1:STEP le %s0/produce:step))"
                        % (lead, prod))),

             Family("consume1").add(
                not_producer(),
                Inlimit("limit:consume"),
                consume1()),

             Family("consume2").add(
    # loop is "exploded"
    # consume limit may be changed manually with Xcdp
    # to reduce or increase the load
                Inlimit("limit:consume"),
                not_producer(),
                consume2(beg, fin)))

##########################################################################
class Cray(ic.SeedOD):
    def __init__(self):
        super(Cray, self).__init__()

    def setup(self, node):
        account= "UNSET"
        global user, host
        if user == "emos" or host in ("ode", "eode", "ecf"):
            rdir = "/vol/lxop_emos_nc/output"
            user= "emos"
            account= "oesu"
        else: pass
        self.defs.add_extern("/o/main:YMD")

        node.add(
            Clock("real"),
            Defstatus("suspended"),
            Label("info", "test,logsvr tasks generated by cray.py"),
            Event("1"),
            Meter("step", -1, 100),
            Repeat(kind="day", step=1),
            limits(),

            Variables(ECF_JOB_CMD= SUBMIT,
                      ECF_KILL_CMD= KILL,
                      ECF_STATUS_CMD= STATUS,
                      ECF_CHECK_CMD= CHECK,
                      # ECF_EXTN= ".sms",
                      ACCOUNT=  "UNSET",
                      SCHOST= "localhost",
                      ECF_INCLUDE= idir,
                      ECF_HOME= jdir,
                      ECF_FILES= wdir,
                      QUEUE= "ns", USER= user, LOGDIR= "/tmp",
                      TOPATH= udir + "/logs",
                      SLEEP=    1,     # x120
                      ECF_TRIES= 1,),

            Family("looper").add(Time("09:00"),
                                 Variables(SCHOST= "ibis"),
                                 dummy()), # .add(Defcomplete())),

            create_de(),

            smhi(),

            Compile().main(),

            Task("dummy").add(Time("16:00"),
                              Variables(SCHOST= "localhost",
                                        FAMILY= "unset",
                                        FAMILY1= "unset"),
                              Time("17:00"),),

            fam_ui(),

            # turkey(ic.selection()),

            sapp(),

            Family("maker").add(maker(),),

            call_consumer(ic.selection()).add(
                Label("repeat", "repeat may be attached to a family, or a task, to facilitate requeue or not"),
                Label("event", "event may be set by the task itself, by a user, by a third party job"),
                Label("trigger", "can be on the task-family node, or inside a task (wait)"),
                Label("deadlock", "produce0/produce complete with step=-1"),
                Label("latency", "between task wrapper creation, and new file visible from the server"),
                Label("ZZZ", "icon when the job is waiting the trigger condition"),
                Variables(QUEUE= "ns", USER= user, LOGDIR= "/tmp",
                          SCHOST="localhost"),),

            call_barber_shop().add(Defstatus("complete"),),

            call_dinner().add(Defstatus("complete"),),

            pyth.task(),

            perl.task(),

            ruby.task(),

            Family("blacklist").add(
                Event("cca"),
                Event("ccb"),
                Event("hps"),
                
                Defstatus("complete"))            )

        msg = "have you? setup ssh login, created remote directory" 
        msg += ", set START_LOGSVR=1 (edit task), once, to launch the logserver"
        if "eod" in host: user = "emos"
        print "#MSG: host, user", host, user

        node = node.family("submit").add(
            unit("jonas", "ibis", "no", "/tmp/$USER/jobs", add=(Today("07:00"),
                                                                Today("21:00"))),
            Variables(USER=     user,
                      ACCOUNT=  account), # 

            # Time("09:00"),

            self.arg4(),
            self.burst(),
            # self.arg3(),
            # self.prb(),

            )

    def burst(self):
        sub = "/home/ma/emos/bin/smssubmit "
        out = []
        for num in xrange(0, 10):
            out.append(Family("%03d" % num).add(Task("test")))
        
        return Family("arg3").add(
            Defstatus("complete"),
            Variables(SCHOST= "cca-b"),
            out
            )

    def arg3(self):
        sub = "/home/ma/emos/bin/smssubmit "
        return Family("arg3").add(
            Defstatus("complete"),

            self.submits(),

            Variables(ECF_JOB_CMD= sub +"%USER% %SCHOST% %ECF_JOB%",
                      ECF_KILL_CMD= sub + "%USER% %SCHOST% %ECF_JOB% kill",
                      ECF_STATUS_CMD= sub + "%USER% %SCHOST% %ECF_JOB% status",
                      ),
            )

    def arg4(self):
        return Family("arg4").add(self.submits())

    def prb(self):
        sub = "/home/ma/emos/bin/smssubmit "
        return Family("prb").add(
            Defstatus("complete"),
            self.submits(),
            Label("info", "simulate prb at submission with % remaining"),
            Variables(ECF_JOB_CMD= sub +"%USER% %SCHOST% %ECF_JOB% %STTHOST%%ECF_JOBOUT%",
                      ECF_KILL_CMD= sub + "%USER% %SCHOST% %ECF_JOB% %STTHOST%%ECF_JOBOUT% kill",
                      ECF_STATUS_CMD= sub + "%USER% %SCHOST% %ECF_JOB%  %STTHOST%%ECF_JOBOUT% status",
                      ),                  )

    def submits(self):
        global host, user
        def parallel(pes=1, thr=1, hyp="no"):
            return Family("p%d_%d_%s" %(pes, thr, hyp)).add(
                Variables(NPES=pes, THREADS=thr,                           
                          HYPER=hyp), 
                Task("test").add(Label("info", ""), 
                                 Meter("step", -1, 120),
                                 Event(1)))
        ct1logs= "/sc1/tcwork/emos/logs"
        # ct1logs = '/home/ma/%s/logs' % user
        if ecflow: base = 1000
        else: base = 900000
        base = 1000
        if "eod" in host: user = "emos"

        out = [

            unit("cray", HCRAY, "ns", ct1logs, leaf=False).add(
                unit("ccp", "cca-login1", "ns", ct1logs).add(
                    Complete(ic.psel() + "/blacklist:cca"),),
                unit("cca", "cca-login1", "ns",    ct1logs).add(
                    logsvr("ping"),
                    Complete(ic.psel() + "/blacklist:cca"),),
                unit("ccb", "ccb-login1", "ns",    ct1logs).add(
                    logsvr("ping"),                    ),
                Family("parallel_a").add(
                    # Defcomplete(), ###
                    Complete(ic.psel() + "/blacklist:cca"),
                    Variables(SCHOST= "cca",
                              QUEUE= "os",
                              LOGHOST= "cca-il2",
                              ECF_OUT= ct1logs,
                              LOGDIR=  ct1logs,),
                    parallel(1, 1),
                    parallel(1, 24),
                    parallel(2, 12),
                    parallel(2, 24),
                    parallel(2, 15),
                    
                    parallel(1, 2, "yes"),
                    parallel(2, 24, "yes"),
                    parallel(24, 2, "yes"),
                    parallel(10, 5, "yes"),),

                Family("parallel_b").add(
                    Complete(ic.psel() + "/blacklist:ccb"),
                    Variables(SCHOST= "ccb",
                              QUEUE= "os",
                              LOGHOST= "ccb-il2",
                              ECF_OUT= ct1logs,
                              LOGDIR=  ct1logs,),
                    parallel(1, 1),
                    parallel(1, 24),
                    parallel(2, 12),
                    parallel(2, 24),
                    parallel(2, 15),
                    
                    parallel(1, 2, "yes"),
                    parallel(2, 24, "yes"),
                    parallel(24, 2, "yes"),
                    parallel(10, 5, "yes"),),

                If(user == "emos",
                   Variables(LOGHOST= HCRAY,
                             LOGPORT= base + get_uid()),
                   Variables(LOGHOST= HCRAY,
                             LOGPORT= base + get_uid())),
                )
            ]      

        lxop_out = "/vol/lxop_emos_nc/output"
        if user == "emos" or host in ("ode", "eode", "ecf"):
            ic.SUBM = "/home/ma/emos/bin/trimurtu"
            sms2ecf.subm = "/home/ma/emos/bin/trimurti"

            out += (
                Variables(LOGPORT= 9316),
                unit("cca", "cca", "ns", "/sc1/tcwork/emos_dir", "oesu").add(
                    Variables(STHOST= "/c1",
                              LOGHOST= "cca"),
                    logsvr("ping"),
                    Complete(ic.psel() + "/blacklist:cca"),),

                unit("ecgb", "ecgb", "emos", "/vol/emos_nc/output"),
                unit("lxc", "lxc", "emos", "/vol/emos_nc/output"),
                unit("lxab", "lxab", "serial", "/vol/emos_nc/output"),
                unit("lxop", "lxop", "emos", lxop_out
                     ).add(Variables(ECF_OUT= lxop_out,
                                     ECF_LOGHOST= "lxop",
                                     ECF_LOGPORT= 9316,
                                     LOGDIR=  lxop_out, )),
                unit("sappa", "sappa", "serial", "/vol/emos_nc/output"),
                unit("sappb", "sappb", "serial", "/vol/emos_nc/output"),
                unit("opensuse131", "opensuse131", "serial", "/vol/emos_nc/output"),
                Family("hps").add(
                    Complete(ic.psel() + "/blacklist:hps"),
                    unit("acq", "acq", "emos", "/tmp/emos"),
                    unit("pp1", "pp1", "emos", "/tmp/emos"),
                    unit("pp2", "pp2", "emos", "/tmp/emos"),
                    unit("pp3", "pp3", "emos", "/tmp/emos"),
                    unit("pp4", "pp4", "emos", "/tmp/emos"),
                    unit("bilbo", "bilbo", "emos", "/tmp/emos"),
                    unit("hallas", "hallas", "emos", "/tmp/emos"),
                    # unit("itanium", "itanium", "emos", "/tmp/emos"),),
                unit("localhost", "localhost", "ns", "/tmp/emos/logs"),
                unit("ibis", "ibis", "ns", "/tmp/emos/logs"),
                ))
        else:
            rdir = "/tmp"
            out += (unit("localhost", "localhost", "ns", "/tmp/%s/logs" % user),
                     unit("lxab", "lxab", "serial", udir + "/logs"),
                     unit("lxop", "lxop", "emos", udir + "/logs"),
                     unit("ecgb", "ecgb", "ns", udir + "/logs"),
                     unit("lxc", "lxc", "ns", udir + "/logs"),         )
        return out

def maker():
    return Task("maker").add(
        Variables(ECF_FILES= wdir,
                  ECF_INCLUDE= "/home/ma/emos/def/o/include",
                  WSHOST= "ibis",
                  USER= "emos",
                  SCHOST= "ibis",
                  ECF_OUT= "/vol/lxop_emos_nc/output",),   )

def usage():
    print '''cray.py -h -u [user] -n [node] -s [suite-name] \
 -e: ecflow'''

if __name__ == "__main__":
    global user, udir, ecflow, host
    suites = { "cray": Cray,
               "test": Cray,
           }
    # suite = "test"; node = "eode"
    # argv = cli_proc.process(suites, selection=suite, host=node)
    user = get_username()
    try: host = sys.argv[-1]
    except: host= "localhost"
    ecflow  = False
    ecflow  = True
    suite   = None
    user = get_username()
    opts, args = getopt.getopt(
          sys.argv[1:], "hp:u:es:n:p:", 
          ["help", "port", "user", "ecflow", "suite", "node", "path"])
    print "#MSG: opts, args", opts, args

    output = None
    verbose = False
    for o, a in opts:
        if   o in ("-n", "--node"):   host = a
        elif o in ("-h", "--help"):   usage(); sys.exit()
        elif o in ("-s", "--suite"):  suite =  a
        elif o in ("-p", "--path"):   path =  a
        elif o in ("-e", "--ecflow"): ecflow = True
        elif o in ("-u", "--user"):
            user = a
        elif o in ("-p", "--port"):
            port = a
        else: print "#ERR: what?", o, a; assert False, "unhandled option"

    import cli_proc

    if suite is None: 
        cli_proc.ip.SELECTION = user
    elif '/' in suite:
        sel = suite
        if '/' == suite[0]: sel = sel[1:]
        if '/' in sel: sel, family = sel.split('/', 1)
        cli_proc.ip.SELECTION = sel
    else: cli_proc.ip.SELECTION = suite

    if ecflow and host not in ("ode", "map", "pikachu"):
        ic.ecf.ECF_MODE = "ecflow"
    else: 
        import sms2ecf
        sms2ecf.ECF_MODE = "sms"
        ic.ecf.ECF_MODE = "sms"

    print "#MSG: ecflow/sms: ", ic.ecf.ECF_MODE
    global wdir, rdir, fdef, jdir

    ########### SETTINGS ################
    udir = pwd.getpwnam(user).pw_dir
    wdir = udir + "/course/cray"
    rdir = "/sc1/tcwork/emos/logs"
    fdef = wdir + "/cray.def"
    jdir = "/tmp/%s/cray" % user
    ddef = udir + "/course/cray"
    fdef = ddef + "/cray.def"
    idir = "/home/ma/emos/def/cray/include"
    if user == "map" or host in ["map", ]:
        wdir = "/home/ma/map/course/cray"
    elif user == "emos" or host in ["ode", "od3", "eode"]:
        jdir="/vol/lxop_emos_nc/output"
        rdir="/vol/lxop_emos_nc/output"
        udir="/vol/lxop_emos_nc/output"

    print "##", host, suite
    argv = cli_proc.process(suites, selection=suite, host=host, proc=False)
    try:    os.makedirs(wdir)
    except: pass # one line on purpose
    try:    os.makedirs(jdir)
    except: pass

    create_task()
    create_smhi()
