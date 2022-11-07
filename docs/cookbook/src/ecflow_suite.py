#!/usr/local/apps/python/current/bin/python
import getopt
import os
import pwd
# import sh
import sys
sys.path.append("/opt/cray/sdb/1.0-1.0502.50558.6.21.ari/lib64/py")
def get_username():
    return pwd.getpwuid( os.getuid() )[ 0 ]
def get_uid():
    return pwd.getpwnam(get_username()).pw_uid
"""
python docstring: demonstrate python cli + ecFlow task wrapper + task loader
$manual
DESCRIPTION:
sweeper:
find out min/max among modeleps_nemo tasks
OPERATORS: please, set complete if problematic
ANALYST: example as pure python task - job
$end
$comment
  comments can be added ...
$end
"""
PATH = "/usr/local/apps/ecflow/4.0.9/lib/python2.7/site-packages/ecflow"
sys.path.insert(0, PATH)
import ecflow as ec
MICRO = "$$" # double dollar to please ecFlow micro character balance
task_content = """#!/bin/ksh
#!/bin/bash # NOK while typeset -Z2 may be found... if [[ var =  @(v1) ]]; then ...
%include <trap.h>
inc=%ECF_HOME%/%SUITE%/%FAMILY%/preproc.job1
if [[ -f $inc ]]; then
  . $inc
# deterministic runs are lost:
\rm -f $inc || :
fi
xevent 1
i=0; while ((i < 100)) ; do xmeter step $i; ((i+=1)); done
xlabel info "done"
trap 0; xcomplete; exit 0
"""
inc_content = """# request from Blazej?
xevent 2
xlabel info "%SUITE% %TASK%"
"""
def create_wrapper(name, content):
    wrapper = open(name, "w")
    print >>wrapper, content
    wrapper.close()
