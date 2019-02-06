#!/usr/bin/env python
from __future__ import print_function
# -m trace --count -C .
"""an example for a simple python client to help ecFlow scheduling
 + unit test
 + task loading in the suite

one script DO-IT-ALL:
  - play suite + update suite holder aka def-file

  - script as a wrapper for a task

  - download content (passive client)

  - upload statuses (active client)

$manual

  MIRRORING suite from server to server

  test - suspend suite and rerun job if aborted... Thanks

$end
$comment
  a comment in a comment
$end

TODO: check MICRO robust sync
"""
import argparse
import os
import pwd
import sys
import time
import unittest
from time import gmtime, strftime

ECFLOWP = "/usr/local/apps/ecflow/current"
ECFLOWC = ECFLOWP + "/bin/ecflow_client "
sys.path.append(ECFLOWP + "/lib/python2.7/site-packages/ecflow")
child = None


def getreq():
    return Family("getreq").add(Task("collectreq"))


def ymd():
    return [Edit("YMD", "29090909"), Label("ymd", "29090909")]


def excepthook(exctype, value, traceback):
    if exctype == KeyboardInterrupt:
        if child:
            child.report("abort", "keyb")
    else:
        sys.__excepthook__(exctype, value, traceback)
        if child:
            child.report("abort", "gen")


def fullname(item):
    return ""

sys.excepthook = excepthook
from ecf import *  # mirror
import ecf
import ecflow


# class Edit(ecf.Variables):    pass

MICRO = "$$"  # keep ecFlow pleased with micro character balance
DEBUG = 1
DEBUG = 0
USER = "emos"
# USER = "map"

sms_status = {-1: "unknown",
              0: "unknown",
              1: "suspended",
              2: "complete",
              3: "queued",
              4: "submitted",
              5: "active",
              6: "aborted",
              7: "shutdown",
              8: "halted",
              9: "unknown", }
sms_type = {13: "definition",
            12: "suite",
            11: "family",
            10: "task",
            32: "alias", }


############################
def usage():
    print(sys.argv[0], """
      -h # help
      -m <server@port> # mirror path fri
      -t # unit test
      -r # load the task definition in a suite, associated with -p option
      -r # with -m, allow suite loading in destination
      -u usage

can also be used as a module: gen_task, gen_suite

    """)
    for num, val in enumerate(sys.argv):
        print(num, val)
    child.report("abort")
    sys.exit(2)


############################
def get_uid():
    return pwd.getpwnam(get_username()).pw_uid


############################
def gen_task(var=True, load_only=False, kind=None):
    from ecf import Task, Label, Variables
    pwd = os.getcwd()
    name = sys.argv[0]
    if '/' in name:
        name = name.split('/')[-1]
    if '.' in name:
        name = name.split('.')[0]
    rec = "00:30"
    cmd = "/home/ma/emos/bin/trimurti emos eurus $ECF_JOB$ $ECF_JOBOUT$"
    cmd = "/usr/local/apps/python/2.7.12-01/bin/python $ECF_JOB$"
    WDIR = pwd
    ODIR = pwd

    load = (Label("info", "mirror suites..."),
            Variables(ECF_MICRO=MICRO[0],
                      ECF_HOME=ODIR,
                      ECF_EXTN=".py",
                      ECF_JOB_CMD=cmd, ))
    if load_only:
        return load
    return Task(name).add(
        Cron("04:00 20:00 " + rec),
        If(var, load))


############################
# def stop(msg, num, child=None):
#     print(msg);
#     if child:
#         child.report(msg)
#         if num == 0: child.report("complete")
#     sys.stdout.flush()
#     sys.stderr.flush()
#     sys.exit(num)
def timer(a, b):
    if child is None:
        return
    child.report("timeout")
    child.report("complete")
import signal
signal.signal(signal.SIGALRM, timer)
signal.alarm(180)


