#!/usr/bin/env python
import sys, os, ih
from ecflow import *
from inc_hostc import *

PROJECT = "skull"
SELECTION = PROJECT
# SELECTION = "e%s"%PROJECT
if SELECTION == "skull":    FIRST_DAY = 0
elif SELECTION == "eskull": FIRST_DAY = 1

USER = os.getenv("USER")
SCRATCH = os.getenv("SCRATCH")
HOME = os.getenv("HOME")
HOST = os.getenv("HOST")
PWD = os.getenv("PWD")

GROUP = ih.shcmd("id -gn")
SCHOST = "c1a"
WSHOST = ih.shcmd("uname -n")
CLHOST = "lxa"

if USER == "map":
    WSQUEUE = "serial"
    WS_QUEUE_EPILOG = "ns"
    SCQUEUE = "ns"
    SC_QUEUE_EPILOG = "ns"
    ACCOUNT = "ecodmdma"
elif USER == "emos":
    WSQUEUE = "serial"
    WS_QUEUE_EPILOG = "ns"
    SCQUEUE = "ns"
    SC_QUEUE_EPILOG = "ns"
    ACCOUNT = "oesu"
elif USER[1:2] == "tr":
    WSQUEUE = "serial"
    WS_QUEUE_EPILOG = "ns"
    SCQUEUE = "ns"
    SC_QUEUE_EPILOG = "ns"
    ACCOUNT = "ectrain"
else:
    WSQUEUE = "serial"
    WS_QUEUE_EPILOG = "ns"
    SCQUEUE = "ns"
    SC_QUEUE_EPILOG = "ns"
    ACCOUNT = "decmsaf"

def def_fam():
    def_suspended()
WSLOGDIR = SCRATCH+"/output"
ih.shcmd("mkdir -p %s/%s" %(WSLOGDIR,SELECTION))
SCLOGDIR = "/s1a/ms_dir/%s/%s/smsjoboutput" % (GROUP, USER)
WDIR = "/s1a/ms_dir/%s/%s/wdir" % (GROUP, USER)
VERSION = 0040
FIRST_DATE = ih.shcmd("\date +%Y%m%d")
LAST_DATE = ih.shcmd("newdate -D %s 1" % (FIRST_DATE))
QUEUE_EPILOG = "ns"

def onws():
    var('QUEUE', WSQUEUE)
    var('QUEUE_EPILOG', WS_QUEUE_EPILOG)
    var('LOGDIR', WSLOGDIR)
    var('SMSOUT', WSLOGDIR)
    var('WSHOST', HOST)

def oncl():
    var('QUEUE', WSQUEUE)
    var('QUEUE_EPILOG', WS_QUEUE_EPILOG)
    var('LOGDIR', WSLOGDIR)
    var('SMSOUT', WSLOGDIR)
    var('WSHOST', "linux_cluster")
    var('WSHOST', CLHOST)

def onsc():
    var('QUEUE', SCQUEUE)
    var('QUEUE_EPILOG', SC_QUEUE_EPILOG)
    var('LOGDIR', SCLOGDIR)
    var('SMSOUT', SCLOGDIR)
    var('WSHOST', SCHOST)

def onecg():
    var('QUEUE', "normal")
    var('QUEUE_EPILOG', "normal")
    var('LOGDIR', WSLOGDIR)
    var('SMSOUT', WSLOGDIR)
    var('WSHOST', "ecgate")

def day():
    # weekdey
    # edit WEEKDAY $*; label weekday $* # CONV
    var('WEEKDAY', "$*")

from inc_skull import *
import dinner 
import barber
import weekly 
USE_SMS = 1
USE_ECF = 1
if  not USE_SMS and  not USE_ECF:
    print "it cannot be"
    abort()

def create_suite():
    print SELECTION
    s = suite(SELECTION)
    DEFS = Defs()
    DEFS.add_suite(s)
    
    def_suspended()
    var('USER', USER)
    var('SCHOST', SCHOST)
    var('WSHOST', WSHOST)
    var('CLHOST', CLHOST)
    var('WSLOGDIR', WSLOGDIR)
    var('SCLOGDIR', SCLOGDIR)
    var('DATEMASK', "*.*.*")
    var('QUEUE_EPILOG', QUEUE_EPILOG)
    HOMEDIR = HOME+"/sms_server/"+SELECTION
    HOMEDIR = PWD
    if USE_SMS:
        var('SMSLOGHOST', SCHOST)
        var('SMSLOGPORT', 9316)
        var('SMSTRIES', 1)
        var('SMSHOME', SCRATCH)
        var('SMSFILES', HOMEDIR+"/smsfiles")
        var('SMSINCLUDE', HOMEDIR+"/include")
        var('SMSCMD', "/home/ma/emos/bin/smssubmit %USER% %WSHOST% %SMSJOB% %SMSJOBOUT%")
        var('SMSKILL', "/home/ma/emos/bin/smskill %USER% %WSHOST% %SMSRID% %SMSJOBOUT%")
    if USE_ECF:
        var('ECF_LOGHOST', SCHOST)
        var('ECF_LOGPORT', 9316)
        var('ECF_TRIES', 1)
        var('ECF_HOME', SCRATCH)
        var('ECF_FILES', HOMEDIR+"/smsfiles")
        var('ECF_INCLUDE', HOMEDIR+"/include")
        var('ECF_JOB_CMD', "/home/ma/emos/bin/smssubmit %USER% %WSHOST% %ECF_JOB% %ECF_JOBOUT% ")
        var('ECF_KILL_CMD', "/home/ma/emos/bin/smskill %USER% %WSHOST% %ECF_RID% %ECF_JOBOUT% ")
    # endif
    var('ACCOUNT', ACCOUNT)
    var('VERSION', VERSION)
    var('EPSVERSION', "0001")
    var('RFVERSION', "0001")
    var('DELTA_DAY', 0)
    var('EMOS_BASE', "00")
    var('USE_YMD', "true")
    var('FSFAMILY', "/")
    var('BINS', HOME)
    var('XBINS', "/home/ma/emos/bin")
    var('STREAM', "undef")
    var('EPSTYPE', "undef")
    var('MEMBER', 0)
    var('FIRST_DAY', FIRST_DAY)
    var('EMOS_TIME_STEP_H', 00)