def loader(test=1, # host=os.getenv("ECF_TEST_HOST", None),
           host=None, port=None, path=None):
    """ replace sweeper task in test/oper mode """
    sys.path.append("/home/ma/emos/def/o/def")
    import ecf
    from ecf import Suite, Family, Task, Variables, Trigger, Complete, Repeat, Defstatus, Event, Meter, Label, Limit, Inlimit
    if test:
        env = {"host": os.getenv("ECF_TEST_HOST", None),
               "port": os.getenv("ECF_TEST_PORT", None),
               "path": '/' + SUITE + "/submit", }
    else:
        env = {"host": os.getenv("ECF_OPER_HOST", None),
               "port": os.getenv("ECF_OPER_PORT", None),
               "path": '/' + SUITE + "/submit", }
    defs = ecf.Defs()
     
    if host: env["host"] = host
    if port: env["port"] = port
    if env["host"] != "eurus": print host, env["host"]; raise
    if path is None: path = env["path"]
    try: sname = str(path.split('/')[1])
    except: sname = "suite"
    try: fname = str(path.split('/')[2])
    except: fname = "submit"
    user = get_username()
    submit = "/home/ma/emos/bin/trimurti.41r2"
    rid = submit + " %USER% %HOST% %ECF_RID:0% %ECF_JOB% $ECF_JOBOUT% "   
    ecf_home = pwd.getpwnam(user).pw_dir + "/ecflow_server"
    ecf_files = ecf_home + "/smsfiles"       
    ecf_include = ecf_home + "/include"
               
    client=" ECF_PASS=%ECF_PASS% ECF_NAME=%ECF_NAME% ecflow_client "
    hosts = ["cca", "ccb", "cct", "ecgb", "lxc", "opensuse131", "localhost"]
    suite = ecf.Suite(sname).add(
        Defstatus("suspended"),
        Variables(ECF_JOB_CMD= submit + " %USER% %HOST% %ECF_JOB% %ECF_JOBOUT%",
                  ECF_KILL_CMD= rid + "kill",
                  ECF_STATUS_CMD= rid + "status",
                  ECF_CHECK_CMD= rid + "check",
                  USER = user,
                  ECF_HOME= ecf_home,
                  ECF_FILES= ecf_files,
                  ECF_INCLUDE= ecf_include,
                  ECF_EXTN= ".sms",
                  HOST="localhost", LOGDIR= "%ECF_HOME%", SMSOUT= "%ECF_HOME%", # trap.h
                  ),
        Family("limits").add(Defstatus("complete"),
                             Label("memo", "use click-mouse-3-Edit to leave a message"),
                             Label("begin", "Edit-Pref-Admin + suite-node-click3-Begin"),
                             Label("example", "four cases: no job, no submit - no job but submit - job created but no submit - job + submit"),
                             Limit("tasks", 10)),
        Inlimit("/%s/limits:tasks" % sname),
         
        Task("dummy").add(Variables(ECF_DUMMY_TASK= 1)),
        Family("ssh_login").add(
            Label("one_liner", "dummy script, no need for job, execute ECF_JOB_CMD"),
            Repeat("HOST", start=hosts, kind="enumerated"),
            Variables(ECF_JOB_CMD= client + "--init;" +
                      # "xterm -T %HOST% -e ssh %HOST%;" +
                      "ssh %HOST% pwd;" +
                      client + "--complete;"),
            Task("simple"),
        ),
        Family("preprocess").add(
            Task("preproc").add(
                Variables(ECF_JOB_CMD= client + " --init;"
                          + client + "--complete")),
            Task("simple").add(
                Label("info", ""),
                Event(2),
                Trigger("preproc eq complete")),
        ),
        Family("submit").add(
            Trigger(["ssh_login", ]),
            [ Family(host).add(
                Variables(HOST= host),
                Task("simple").add(Event(1),
                                   Meter("step", -1, 100, 90),
                                   Label("info", "")),
            ) for host in hosts ]
        ),
        Family("process").add(
            Label("info", "outer-follower-family + inner-run-sometime example"),
            Label("stop", "at first abort"),
            Trigger("./process ne aborted"),
            Family("daily").add(
                Repeat("YMD", 20160101, 20321212, kind="date"),
                Task("simple"),
                Family("decade").add(
                    Label("info", "Show-Icons-Complete"),
                    Complete("../daily:YMD % 10 ne 0"),
                    Task("simple")),
            ),
            Family("monthly").add(
                Trigger("monthly:YM lt daily:YMD / 100 or daily eq complete"),
                Repeat(name="YM", start=
                       [ "%d" % ( YM)
                         for YM in range(201601, 203212+1)
                         if (YM % 100) < 13],
                       kind="enum"),
                Task("simple"),
                Family("odd").add(
                    Complete("../monthly:YM % 2 eq 0"),
                    Task("simple")),
),
            Family("yearly").add(
                Repeat("Y", 2016, 2032, kind="integer"),
                Trigger("yearly:Y lt daily:YMD / 10000 or daily eq complete"),
                Task("simple"),
                Family("decade").add(
                    Complete("../yearly:Y % 10 ne 0"),
                    Task("simple")),
                Family("century").add(
                    Complete("../yearly:Y % 100 ne 0"),
                    Task("simple")),
            )
        )
    )
    # sh.mkdir(ecf_files);     sh.mkdir( ecf_include);     sh.ln("-sf ~emos/def/o/trap.h %s" % ecf_include)
    os.system(""
+ "mkdir %s;" % ecf_files
+ "mkdir %s;" % ecf_include
+ "ln -sf ~emos/def/o/include/trap.h %s;" % ecf_include # trap + init + lxop .running
              + "touch %s/rcp.h;" % ecf_include
              + "touch %s/env_setup.h;" % ecf_include
# +"ln -sf ~emos/def/o/include/env_setup.h %s;" % ecf_include # link PBS job to .running
# +"ln -sf ~emos/def/o/include/rcp.h %s;" % ecf_include # rapatriate remote job@completion
    )
    create_wrapper(ecf_files + "/simple.sms", task_content)
    create_wrapper(ecf_files + "/preproc.sms", inc_content)
    defs.add_suite(suite)
    print "#MSG: replacing", path, env["host"], env["port"]
    ecf.Client(env["host"], env["port"]).replace(path, defs, 1, 1)
