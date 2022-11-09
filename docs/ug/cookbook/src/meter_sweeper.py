#!/usr/bin/env python

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
import time
import os.path
import getopt
import sys

PATH = "/usr/local/apps/ecflow/4.0.9/lib/python2.7/site-packages/ecflow"
sys.path.insert(0, PATH)
import ecflow as ec

MICRO = "$$"  # double dollar to please ecFlow micro character balance
TARGET_TASK = "modeleps_nemo"  # use option -t to overwrite
TARGET_METER = "step"  # would this need an option???
MC_STEPS_LIST = (
    "/mc/main/12/legA/fc",  # use option -p for single path
    "/mc/main/12/legB/fc",
    "/mc/main/18bc/legA/fc",
    "/mc/main/00/legA/fc",
    "/mc/main/00/legB/fc",
    "/mc/main/06bc/legA/fc",
    "/mofc/thu/01/legC/fc",
    "/mofc/thu/hind/14/back",
    "/mofc/mon/01/legC/fc",
    "/mofc/mon/hind/14/back",
)


def focus_below(node):
    """continue down below nodes which name is expected ;
    please update for external use...
    """
    return (
        node.name() in ("00", "12", "18bc", "06bc", "legA", "legB", "pf", "cf", "fc")
        or "main" == node.name()
    )


def process(item, path=None, low=999, high=-1, task=None):
    """track nodes to follow and update min/max"""
    if type(item) == str:
        defs = ec.Defs(item)
    elif isinstance(item, ec.Client):
        item.sync_local()
        defs = item.get_defs()
    elif isinstance(item, ec.Defs):
        defs = item
    else:
        defs = None

    if defs:
        for node in defs.suites:
            if node.name() != str(path.split("/")[1]):
                continue
            status = "%s" % node.get_state()
            if node.is_suspended() or status == "unknown":
                continue
            for item in node.nodes:
                low, high = process(item, path, low, high, task)

    elif isinstance(item, ec.Family):
        for node in item.nodes:
            if focus_below(node):
                return process(node, path, low, high, task)
            low, high = process(node, path, low, high, task)

    elif isinstance(item, ec.Task):
        node = item
        path2node = item.get_abs_node_path()
        status = "%s" % node.get_state()

        # ignore cal-val # ecmwf specific
        if "/cv/" in path2node or not task in item.name():
            return low, high
        if "/cv/" in path2node and (high > 240 or low < -1):
            high = 240
            return low, high

        stamp = ""
        evt = ""
        meter = ""
        if node.is_suspended():
            status = "suspended"
        try:
            for att in node.meters:
                meter = "%s" % att.value()
                if att.name() != TARGET_METER:
                    continue
                if att.value() > high:
                    high = att.value()
                elif att.value() < low:
                    low = att.value()

        except Exception as excpt:
            print("#! problem with line:", excpt)
        # name = "%20s %s %5s %5s" % (path2node, stamp, evt, meter)

    else:
        print(type(item))

    return low, high


def create_task():
    """add ecFlow task into a family..."""
    sys.path.append("/home/ma/emos/o/def")
    import ecf

    steps = 360
    return ecf.Task("sweeper").add(
        ecf.Variables(
            ECF_FILES=os.getenv("ECF_FDI", None),
            ECF_EXTN=".py",
            ECF_JOB_CMD="ssh -x $WSHOST$ $ECF_JOB$ > $ECF_JOBOUT$ 2>&1 &",
            ECF_MICRO=MICRO[0],
        ),
        ecf.Label("info", ""),
        ecf.Defcomplete(),
        ecf.Meter("min", -1, steps, steps),
        ecf.Meter("max", -1, steps, steps),
    )


def loader(test=1, host=os.getenv("ECF_TEST_HOST", None)):
    """replace sweeper task in test/oper mode"""
    sys.path.append("/home/ma/emos/o/def")
    import ecf

    if test:
        env = {
            "host": os.getenv("ECF_TEST_HOST", None),
            "port": os.getenv("ECF_TEST_PORT", None),
            "path": "/admine/steps",
        }
    else:
        env = {
            "host": os.getenv("ECF_OPER_HOST", None),
            "port": os.getenv("ECF_OPER_PORT", None),
            "path": "/admin/steps",
        }
    defs = ecf.Defs()

    path = env["path"]
    sname = str(path.split("/")[1])
    fname = str(path.split("/")[2])

    suite = ecf.Suite(sname).add(
        ecf.Extern(MC_STEPS_LIST, defs),
        ecf.Family(fname).add(
            create_task().add(
                ecf.Cron("04:30 23:59 03:00"),
                ecf.Trigger("==active or ".join(MC_STEPS_LIST) + "==active"),
                ecf.Variables(WSHOST=host, QUEUE="test"),
            )
        ),
    )

    defs.add_suite(suite)

    print("#MSG: replacing", path, env["host"], env["port"])
    ecf.Client(env["host"], env["port"]).replace(path, defs, 1, 1)