############################
class Child(object):
    """ facilitate job behaviour with ecFlow server:
    just instanciate and call complete when finished

    obj = Child(); obs.report("complete")

    this does nothing when script is called from command line
    """

    def __init__(self):
        import signal
        env = {"ECF_HOST": "$ECF_HOST:$",
               "ECF_NODE": "$ECF_NODE:$",
               # check can be on a server, child on another
               "ECF_PASS": "$ECF_PASS$",
               "ECF_NAME": "$ECF_NAME$",
               "ECF_PORT": "$ECF_PORT:0$",
               "ECF_TRYNO": "$ECF_TRYNO$", }
        self.client = None
        if MICRO[0] in env["ECF_PORT"]:
            print("#MSG: cli mode")
            return
        print("#MSG: will communicate with server...")
        print("#MSG: kill: ssh %s kill -15 %d" % (os.uname()[1], os.getpid()))
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
        self.set_client()

    def set_client(self):
        self.client = ecflow.Client()
        if len(MICRO) == 2:
            host = env["ECF_HOST"]
            print(host)
            if host == "" or "%" in host:
                host = env["ECF_NODE"]
            self.client.set_host_port(host, int(env["ECF_PORT"]))
            self.client.set_child_pid(os.getpid())
            self.client.set_child_path(env["ECF_NAME"])
            self.client.set_child_password(env["ECF_PASS"])
            self.client.set_child_try_no(int(env["ECF_TRYNO"]))
        else:
            host = "$ECF_HOST:$"
            print(host)
            if not (host != "" and "%" not in host):
                host = "$ECF_NODE:$"
            self.client.set_host_port(host, int("$ECF_PORT:0$"))
            self.client.set_child_pid(os.getpid())
            self.client.set_child_path("$ECF_NAME$")
            self.client.set_child_password("$ECF_PASS$")
            self.client.set_child_try_no(int("$ECF_TRYNO$"))
        self.client.child_init()

        self.client.child_label("info", "%s" % strftime("%Y-%m-%d %H:%M:%S",
                                                        gmtime()))
        self.client.set_child_timeout(20)
        print(self.client)


    def signal_handler(self, signum, frame):
        """ catch signal """
        print('Aborting: Signal handler called with signal ', signum)
        self.report("abort", "Signal handler called with signal " + str(signum))

    def __exit__(self, exc_type, exc_value, traceback):
        self.report("abort", "__exit__")

    def report(self, msg, meter=None):
        """ communicate with ecFlow server """
        if not self.client:
            print("#MSG: no child")
            if msg in ("stop", "complete"):
                sys.stdout.flush()
                sys.stderr.flush()
                print("#MSG: no child stop")
                sys.exit(0)
            else:
                print(msg, meter)
            return
        elif msg in ("stop", "complete"):
            self.client.child_complete()
            self.client = None
            sys.stdout.flush()
            sys.stderr.flush()
            print("#MSG: stop")
            sys.exit(0)
        elif msg in ("abort", ):
            self.client.child_abort()
            self.client = None
            raise Exception(msg)
        elif meter:
            self.client.child_meter(msg, meter)
        else:
            self.client.child_label("info", msg)


############################
destinations = {
    "test": "${DEST_HOST:=ecgate}@${DEST_PORT=31415}",
}

def gen_suite(host=None, port=None, path=None):
    import ecf
    from ecf import (Task, Label, Variables, Suite, Defs,
                     Family, Trigger, Defstatus, Meter, Event)
    destinations = dict()
    defs = Defs()
    sname = "mirror"
    if DEBUG:
        print(definitions.keys())

    print(definitions.keys())
    print(destinations.keys())
    defs.add_suite(
        Suite(sname).add(
            ecf.Defstatus("suspended"),
            Limit("one", 2),
            Inlimit("one"),
            gen_task(load_only=True),
            Variables(ALL_ECF=" "),
                      # ALL_SMS=" ".join(all_sms)            
            # Variables(DESTINATIONS= "vsms3@31415 vsms1@433334"),          #
            # eurus@1630"),
            [Family(name).add(
                Variables(DESTINATIONS=destinations[name]),
                Label("info", ""),
                gen_task(0, kind=name), )
             for name in definitions.keys()
             # if name in destinations.keys()
             ]))
    if DEBUG:
        print(defs)
    return defs



definitions = {  # strings as path for all nodes below sync
    # or suites, subset of the original, leaves are task...
    # or else not sync'ed
    # shall we add list of string/path ???

    "test": Suite("test").add(
        Defstatus("suspended"),
        Family("f1").add(
            Edit(YMD=20320101),
            Task("t1"))), 
}

############################