# repeat date YMD    $FIRST_DATE $LAST_DATE
    var('YMD', FIRST_DAY)
    var('  SUITE_START', FIRST_DAY)
    family("limits")
    def_complete()
    limit("lim", 3)
    endfamily() #

    family("make")
    complete("/%s:YMD ne /%s:SUITE_START" % (SELECTION, SELECTION))
    family("wst")
    onws()
    call_skull_make()
    endfamily() #
    family("ecg")
    onecg()
    call_skull_make()
    endfamily() #
    family("hpc")
    onsc()
    call_skull_make()
    endfamily() #
    family("lcl")
    oncl()
    if USE_SMS:
        var('SMSCMD', "/home/ma/emos/bin/smssubmit.new %USER% %WSHOST% %SMSJOB% %SMSJOBOUT% ")
    if USE_ECF:
        var('ECF_JOB_CMD', "/home/ma/emos/bin/smssubmit.new %USER% %WSHOST% %ECF_JOB% %ECF_JOBOUT% ")
    call_skull_make()
    endfamily()
    endfamily()

    family(PROJECT)
    def_fam()
    families = '00 12'
    var('ENSEMBLES', 50)
    trigger("make==complete")
    for fam in qw(families):
        family(fam)
        if fam == "00":
            var('DELTA_DAY', 1)
        else:
            var('DELTA_DAY', 0)
        var('EMOS_BASE', fam)
        call_skull(PROJECT)

        family("pop")
        oncl()
        trigger("./"+PROJECT+"==complete")
        call_pop_skull()
        endfamily("pop")

        family("ws")
        onws()
        trigger("./"+PROJECT+"==complete")
        call_pop_skull()
        endfamily("ws")
        endfamily(fam)

    endfamily(PROJECT)

    family("consumer")
    def_fam()
    import consumer
    consumer.call_consumer(SELECTION)
    endfamily("consumer")

    onws()
    var('SLEEP', 20)

    dinner.call_dinner()

    barber.call_barber_shop()

    # weekly.call_weekly(SELECTION, FIRST_DATE)

    task("perl")
    def_suspended()
    if USE_SMS:
        var('SMSMICRO', "^")
        var('SMSCMD', "^SMSJOB^ > ^SMSJOBOUT^ 2>&1")
    if USE_ECF:
        var('ECF_MICRO', "^")
        var('ECF_JOB_CMD', "^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1")
        meter( "step", -1, 100)

    task("python")
    def_suspended()
    if USE_SMS:
        var('SMSMICRO', "^")
        var('SMSCMD', "^SMSJOB^ > ^SMSJOBOUT^ 2>&1")
    if USE_ECF:
        var('ECF_MICRO', "^")
        var('ECF_JOB_CMD', "^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1")
    meter( "step", -1, 100)
    endsuite(SELECTION)
    return DEFS

def expand(defs, play=True, load=True):
    file_name = PROJECT + ".dump"
    f = open(file_name, "w")
    print >>f, defs
    # load into sms
    if play:
        prog=''
        cmd = "cdp -c 'play -l %s'"% (file_name)
        print cmd
        os.system(cmd)

    if play:
        if   USER == "map":  prog = "set SMS_PROG 900130;"
        elif USER == "emos": prog = "set SMS_PROG 314159;"
        else: 
            g = 90000 + int(GROUP)
            prog = "set SMS_PROG %s;" % (g)
        cmd = "cdp -c '%s login %s %s 1; play -r /%s %s'" \
            % (prog, HOST, USER, PROJECT, file_name)
        print cmd
        os.system(cmd)

    # load into ecflow
    ECF = "/usr/local/apps/ecflow/current/bin/ecflow_client"
    if load:
        port = ''
        if   USER == "map":  port = "--port 20130 --host ibis"
        elif USER == "emos": port = "--port  3199 --host ibis"

        cmd = "%s %s --load %s" % (ECF, port, file_name)
        print cmd
        res = os.system(cmd)
        if res != 0:
            cmd = "%s %s --replace /%s %s" % (ECF, port, PROJECT, file_name)
            print cmd
            os.system(cmd)

if __name__ == "__main__":
    defs = create_suite()
    expand(defs, play=True, load=True)
sys.exit(0)