class Sweep(object):
    """find min/max meter value below a node"""

    def __init__(self, host=None, port=None, path=None, task=None):
        import signal

        if host is None:
            host = os.getenv("ECF_NODE", "$ECF_NODE$")
        if port is None:
            port = int(os.getenv("ECF_PORT", "$ECF_PORT$"))
        self.clt = ec.Client(host, port)
        self.cl2 = None
        if path is None:
            self.path = MC_STEPS_LIST
        else:
            self.path = path
        if task is None:
            self.task = TARGET_TASK
        else:
            self.task = task

        # shall make sense when processed into job by ecflow
        # name remains when not processed...
        ecfv = {
            "ECF_NODE": "$ECF_NODE$",
            "ECF_PASS": "$ECF_PASS$",
            "ECF_NAME": "$ECF_NAME$",
            "ECF_PORT": "$ECF_PORT$",
            "ECF_TRYNO": "$ECF_TRYNO$",
        }

        for key, val in ecfv.items():
            if key in val:
                ecfv[key] = os.getenv(key, None)
        if ecfv["ECF_NODE"] and ecfv["ECF_NAME"]:
            print("#MSG will communicated with server...")
            print("#kill: ssh %s kill -15 %d" % (ecfv["ECF_NODE"], os.getpid()))
            self.cl2 = ec.Client()
            self.cl2.set_host_port(ecfv["ECF_NODE"], ecfv["ECF_PORT"])
            self.cl2.set_child_pid(os.getpid())
            self.cl2.set_child_path(ecfv["ECF_NAME"])
            self.cl2.set_child_password(ecfv["ECF_PASS"])
            self.cl2.set_child_try_no(int(ecfv["ECF_TRYNO"]))
            self.cl2.child_init()
            self.cl2.set_child_timeout(20)

            for sig in (
                signal.SIGINT,
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
                signal.SIGPWR,
            ):
                signal.signal(sig, self.signal_handler)

    def signal_handler(self, signum, frame):
        """catch signal"""
        print("Aborting: Signal handler called with signal ", signum)
        self.cl2.child_abort("Signal handler called with signal " + str(signum))

    def report(self, msg, meter=None):
        """communicate with ecFlow server"""
        if not self.cl2:
            return
        if meter:
            self.cl2.child_meter(msg, meter)
        elif msg == "stop":
            self.cl2.child_complete()
            sys.exit(0)
        else:
            self.cl2.child_label("info", msg)

    def update(self):
        """refresh status tree asking ecFlow server"""
        num = 20
        leg = "legA"
        for path in self.path:
            first = 1
            low = 0
            high = 0
            self.clt.ch_register(False, [str(path.split("/")[1])])
            if "/back" in path:
                path += "/%02d/%s" % (num, leg)

            while 1:
                clt = self.clt
                status = ""
                try:
                    if clt.news_local() or first:  # has the server changed
                        clt.sync_local()
                        first = 0
                        defs = clt.get_defs()
                        node = defs.find_abs_node(path)
                        if node:
                            status = "%s" % node.get_state()
                        low, high = process(node, self.path, 999, -1, self.task)
                        sleep = 30
                    else:
                        sleep = 90
                    msg = "... %ds %s %s %d %d" % (sleep, path, status, low, high)
                    print(msg)
                    if type(low) != type(high):
                        print(type(low), type(high))

                    if status == "complete":
                        if leg == "legB":
                            num -= 1
                            leg = "legA"
                        else:
                            leg = "legB"
                        break

                    elif (
                        low == high
                        and status in ("queued", "aborted", "complete")
                        or node is None
                    ):
                        if node:
                            msg = "# %s %s" % (node.get_abs_node_path(), status)
                            print(msg)
                            self.report(msg)
                        break
                    else:
                        self.report(msg)
                        self.report("min", low)
                        self.report("max", high)
                    sys.stdout.flush()
                    time.sleep(sleep)
                except RuntimeError as exp:
                    print(str(exp))


def usage():
    """help"""
    print(
        """client
  -o: operational node is to be replaced, default is test node,
      provide this option BEFORE -r
  -r: replace task node
  -p: path to look below, by default internal list MC_STEPS_LIST will be used
  -h: this help
 
ECF_NODE=localhost ECF_PORT=31415 ./client.py --path /mc/main/18bc/legA/fc -e
 
"""
    )


if __name__ == "__main__":
    try:
        OPTS, ARGS = getopt.getopt(
            sys.argv[1:], "hep:rot:", ["help", "ens", "path", "replace", "oper", "task"]
        )
    except getopt.GetoptError as err:
        print("# what?", usage())
        sys.exit(2)

    TEST = 1
    PATH = None
    TASK = None
    for o, a in OPTS:
        if o in (
            "-e",
            "--ens",
        ):
            Sweep(path=PATH).update()
            sys.exit(0)
        elif o in (
            "-r",
            "--replace",
        ):
            loader(TEST)
            sys.exit(0)
        elif o in (
            "-p",
            "--path",
        ):
            PATH = a
        elif o in (
            "-o",
            "--oper",
        ):
            TEST = 0
        elif o in (
            "-t",
            "--task",
        ):
            TASK = a
    CLT = Sweep(path=PATH, task=TASK)
    CLT.update()
    CLT.report("stop")