class Mirror(object):

    """
$nopp
$end
    """

    def _protect_oper(self, server):
        if server == "vsms1@32112":
            if 1:
                child.report("abort", "protect")
            elif ("/o" in self.path or
                  self.path == "all" or
                  "/eda" in self.path or
                  "/mc" in self.path):
                raise Exception("# wont do that...")

    def _warmup(self):
        self._set_register(self.path)
        if not self.is_sms:
            self.source.ch_register(False, self.register)
            self.source.sync_local()

    def _send(self, msg="", client=None, send=False):
        """ self.grouped is None: no buffering, immediate send
            msg accumulate (send False by default)
            msg is sent ("", client, send=True)
        """
        if self.grouped is None:
            print("#MSG: send", self.grouped)
            client.group(msg)
            time.sleep(self.snap)
            return

        self.grouped += msg
        if send and self.grouped != "":
            print("#MSG: send", self.grouped)
            client.group(self.grouped)
            client.sync_local()
            self.grouped = ""
            time.sleep(self.snap)

    def __init__(self, server="eurus@1630", path="/e_45r1", replay=False, is_sms=False):
        self.is_sms = is_sms
        self.replay = replay  # allow replay from skel definition
        self.act = 1         # active ie force change allowed
        self.snap = 1        # sleep interval
        self.wait = 0  # refresh interval,continue as long as sub/act node exist
        self.path = path
        self.grouped = None  # would use atomic alter
        self.grouped = ""  # command to send to the server ... grouped
        self._set_register(path)

        if server == "all":
            self.servers = "$DESTINATIONS$".split(" ")
        elif " " in server:
            self.servers = server.split(" ")
        elif "@" in server:
            host, port = server.split("@")
            self.servers = ("%s@%s" % (host, port), )
        elif ":" in server:
            host, port = server.split(":")
            self.servers = ("%s:%s" % (host, port), )
        if self.servers[-1] == "":
            del self.servers[-1]
        host = os.getenv("ECF_HOST", "$ECF_HOST:none$")
        if MICRO[0] in host or host == "none":
            host = os.getenv("ECF_NODE", "$ECF_NODE$")
        # if MICRO[0] in host: host = "localhost"
        port = os.getenv("ECF_PORT", "$ECF_PORT$")
        if MICRO[0] in port:
            port = 1500 + int(get_uid())
        # when started with ecflow_start.sh
        # aka reference/source  server name
        self.sname = "%s:%s" % (host, port)

        if "$SOURCE:$" != "" and MICRO[0] not in "$SOURCE:$":
            self.sname = "$SOURCE:$"
        if DEBUG:
            print("#DBG: source is", self.sname)
        self.set_source()

    def set_source(self):
        self.source = ecflow.Client(self.sname)
        self._warmup()

    def __exit__(self):
        if type(self.source) == ecflow.Client:
            try:
                self.source.ch_drop()
            except:
                pass

    def _set_register(self, path):
        if '/' == path[0]:
            suite = str(path.split('/')[1])
            self.register = [suite, ]

        elif path == "all":
            self.register = skel.keys()

        else:
            child.report("abort", "register")

    def _check(self, server, client, stop=0):
        if client == self.source:
            stop = 1

        for suite in self.register:
            top = client.get_defs().find_abs_node("/" + suite)

            if top is None:
                print("#WAR: node not found", suite, server, )
                if stop:
                    child.report("abort", "check")
                elif self.act and server != self.sname:
                    self._protect_oper(server)
                    if self.replay:
                        if type(definitions[suite]) == str:
                            print("#WAR: not yet")
                            child.report("abort", "str")
                        print("replaying")

                        defs = ecflow.Defs()
                        defs.add_suite(definitions[suite])
                        host = "localhost"
                        client.load(defs)
                        client.suspend('/' + suite)
                        client.begin_suite(suite)
                        client.sync_local()
                    else:
                        print("replay?")
                else:
                    pass

            elif server != self.sname and not (
                    top.is_suspended() or  # normal backup server
                    "%s" % top.get_state() == "unknown"):  # test server
                print("#ERR: top node shall be suspended - NOGO")
                print("#ERR:", server, top.name(), top.get_state())
                if 1:
                    child.report("abort", "state")
                print("#MSG: stop")
                child.report("abort", "state")  # sys.exit(1)

            else:
                pass

            if suite not in definitions.keys():
                raise Exception(definitions.keys(), suite)

            if type(definitions[suite]) == str:
                path = definitions[suite]

            elif suite in self.path:
                path = self.path

            else:
                return

            node = client.get_defs().find_abs_node(path)
            if node:
                return
            print("#WAR: node not found", path, server)
            if stop:
                child.report("abort", "stop")

    def _update_var(self, var, nfrom, ndest, server, client, kind="edit"):
        name = var.name()
        vfrom = None
        vdest = None
        repeat = nfrom.get_repeat()
        print("############# %s" % repeat, nfrom.get_abs_node_path())
        if repeat and repeat.name() == name:
            vfrom = repeat
        else:
            for vfrom in nfrom.variables:
                if vfrom.name() == name:
                    break

        for vdest in ndest.variables:
                if vdest.name() == name:
                    break
        pnode = ndest.get_abs_node_path()
        pnode = nfrom.get_abs_node_path()

        if vfrom is None:
            return

        elif vfrom.name() != name:
            return

        if vdest is None:
            return

        vname = vdest.name()
        vval = vdest.value()
        if vname != name:
            return

        if vfrom.name() == vname and str(vfrom.value()) != str(vval) or FORCE:
            if type(vfrom) == Repeat:
                self.action(ndest.get_abs_node_path(), server, client,
                            kind="repeat", value=str(vfrom.value()),
                            msg="repeat requeue")
            else:
                self.action(pnode, server, client, kind=name,
                            value=str(vfrom.value()), msg="value update")

    def process(self, server=None, client=None, item=None, var=False):
        if server is None:
            self._check(self.sname, self.source, 1)

            for server in self.servers:
                server = server.replace('@', ':')
                if DEBUG:
                    print("#DBG: target is", server)
                host, port = server.split(':')

                if int(port) < 65536 and 0 == comm(
                        ECFLOWC + "--ping --port %s --host %s" % (port, host), False):
                    client = ecflow.Client(server)
                    client.ch_register(False, self.register)
                    client.sync_local()
                    self._check(server, client)
                    for suite in self.register:
                        node = definitions[suite]
                        if type(node) == str:
                            node = self.source.get_defs().find_abs_node(node)
                        # try: node.add(Label("memo", "This suite was generated by mirror.py"))
                        # except: pass # used to sync, too late
                        # update variables
                        self.process(server, client, node, "repeat")
                        # self.process(server, client, node) # update statuses
                        # self.process(server, client, node, "edit") # update
                        # variables
                    client.ch_drop()

                else:
                    # child.report("abort","res")
                    print("server is not responding")

            if self.wait > 0:
                print("#MSG: start again... snap ", self.wait)
                time.sleep(self.wait)
                self.wait = 0
                self.source.sync_local()
                self.process()
            return

        elif server == self.sname:
            child.report("abort", "same name")  # DONT

        elif client is None:
            child.report("abort", "no client")  # DONT

        elif type(item) in (ecflow.Suite, ecf.Suite):

            if 0: pass
            else:
                # suite =
                # client.get_defs().find_abs_node(item.get_abs_node_path())
                suite = client.get_defs().find_abs_node(item.fullname())
                # print(type(client), item.fullname(), type(suite))
                if suite:
                    if not suite.is_suspended():
                        print("#ERR: top node shall be suspended - stop",
                              suite, server)
                        return
                    for node in suite.nodes:
                        self.process(server, client, node, var)
            return

        elif type(item) in (ecflow.Family, ecflow.Task):
            pnode = item.get_abs_node_path()
            nfrom = self.source.get_defs().find_abs_node(pnode)
            if 0: pass
            else:
                ndest = client.get_defs().find_abs_node(pnode)

            if nfrom is None:
                if var:
                    return  # ignore
                print("#ERR: node not found, please update mirror.py skel",
                      pnode, self.sname)
                child.report("abort", "nfrom")

            elif ndest is None:
                if var:
                    return  # ignore
                print("#ERR: node not found, replay?", pnode, server)
                return

            else:
                pass
            if nfrom is not None:
                status = "%s" % nfrom.get_state()
            else:
                raise Exception(pnode, self.source, "nfrom is not")

            if 0: pass
            else:
                dstate = "%s" % ndest.get_state()
            is_task = type(item) == ecf.Task

            for evs in nfrom.events:
                for evd in ndest.events:
                    # if evd.name_or_number() != evs.name_or_number(): continue
                    if evd.name() == "" and evd.number() != evs.number():
                        continue
                    if evd.value() == evs.value():
                        break
                    print("#event", evs.name(), evd.number(), evs.number(),
                          evd.value(), evs.value())
                    if evs.value():
                        if evs.name() == "":
                            self._send("alter change event %d set %s;" %
                                       (evs.number(), pnode), client)
                            # client.alter(pnode, "change", "event", "%d" %
                            # evs.number(), "set")
                        else:
                            self._send("alter change event %s set %s;" %
                                       (evs.name(), pnode), client)
                        # else: client.alter(pnode, "change", "event",
                        # evs.name(), "set")

            for evs in nfrom.meters:
                for evd in ndest.meters:
                    if evd.name() != evs.name():
                        continue
                    if evd.value() == evs.value():
                        break
                    print("#meter", evd.value(), evs.value())
                    try:
                        value = evs.value()
                        if value > 100:
                            value = 100
                        if value > 0:
                            self._send("alter change meter %s %s %s;" % (
                                evs.name(), value, pnode), client)
                        # client.alter(pnode, "change", "meter", evs.name(),
                        # "%s" % evs.value())
                    except:
                        pass

            if status != dstate or FORCE:
                print("#WAR: diff ", pnode, status, dstate, )
                if status == "complete":
                    self.action(pnode, server, client, status, is_task,
                                msg="forced complete")
                    if not is_task:
                        self._send("", client, send=True)
                        return

                elif status == "queued":
                    # self._send("force queued %s;" % (pnode), client)
                    self.action(pnode, server, client, status, is_task,
                                msg="forced queued")
                elif status in ("submitted", "active", "unknown", "aborted", ):
                    print("#IGN: status is active/submit, ignored")
                else:
                    raise Exception(status)

            if type(item) in (ecflow.Suite, ecflow.Family):
                for node in item.nodes:
                    self.process(server, client, node, var)

            if var:
                for edit in item.variables:
                    self._update_var(edit, nfrom, ndest, server, client, var)

            self._send("", client, send=True)
        else:
            raise Exception(type(item), item)

    def action(self, pnode, server, client, kind, is_task=False,
               value="20010101",
               msg=""):
        if not self.act:
            return

        self._protect_oper(server)

        print("#MSG:", msg)
        if kind in ("active", "submitted", ):
            return
        elif FORCE:
            pass  # SMS ONLY

        elif kind == "complete":
            if is_task:
                self._send("", client, True)
                client.force_state(pnode, ecflow.State.complete)
            else:
                self._send("", client, True)
                client.force_state_recursive(pnode, ecflow.State.complete)
        elif kind == "queued":  # and is_task:
            self._send("force queued %s;" % pnode, client)

        elif kind == "repeat":
            raise Exception

        else:  # var
            if DEBUG:
                print("#DBG: update var", kind, value)
            if kind in ("queued", ):
                pass
            elif kind not in ("YMD", "JUL", "DAY", "SCHOST", "STHOST", 
                              "YYYY", "MM", "ECF_FILES",
                              "INIBEGINDATE", "MIXTASK"):
                raise Exception(kind)
            if 0: pass
            else:
                client.requeue(pnode)
                self._send("alter change variable %s %s %s;" %
                           (kind, value, pnode), client, True)