class Seed(object):
    """ find min/max meter value below a node """
    def __init__(self, host=None, port=None, path=None, task=None):
        import signal
        if host is None:
            host = os.getenv("ECF_NODE", "$ECF_NODE$")
        if port is None and not MICRO[0] in host:
            try: port = int(os.getenv("ECF_PORT", "$ECF_PORT$"))
            except: port = 31415
        if host and port:
            self.clt = ec.Client(host, port)
        else: self.clt = None
        self.cl2 = None
        if path is None:
            if "$SUITE$" != "/admin" and not "SUITE" in "$SUITE$":
                self.path = ( "/$SUITE$/$FAMILY$/%s" % legs[0],
                              "/$SUITE$/$FAMILY$/%s" % legs[1],
                          )
            else: self.path = MC_STEPS_LIST
        else: self.path = (path, )
        if task is None:
            self.task = TARGET_TASK
        else: self.task = task
        # shall make sense when processed into job by ecflow
        # name remains when not processed...
        ecfv = {"ECF_NODE": "$ECF_NODE$",
                "ECF_PASS": "$ECF_PASS$",
                "ECF_NAME": "$ECF_NAME$",
                "ECF_PORT": "$ECF_PORT$",
                "ECF_TRYNO": "$ECF_TRYNO$",        }
        for key, val in ecfv.items():
            if key in val:
                ecfv[key] = os.getenv(key, None)
        if ecfv["ECF_NODE"] and ecfv["ECF_NAME"]:
            print "#MSG will communicate with server..."
            print "#kill: ssh %s kill -15 %d" % (ecfv["ECF_NODE"], os.getpid())
            self.cl2 = ec.Client()
            self.cl2.set_host_port(ecfv["ECF_NODE"], ecfv["ECF_PORT"])
            self.cl2.set_child_pid(os.getpid())
            self.cl2.set_child_path(ecfv["ECF_NAME"])
            self.cl2.set_child_password(ecfv["ECF_PASS"])
            self.cl2.set_child_try_no(int(ecfv["ECF_TRYNO"]))
            self.cl2.child_init()
            self.cl2.set_child_timeout(20)
            for sig in (signal.SIGINT,
                        signal.SIGHUP,
                        signal.SIGQUIT,
                        signal.SIGILL,
                        signal.SIGTRAP,
                        signal.SIGIOT,
                        signal.SIGBUS,
                        signal.SIGFPE,
                        signal.SIGUSR1,
                        signal.SIGUSR2,
                        signal.SIGPIPE,
                        signal.SIGTERM,
                        signal.SIGXCPU,
                        signal.SIGPWR):
                signal.signal(sig, self.signal_handler)
    def signal_handler(self, signum, frame):
        """ catch signal """
        print 'Aborting: Signal handler called with signal ', signum
        self.cl2.child_abort("Signal handler called with signal " + str(signum))
    def report(self, msg, meter=None):
        """ communicate with ecFlow server """
        if not self.cl2:
            return
        if meter:
            self.cl2.child_meter(msg, meter)
        elif msg == "stop":
            self.cl2.child_complete()
            sys.exit(0)
        else: self.cl2.child_label("info", msg)
def usage():
    """ help """
    print """client
  -o: operational node is to be replaced, default is test node,
      provide this option BEFORE -r
  -r: replace task node
  -h: this help
ECF_NODE=localhost ECF_PORT=31415 ./ecflow_suite.py -s start
"""
if __name__ == '__main__':
    try:
        OPTS, ARGS = getopt.getopt(
            sys.argv[1:], "hros:t:n:p:",
            ["help", "replace", "oper", "task", "suite", "node", "port"])
    except getopt.GetoptError as err:
        print "# what?", usage()
        sys.exit(2)
    TEST = 1
    PATH = None
    TASK = None
    SUITE = "start"
    PORT = None
    NODE = None
    for o, a in OPTS:
        if o in ("-r", "--replace", ):
            loader(TEST, host=NODE, port=PORT, path=TASK)
            sys.exit(0)
        # elif o in ("-p", "--path", ): PATH = a
        elif o in ("-o", "--oper", ):
            TEST = 0
        elif o in ("-t", "--task", ):
            TASK = a
        elif o in ("-s", "--suite", ): SUITE = a
        elif o in ("-n", "--node", ): NODE = a
        elif o in ("-p", "--port", ): PORT = a
"""
python ecflow_suite.py -s suite -p 1630 -n eurus -r
"""