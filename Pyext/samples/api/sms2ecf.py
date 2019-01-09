#!/usr/bin/env python
""" one module to store needed function to translate SMS-ECFLOW suites """
import sys
ECF_MODE = "ecflow" # "sms"

# RAW_MODE:
# False: translate *_CMD (submit, kill, status) variables values for %ECF_ + number args
# True:  just translate the name, and %ECF_, not arguments number

RAW_MODE = False    
# ECF_MODE = "sms"
CDP = "/usr/local/apps/sms/bin/cdp"
subm = "/home/ma/emos/bin/trimurti"
kill = subm
stat = subm

DICT_SMS_ECF = {
    'SMSNAME': 'ECF_NAME',
    'SMSNODE': 'ECF_HOST',
    'SMSPASS': 'ECF_PASS',
    'SMS_PROG': 'ECF_PORT',
    'SMSFILES': 'ECF_FILES',
    'SMSINCLUDE': 'ECF_INCLUDE',
    'SMSTRIES':   'ECF_TRIES',
    'SMSTRYNO': 'ECF_TRYNO',
    'SMSTRIE': 'ECF_TRIE',
    'SMSHOME': 'ECF_HOME',
    'SMSRID': 'ECF_RID',
    'SMSJOB': 'ECF_JOB',
    'SMSJOBOUT': 'ECF_JOBOUT',
    'SMSOUT': 'ECF_OUT',
    'SMSCHECKOLD': 'ECF_CHECKOLD',
    'SMSCHECK': 'ECF_CHECK',
    'SMSLOG': 'ECF_LOG',
    'SMSLIST': 'ECF_LIST',
    'SMSPASSWD': 'ECF_PASSWD',
    'SMSSERVER': 'ECF_SERVER',
    'SMSMICRO': 'ECF_MICRO',
    'SMSPID': 'ECF_PID',
    'SMSHOST': 'ECF_HOST',
    'SMSDATE': 'ECF_DATE',
    'SMSURL': 'ECF_URL',
    'SMSURLBASE': 'ECF_URLBASE',
    'SMSCMD': 'ECF_JOB_CMD',
    'SMSKILL': 'ECF_KILL_CMD',
    'SMSSTATUSCMD': 'ECF_STATUS_CMD',
    'SMSJOBCHECK': 'ECF_CHECK_CMD',
    'SMSURLCMD': 'ECF_URL_CMD',
    'SMSWEBACCES': 'ECF_WEBACCES',
    'SMS_VER': 'ECF_VER',
    'SMS_VERSION': 'ECF_VERSION',
    'SMSLOGHOST': 'ECF_LOGHOST',
    'SMSLOGPORT': 'ECF_LOGPORT',
    'SMSLOGTIMEOUT': 'ECF_LOGTIMEOUT',
}

DICT_ECF_SMS = dict((v, k) for k, v in DICT_SMS_ECF.items())
      