# ignore Family + queued while is might just be that we dont mirror all
# fam/tasks and inheritance get it already complete in dest while still
# queued in source


############################
def replay(path, defs=None):
    import ecf
    miss = "@undef@"
    host = os.getenv("ECF_HOST", miss)
    if host == miss:
        host = os.getenv("ECF_NODE", "localhost")
    port = os.getenv("ECF_PORT", 31415)
    client = ecf.Client(host, port)
    if defs is None:
        defs = gen_suite(host, port, path)
    if DEBUG:
        print(defs)
    # print(defs)
    if "localhost" in host and port in ("3141", 3141) and ("/test" in path) and "/mirror" not in path:
        raise Exception("dont")

    print("#MSG: replacing %s in " % path, host, port, defs)
    client.replace(path, defs, 1, 1)
    return client


def comm(cmd, rem=True):
    import commands
    if rem and cmd in memo.keys():
        return memo[cmd]
    (rc, res) = commands.getstatusoutput(cmd)
    print("#COMM:", rc, res, cmd)
    if rem:
        memo[cmd] = rc
    return rc


############################
class TestMirror(unittest.TestCase):
    """ a test case """

    def test_1(self, test_ok=1):
        sname = "mirror"
        replay('/' + sname, gen_suite())


memo = dict()
############################
if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-?", default=0, help="help")
    parser.add_argument("-f", "--force", action="store_true", help="force")
    parser.add_argument("-h", default=0, help="help")
    parser.add_argument(
        "-m", "--mirror", default="localhost@31415", help="mirror")
    parser.add_argument("-p", "--path", default="/mirror/test", help="path")
    parser.add_argument("-r", "--replay", action="store_true", help="replay")
    parser.add_argument("-t", "--test", action="store_true", help="test")
    parser.add_argument("-u", "--usage", action="store_true", help="test")

    parsed = parser.parse_args()
    global FORCE
    FORCE = parsed.force
    print(parsed.path, parsed.mirror, parsed.replay,
          parsed.sms, parsed.force, parsed.test)
    # raise Exception
    child = Child()

    if parsed.usage:
        print("""

* mirror (slow real-time) a source suite from a source server to a destination server
* mirror suite can be hosted on the source, on the destination or another server
* this script was used in ecflow course to demonstrate
  * one script that can be used from the command line
  * defines a suite
  * play/replace a suite/node
  * a script can be used as task wrapper/template to generate jobs
  * an ecflow client (passive) to download status/value from source
  * an ecflow client (active) to change status/value on destination

* update mirror.py: destination variable with the triplet suite host port

* update mirror.py: definition with the key(suite) and the minimum suite-tree to mirror

* update mirror.py: replay add "if" for "protection" not to overwrite source suite by mistake

* download a copy from the source suite
  ecflow_client --port ${ECF_PORT:3141} --host ${ECF_HOST:localhost} --get > tmp.def

* load the mirror suite on the source server
  SUITE=/test ECF_HOST=${ECF_HOST:=localhoast} ECF_PORT=${ECF_PORT:=3141} ./mirror.py -p /mirror/$SUITE -r # load

* play (suspended) mirrored suite in destination
  ECF_HOST=${DEST_HOST:=ecgate} ECF_PORT=${DEST_PORT:=31415} ./mirror.py -m $DEST_HOST@$DEST_PORT -p /$SUITE

""")
        sys.exit(2)

    if not MICRO[0] in "$ECF_PORT$":
        parsed.mirror = "$DESTINATIONS$"
        parsed.path = "/$FAMILY1$"
        print("#MSG: ssh %s kill -9 %d" % (os.uname()[1], os.getpid()))

    if parsed.test:
        sys.argv.pop()
        unittest.main()

