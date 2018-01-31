#!/usr/bin/env python
from __future__ import print_function
# -m trace --count -C .
"""an example for a simple python client to help ecFlow scheduling
 + unit test
 + task loading in the suite
$manual

  MIRRORING suite from server to server

  test - suspend suite and rerun job if aborted... Thanks

$end
$comment
  a comment in a comment
$end

"""
import getopt
import os
import pwd
import sys
import time
import unittest
from time import gmtime, strftime

from ecf import *  # mirror
import ecf
import ecflow

import signal
ECFLOWP = "/usr/local/apps/ecflow/current"
ECFLOWC = ECFLOWP + "/bin/ecflow_client"
sys.path.append(ECFLOWP + "/lib/python2.7/site-packages/ecflow")
child = None


def getreq():
    return Family("getreq").add(Task("collectreq"))


def ymd():
    return (Edit("YMD", "29090909"), Label("ymd", "29090909"))


def excepthook(exctype, value, traceback):
    if exctype == KeyboardInterrupt:
        if child:
            child.report("abort", "keyb")
    # elif exctype == NameError:
    #     if child: child.report("abort", "name")
    else:
        sys.__excepthook__(exctype, value, traceback)
        if child:
            child.report("abort", "gen")


def fullname(item):
    if type(item) == cdp.sms_node:
        if item.name != '/':
            return fullname(item.parent) + '/' + item.name
    return ""


sys.excepthook = excepthook
CDP = None


class Edit(ecf.Variables):
    pass