def translate(name, value=None):
    """ find and replace submit, kill and status command, according to
    sms/ecflow mode """
    value2 = value
    transl = name
    smssubmit = "trimurti"
    ecfsubmit = "/home/ma/emos/bin/ecflow_submit"

    def add_ext(name, value):
            if "_v2" in value: rc = " %ECF_JOB% %SCHOST% %USER%"
            elif "kill" in value or "stat" in value:
                if "WSHOST" in value: rc = RIDCLWS
                else: rc = RIDCL
            # if "submit" in value: 
            #     if "WSHOST" in value: rc = JOBCLWS
            #     else: rc = JOBCL
            elif "WSHOST" in value: rc = JOBCLWS
            else: rc = JOBCL

            if name in  ('ECF_JOB_CMD', 'SMSCMD'):
                 rc = SUBCMD + " " + rc 
            elif name in ('ECF_KILL_CMD', 'SMSKILL'):
                if "submit" in KILLCMD:
                    rc = KILLCMD + " %s kill" % rc
                else: rc = KILLCMD + " %s " % rc
            elif name in ('ECF_STATUS_CMD', 'SMSSTATUS', 
                           'ECF_CHECK_CMD', 'SMSJOBCHECK'):
                if "submit" in STATCMD:
                     rc = STATCMD + " %s status" % rc
                else: rc = STATCMD + " %s" % rc
            return rc

    if ECF_MODE == "ecflow" and name in DICT_SMS_ECF.keys():
        if name in ('SMSCMD', 'SMSKILL', 'SMSSTATUSCMD', 'SMSHOME'):
            value2 = value.replace('SMS', 'ECF_')
            value  = value2.replace(smssubmit, ecfsubmit)
        elif name in ('ECF_JOB_CMD', 'ECF_KILL_CMD', 'ECF_STATUS_CMD'):
            value2 = value.replace('SMS', 'ECF_')
            value  = value2.replace(smssubmit, ecfsubmit)
        transl = DICT_SMS_ECF[name]

    elif ECF_MODE == "sms"  and name in DICT_ECF_SMS.keys():
        if name in  ('ECF_CMD_CMD', 'ECF_KILL_CMD', 'ECF_STATUS_CMD'):
            value2 = value.replace('ECF_', 'SMS')  
        transl = DICT_ECF_SMS[name]

    else: pass

    if ECF_MODE == "sms" and RAW_MODE:
        value2 = value.replace("ECF_", "SMS")
        return transl, value2
 
    elif ECF_MODE == "sms" and "CMD" in name and not "submit" in value:
        if name in ('ECF_JOB_CMD'): name = "SMSCMD"
        elif name in ('ECF_KILL_CMD'): name = "SMSKILL"
        elif name in ('ECF_STATUS_CMD'): name = "SMSSTATUS"
        value2 = value.replace("ECF_", "SMS")
        value  = value2

    elif ECF_MODE == "sms":
             SUBCMD  = subm
             KILLCMD = kill
             STATCMD = stat

             JOBCL  = ' %USER% %SCHOST% %SMSJOB% %SMSJOBOUT%'
             RIDCL  = ' %USER% %SCHOST% %SMSRID% %SMSJOB% %SMSJOBOUT%'
             STATL  = ' %USER% %SCHOST% %SMSRID% %SMSJOB%'
             STATLWS  = ' %USER% %WSHOST% %SMSRID% %SMSJOB%'

             JOBCLWS = JOBCL.replace("%SC", "%WS")
             RIDCLWS = RIDCL.replace("%SC", "%WS") # .replace("HOST%", "HOST% %SMSRID%")
             if name in  ('ECF_JOB_CMD', 'SMSCMD'):
                 value2 = add_ext(name, value)
             elif name in ('ECF_KILL_CMD', 'SMSKILL'):
                 if "submit" in KILLCMD:
                     value2 = KILLCMD + add_ext(value, value) + " kill"
                 else: value2 = KILLCMD + add_ext(value, value)
             elif name in ('ECF_STATUS_CMD', 'SMSSTATUS', 
                           'ECF_CHECK_CMD', 'SMSJOBCHECK'):
                 if "submit" in STATCMD:
                     value2 = STATCMD + add_ext(value, value) + " status"
                 else: value2 = STATCMD + add_ext(value, value)
             return transl, value2
    return transl, value


def get(name, host="localhost", port=314159):
    import os, pwd
    os.getlogin = lambda: pwd.getpwuid(os.getuid())[0]
    user = os.getlogin()
    cmd = "setenv -i HOME USER; set SMS_PROG %d; login %s %s 1; status > /dev/null ; get; show > %s; " % (port, host, user, name)
    os.system("cdp -c '%s'" % cmd)

def ignore(): pass

ignored = ( "action", "owner", "text", "migrate", "automigrate", "autorestore")
def sms2ecf(orig, dest):
   import re
   fop = open(dest, 'w')
   with open(orig, 'r') as source:
        try:
            for line in source:
                for key in ignored:
                    if " %s " % key in line: 
                        continue                
                print(line, file=fop)
        except:
            print("oops")

def load(name, host="localhost", port=31415):
    import ecflow as ec
    client = ec.Client(host, port)
    try:
        client.load(name)
    except:
        client.replace("/", name)

if __name__ == "__main__":
    import sys
    try:
        sms  = sys.argv[1]
        prog = sys.argv[2]
        ecf  = sys.argv[3]
        port = sys.argv[4]
    except:
        sms  = "localhost"
        prog = 314159
        ecf  = "localhost"
        port = 31415

    before = "from_sms.exp"
    become = "prep_ecf.exp"
    get(name=before)
    sms2ecf(orig=before, dest=become)
    load(name=become)