`    elif parsed.path and parsed.replay:
        print("#MSG: path and replay", parsed.path, parsed.replay)
        port = int(os.getenv("SMS_PROG", 0))
        s = parsed.path.split('/')[1]
        defs = None
        if "mirror" not in parsed.path:
            defs = Defs()
            defs.add_suite(definitions[s])
        replay(parsed.path, defs)

    elif parsed.mirror and parsed.path:
        print(parsed.mirror, "#", parsed.path, parsed.replay, type(Mirror))
        mirror = Mirror(parsed.mirror, parsed.path, parsed.replay, parsed.sms)
        print("#MSG: step")
        mirror.process()
        print("#MSG: step")
        child.report("complete")
        print("#MSG: step")

    else:
        usage()
    print("#MSG: step")
    child.report("complete")
"""
$nopp
  export ECF_HOST=localhost ECF_PORT=$((1500 + $(id -u))) SUITE=/test
  ecflow_client --port $ECF_PORT --host $ECF_HOST --get > tmp.def
  ./mirror.py -p /mirror/$SUITE -r # load

  ECF_HOST=${DEST_HOST:=localhost} ECF_PORT=${DEST_PORT:=31415} ./mirror.py -m $DEST_HOST@$DEST_PORT -p /$SUITE

/tmp/map/work/git/ecflow/Pyext/samples/mirror.py
$end
"""