MICRO = "$$"  # keep ecFlow pleased with micro character balance
DEBUG = 1
DEBUG = 0
USER = "emos"
XPING = "/usr/local/apps/sms/bin/smsping"
x_status = {-1: "unknown",
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
x_type = {13: "definition",
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
    if kind is None:
        pass
    cmd = "trimurti $USER$ $WSHOST$ $ECF_JOB$ $ECF_JOBOUT$"
    WDIR = pwd
    ODIR = pwd

    load = (Label("info", "mirror suites..."),
            Variables(ECF_MICRO=MICRO[0],
                      ECF_FILES=WDIR,
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
    child.report("timeout")
    child.report("complete")


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
        self.client = ecflow.Client()
        if MICRO[0] == MICRO:
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
        print('Aborting: Signal handler called with signal ', signum)
        if self.client:
            self.client.child_abort(
                "Signal handler called with signal " + str(signum))

    def __exit__(self, exc_type, exc_value, traceback):
        if self.client:
            self.client.child_abort()

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
            if 1:
                raise Exception(msg)
        elif meter:
            self.client.child_meter(msg, meter)
        else:
            self.client.child_label("info", msg)


############################
def gen_suite(host=None, port=None, path=None):
    import ecf
    from ecf import (Task, Label, Variables, Suite, Defs,
                     Family, Trigger, Defstatus, Meter, Event)
    destinations = dict()
    defs = Defs()
    if 1:
        dest = "localhost@3141"
        if path:
            if path[0] == '/':
                sname = path.split('/')[1]
                print(sname)
                if sname in definitions.keys():
                    defs.add_suite(
                        definitions[sname].add(
                            Defstatus("suspended")))
                    return defs

    sname = "mirror"
    if DEBUG:
        print(definitions.keys())
    defs.add_suite(
        Suite(sname).add(
            ecf.Defstatus("suspended"),
            Limit("one", 2),
            Inlimit("one"),
            gen_task(load_only=True),
            Variables(ALL_ECF="localhost:31415",
                      ALL_SMS="",
                      [Family(name).add(
                          Variables(DESTINATIONS=destinations[name]),
                          Label("info", ""),
                          gen_task(0, kind=name), )
                          for name in definitions.keys()])))
    if DEBUG:
        print(defs)
    return defs


def o5hres(name="gsup"):
    return Family(name).add(
        Family("an").add(
            Family("main").add(
                Family("ed00").add(
                    Label("info", "YMD"),
                    Edit("YMD", "21000101"),  # USE_YMD
                    Task("an"),
                    Task("fc")))))


definitions = {  # strings as path for all nodes below sync
    # or suites, subset of the original, leaves are task...
    # or else not sync'ed
    # shall we add list of string/path ???
    "aaa": "/aaa/run/ext",

    "test": Suite("test").add(
        Defstatus("suspended"),
        gen_fcgroup1(name="abcd")), }
############################


class Mirror(object):

    """
  $nopp

  $end
    """

    def _protect_oper(self, server):
        if server == "xxx@1234":
            if 1:
                child.report("abort", "protect")
            elif ("/o" in self.path
                  or self.path == "all"
                  or "/eda" in self.path
                  or "/mc" in self.path):
                raise Exception("wont do that...")

    def _warmup(self):
        self._set_register(self.path)
        self.source.ch_register(False, self.register)
        self.source.sync_local()

    def __init__(self, server="eurus@1630", path="/e_41r2", replay=False):
        self.replay = replay  # allow replay from skel definition
        self.act = 1         # active ie force change allowed
        self.snap = 1        # sleep interval
        self.wait = 0  # refresh interval,continue as long as sub/act node exist
        self.path = path

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

        if "$SOURCE:$" != "":
            self.sname = "$SOURCE:$"
        if DEBUG:
            print("#DBG: source is", self.sname)
        self.source = ecflow.Client(self.sname)
        self._warmup()

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
                        client.load(defs)
                        client.suspend('/' + suite)
                        client.begin_suite(suite)
                        client.sync_local()
                    else:
                        print("replay?")
                else:
                    pass

            elif server != self.sname and not (
                    top.is_suspended()  # normal backup server
                    or "%s" % top.get_state() == "unknown"):  # test server
                print("#ERR: top node shall be suspended - NOGO")
                print("#ERR:", server, top.name(), top.get_state())
                if 1:
                    child.report("abort", "state")
                print("#MSG: stop")
                child.report("abort", "state")  # sys.exit(1)

            else:
                pass

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

    def update_var(self, var, nfrom, ndest, server, client, kind="edit"):
        if type(var) == cdp.sms_variable:
            name = var.name
        else:
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

        if type(ndest) == cdp.sms_node:
            vdest = ndest.variable
            while vdest:
                if vdest.name == name:
                    break
                vdest = vdest.next
            pnode = fullname(ndest)
        else:
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

        if type(vdest) == cdp.sms_variable:
            vname = vdest.name
            vval = vdest.value
        else:
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
                        ECFLOWC + " --ping --port %s --host %s" % (port, host)):
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

                elif 0 == comm("SMS_PROG=%s %s %s" % (port, XPING, host)):
                    port = int(port)
                    os.putenv('SMS_PROG', "%d" % port)
                    os.environ['SMS_PROG'] = "%d" % port
                    cdp.cdp_init0()
                    passw = "1"
                    tout = 60
                    top = cdp.sms_cdp_client_login(
                        host, USER, passw, tout, port)
                    if top is None:
                        print("#ERR: node not found", host, port, USER, passw)
                        return

                    for suite in self.register:
                        node = definitions[suite]
                        if type(node) == str:
                            node = self.source.get_defs().find_abs_node(node)
                        # node.add(Label("memo", "This suite was generated by
                        # mirror.py"))
                        self.process(server, top, node, "repeat")
                        # self.process(server, top, node)
                        # self.process(server, top, node, "edit")

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

            if isinstance(client, cdp.sms_node):
                suite = cdp.sms_node_find(item.get_abs_node_path(), client)
                if suite:
                    if not sms_status[suite.status] == "suspended":
                        print("#ERR: top node shall be suspended - stop",
                              suite, server)
                        return
                    node = suite.kids
                    while node:
                        # print(node); print(node.name, server, client, var)
                        self.process(server, client, node, var)
                        node = node.next
            else:
                suite = client.get_defs().find_abs_node(item.get_abs_node_path())
                print(type(client), item.get_abs_node_path(), type(suite))
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
            if isinstance(client, cdp.sms_node):
                ndest = cdp.sms_node_find(pnode, client)
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
            if nfrom:
                status = "%s" % nfrom.get_state()
            else:
                print(pnode, self.source)
                raise Exception

            if isinstance(ndest, cdp.sms_node):
                dstate = sms_status[ndest.status]
            else:
                dstate = "%s" % ndest.get_state()
            is_task = type(item) == ecf.Task

            for evs in nfrom.events:
                if isinstance(ndest, cdp.sms_node):
                    break  # TODO

                for evd in ndest.events:
                    if evd.name() == "" and evd.number() != evs.number():
                        continue
                    if evd.value() == evs.value():
                        break
                    print("#event", evs.name(), evd.number(), evs.number(),
                          evd.value(), evs.value())
                    if evs.value():
                        if 0:
                            client.alter(pnode, "change", "event",
                                         evs.name_or_number(), "set")
                        elif evs.name() == "":
                            client.alter(pnode, "change", "event",
                                         "%d" % evs.number(), "set")
                        else:
                            client.alter(pnode, "change", "event",
                                         evs.name(), "set")

            for evs in nfrom.meters:
                if isinstance(ndest, cdp.sms_node):
                    break  # TODO

                for evd in ndest.meters:
                    if evd.name() != evs.name():
                        continue
                    if evd.value() == evs.value():
                        break
                    print("#meter", evd.value(), evs.value())
                    try:
                        client.alter(pnode, "change", "meter",
                                     evs.name(), "%s" % evs.value())
                    except:
                        pass

            if status != dstate or FORCE:
                print("#WAR: diff ", pnode, status, dstate, )
                if status == "complete":
                    self.action(pnode, server, client, status, is_task,
                                msg="forced complete")
                    if not is_task:
                        return
                elif status == "queued":
                    self.action(pnode, server, client, status, is_task,
                                msg="forced queued")
                elif status in ("submitted", "active", "unknown", "aborted", ):
                    print("status is active/submit, ignored")
                else:
                    print(status)
                    raise Exception

            if type(item) in (ecflow.Suite, ecflow.Family):
                for node in item.nodes:
                    self.process(server, client, node, var)

            if var:
                for edit in item.variables:
                    self.update_var(edit, nfrom, ndest, server, client, var)

        elif type(item) == cdp.sms_node:
            pnode = fullname(item)
            nfrom = self.source.get_defs().find_abs_node(pnode)
            ndest = cdp.sms_node_find(pnode, client)
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
            if nfrom:
                status = "%s" % nfrom.get_state()
            else:
                status = "unknown"
            dstate = "%s" % x_status[ndest.status]
            is_task = x_type[item.type]

            if status != dstate or FORCE:
                print("#WAR: diff ", pnode, status, dstate, )
                if status == "complete":
                    self.action(pnode, server, client, status, is_task,
                                msg="forced complete")
                    if not is_task:
                        return

                elif status == "queued":
                    if is_task:
                        self.action(pnode, server, client, status, is_task,
                                    msg="forced queued")
                    else:
                        pass
                elif status in ("submitted", "active", "aborted", "unknown"):
                    print("status is active/submit, ignored")
                else:
                    print(status, dstate)
                    raise Exception

            if var:
                edit = item.variable
                while edit:
                    self.update_var(edit, nfrom, ndest, server, client, var)
                    edit = edit.next

            if x_type[item.type] == "task":
                return
            node = item.kids
            while node:
                self.process(server, client, node, var)
                node = node.next

        else:
            print(type(item))
            raise Exception

    def action(self, pnode, server, client, kind, is_task=False, value="20010101",
               msg=""):
        if not self.act:
            return

        self._protect_oper(server)

        print("#MSG:", msg)
        if type(client) == cdp.sms_node:
            host, port = server.split(':')
            CDP = "/usr/local/apps/sms/bin/cdp -q -c "
            cmd = CDP + "'set SMS_PROG %s; login %s %s 1;" % (port, host, USER)
        if kind in ("active", "submitted", ):
            return
        elif FORCE:
            pass

        elif kind == "complete":
            if is_task:
                if DEBUG:
                    print("#DBG: force complete")
                if type(client) == cdp.sms_node:
                    cmd += "force complete %s; exit 0;'" % pnode
                    rc = comm(cmd)
                else:
                    client.force_state(pnode, ecflow.State.complete)
            else:
                if DEBUG:
                    print("#DBG: force rec complete")
                if type(client) == cdp.sms_node:
                    cmd += "force -r complete %s; exit 0;'" % pnode
                    rc = comm(cmd)
                else:
                    client.force_state_recursive(pnode, ecflow.State.complete)
                    client.sync_local()

        elif kind == "queued":  # and is_task:
            if DEBUG:
                print("#DBG: force queued")
            if type(client) == cdp.sms_node:
                cmd += "force queued %s; exit 0;'" % pnode
                rc = comm(cmd)
            else:
                client.force_state(pnode, ecflow.State.queued)

        elif kind == "repeat":
            raise Exception

        else:  # var
            if DEBUG:
                print("#DBG: update var", kind, value)
            if kind in ("queued", ):
                pass
            elif kind not in ("YMD", "JUL", "DAY", "SCHOST", "STHOST",
                              "YYYY", "MM", "INIBEGINDATE", "MIXTASK"):
                raise Exception(kind)
            if type(client) == cdp.sms_node:
                cmd0 = cmd
                if kind in ("YMD", ):
                    cmd += "requeue %s; " % (pnode)
                cmd += "alter -V %s:%s %s; exit 0;'" % (pnode, kind, value)
                rc = comm(cmd)
                cmd += "alter -l %s:%s %s; exit 0;'" % (pnode, kind, value)
                rc = comm(cmd)
            else:
                if kind in ("YMD", ):
                    client.requeue(pnode)
                client.alter(pnode, "change", "variable", kind, value)
                try:
                    client.alter(pnode, "change", "label", kind, value)
                except:
                    pass

        time.sleep(self.snap)

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
    print("#MSG: replacing %s in " % path, host, port, defs)
    client.replace(path, defs, 1, 1)
    return client


def comm(cmd, rem=1):
    import commands
    if rem and cmd in memo.keys():
        return memo[cmd]
    (rc, res) = commands.getstatusoutput(cmd)
    print("#COMM:", rc, res, cmd)
    if rem:
        memo[cmd] = rc
    return rc


def sms_replay(path, defs=None):
    import commands
    import ecf
    miss = "@undef@"
    host = os.getenv("ECF_HOST", miss)
    if host == miss:
        host = os.getenv("SMSNODE", "localhost")
    prog = os.getenv("SMS_PROG", 314159)
    deff = "mirror.tmp"
    sname = path.split('/')[1]
    if defs is None:
        defs = Defs()
        if type(definitions[sname]) == str:
            server = "localhost:31415"
            client = ecflow.Client(server)
            client.ch_register(False, [sname, ])
            client.sync_local()
            defs = client.get_defs()
        else:
            defs.add_suite(definitions[sname])
        if DEBUG:
            print(defs)
        fd = open(deff, "w")
        print(defs, file=fd)
        fd.close()
    cmd = "cdp -c 'set SMS_PROG %s; login %s %s 1; play -r %s %s; exit 0'" % (
        prog, host, USER, path, deff)
    rc = comm(cmd)


############################
class TestMirror(unittest.TestCase):
    """ a test case """

    def test_1(self, test_ok=1):
        replay('/' + sname, gen_suite())


memo = dict()
############################
if __name__ == '__main__':
    try:
        OPTS, ARGS = getopt.getopt(
            sys.argv[1:], "fhi:m:n:p:rstw",
            ["force", "help", "interval", "mirror", "number", "path", "replay",
             "sms", "test", "wait", ])
    except getopt.GetoptError as err:
        print("#what?", usage())

    global FORCE
    FORCE = False  # FORCE = True
    MIRROR = None
    WAIT = False
    PATH = None
    NUM = None
    INTERVAL = 30
    REPLAY = False
    ECFLOW = True

    for o, a in OPTS:
        if o in ("-h", "--help"):
            usage()
        elif o in ("-f", "--force"):
            FORCE = True
        elif o in ("-i", "--interval"):
            INTERVAL = int(a)
        elif o in ("-m", "--mirror"):
            MIRROR = a
        elif o in ("-n", "--number"):
            NUM = int(a)
        elif o in ("-p", "--path"):
            PATH = a
        elif o in ("-r", "--replay"):
            REPLAY = True
        elif o in ("-s", "--sms"):
            ECFLOW = False
        elif o in ("-t", "--test"):
            unittest.main(argv=[sys.argv[0]])
            sys.exit(0)
        elif o in ("-w", "--wait"):
            WAIT = True

    child = Child()
    if not MICRO[0] in "$ECF_PORT$":
        MIRROR = "$DESTINATIONS$"
        PATH = "/$FAMILY1$"
        print("#MSG: ssh %s kill -9 %d" % (os.uname()[1], os.getpid()))

    if PATH and REPLAY:
        print("#MSG: path and replay", PATH, REPLAY)
        if ECFLOW:
            if 0:
                replay(PATH)
            else:
                s = PATH.split('/')[1]
                if "mirror" not in PATH:
                    defs = Defs()
                    defs.add_suite(definitions[s])
                else:
                    defs = None
                replay(PATH, defs)
        else:
            sms_replay(PATH)

    elif MIRROR and PATH:
        mirror = Mirror(MIRROR, PATH, REPLAY)
        print("#MSG:step")
        mirror.process()
        print("#MSG:step")
        child.report("complete")
        print("#MSG:step")

    elif NUM and PATH:
        check3of5(NUM, PATH, WAIT, INTERVAL)

    else:
        usage()
    print("#MSG:step")

    child.report("complete")
"""
$nopp
$end
"""
