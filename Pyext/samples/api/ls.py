#!/usr/bin/env python
# Copyright 2009-2012 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
"""
dump server content, or a suite, line by line, fullname, for eas
"""
from __future__ import print_function
import sys
import os
try:
    import ecflow
except ImportError:
    sys.path.append(
        '/usr/local/apps/ecflow/current/lib/python2.7/site-packages')
    import ecflow


def printer(opt, iterate, full):
    if opt:
        for item in iterate:
            print(full + ":%s" % item)


def liner(full, name, fct):
    try:
        item = fct()
        if item:
            print(full + ":%s " % name + "%s" % item)
    except:
        pass


def def_port():
    return 1500 + int(pwd.getpwnam(get_username()).pw_uid)


def get_defs(host=None, port=None, path=None):
    if host is None:
        host = os.getenv("ECF_HOST", "localhost")
    if port is None:
        port = os.getenv("ECF_PORT", def_port())

    client = ecflow.Client(host, port)
    if path:
        if '/' in path and '/' != path:
            reg = str(path.split('/')[1])
            client.ch_register(False, [reg, ])
    client.sync_local()
    return client.get_defs()


class Ls(object):

    def process(self, node, path):
        full = node.get_abs_node_path()
        if ":" in path:
            top, nam = path.split(":")
        else:
            nam = None
            top = path
        if top in full:
            pass
        elif full in top:
            rc = None
            for curr in node.nodes:
                res = self.process(curr, path)
                if res is not None:
                    rc = res
            return rc
        else:
            return None

        if nam is None or self.opt.display:
            if type(node) == ecflow.Task:
                num = 10
                kind = "task"
            elif type(node) == ecflow.Family:
                num = 11
                kind = "family"
            elif type(node) == ecflow.Suite:
                num = 12
                kind = "suite"
            elif type(node) == ecflow.Alias:
                return
            else:
                print(type(node))
                raise BaseException("node kind?", type(node))
            print(node.get_abs_node_path(), num, kind, end="")

        if self.opt.status:
            status = "%s" % node.get_state()
            print(status)

        elif self.opt.display:
            status = "%s" % node.get_state()
            nums = {"unknown": 0,
                    "suspended": 1,
                    "complete": 2,
                    "queued": 3,
                    "submitted": 4,
                    "active": 5,
                    "aborted": 6,
                    "shutdown": 7,
                    "halted": 8}
            print(nums[status], status)

        else:
            print()

        printer(self.opt.events, node.events, full)
        printer(self.opt.meters, node.meters, full)
        printer(self.opt.labels, node.labels, full)

        printer(self.opt.limits, node.limits, full)
        printer(self.opt.limits, node.limits, full)

        printer(self.opt.dates, node.dates, full)
        printer(self.opt.dates, node.days, full)

        printer(self.opt.times, node.times, full)
        printer(self.opt.times, node.crons, full)
        printer(self.opt.times, node.todays, full)
        if self.opt.triggers:
            trig = node.get_trigger()
            if trig:
                print(full, ":trigger", trig)
            trig = node.get_complete()
            if trig:
                print(full, ":complete", trig)
        # for item in node.zombies: print(full + ":%s" % item, end="")
        if self.opt.variables or ":" in path:
            item = node.get_repeat()
            if not item.empty():
                if nam == item.name() or not nam:
                    print(full + ":repeat ", item.name(), end="")

            for item in sorted(node.variables):
                if 1 and nam == item.name() or not nam:
                    print(full + ":edit", item.name(), end="")
                    if self.opt.verbose:
                        print(" " + item.value())
                    else:
                        print()
                    if nam == item.name():
                        sys.exit(0)

            gvar = ecflow.VariableList()
            if self.opt.gvar:
                node.get_generated_variables(gvar)
            for item in gvar:
                if 1 and nam == item.name() or not nam:
                    try:
                        print(full + ":gedit", item.name(), )
                        if self.opt.verbose:
                            print(" %s" % item.value())
                        else:
                            print()
                    except:
                        pass
                    if nam == item.name():
                        sys.exit(0)

        item = node.get_late()
        if item:
            print(full, ":late", item)
        #item = node.get_defstatus();
        # if item:
        #    item = "%s" % item
        #    if "queued" not in item: print(full, ":defstatus", item)
        item = node.get_autocancel()
        if item:
            print(full, ":autocancel", item)

        # #liner(full, "late", node.get_late)
        # liner(full, "clock", node.get_clock)
        liner(full, "defstatus", node.get_defstatus)
        # liner(full, "autocancel", node.get_autocancel)
        # liner(full, "complete", node.get_complete)
        # liner(full, "trigger", node.get_trigger)

        if self.opt.recursive:
            for curr in node.nodes:
                self.process(curr, path)
        return node

    def __init__(self, parsed=None):
        if parsed.path is None:
            parsed.path = '/'
        if parsed.all:
            parsed.recursive = True
            parsed.verbose = True
            # parsed.display = True
            parsed.dates = True
            parsed.times = True
            parsed.meters = True
            parsed.events = True
            parsed.labels = True
            parsed.limits = True
            parsed.triggers = True
            # parsed.gvar = True
            # parsed.status = True
            # parsed.sms = True
            parsed.variables = True

        self.opt = parsed
        # print(parsed); raise BaseException
        path = parsed.path
        filename = parsed.path.replace("/", "_") + ".tmp"
        found = False
        if 0:  # not parsed.force:
            try:
                fid = open(filename, "r")
                found = True
            except:
                pass
        if found:
            print("#MSG: %s found" % filename)
        elif parsed.sms:
            user = os.getlogin()
            log = "set SMS_PROG %s; login %s %s 1;" % (
                parsed.port, parsed.host, user)
            grep = "|grep -v 'trigger ( an eq complete or an eq unknown)'"
            grep = "|grep -v 'fsobs eq complete or fsobs eq unknown'"
            get = "get %s; show %s %s > %s; " % (path, path, grep, filename)
            cdp = "/usr/local/apps/sms/bin/cdp"
            cmd = cdp + "<<EOF\n" + log + get + "exit 0\nEOF"
            print("#cmd:", cmd)
            os.system(cmd)
        elif parsed.file:
            # filename = parsed.file
            # print(parsed.file)
            defs = ecflow.Defs(parsed.file)
        else:
            defs = get_defs(parsed.host, parsed.port, path)
            defs.auto_add_externs(True)
            defs.save_as_defs(filename)

        # defs = ecflow.Defs(filename)
        # defs.auto_add_externs(True)
        for s in defs.suites:
            if s.get_abs_node_path() in path or path == "/":
                self.process(s, path)


def pars():
    import argparse
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-?", default=0, help="help")
    parser.add_argument("-h", "--host", default="localhost", help="host")
    parser.add_argument("-p", "--port", default="%d" % def_port(), help="port")
    parser.add_argument("-a", "--all", action="store_true", help="all")
    # parser.add_argument("-A", "--All", action="store_true", help="all")
    parser.add_argument("-f", "--file", default=None, help="file")
    parser.add_argument("-F", "--force", action="store_true", help="force")
    # parser.add_argument("-n", "--node", default="/", help="node")
    parser.add_argument("-N", "--node", default=None, help="node")
    parser.add_argument("-P", "--path", default="/", help="path")
    parser.add_argument("-R", "--recursive",
                        action="store_true", help="recursive")
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose")
    parser.add_argument("-d", "--display", action="store_true", help="display")
    parser.add_argument("-D", "--dates", action="store_true", help="Dates")
    parser.add_argument("-t", "--times", action="store_true", help="times")
    parser.add_argument("-m", "--meters", action="store_true", help="meters")
    parser.add_argument("-e", "--events", action="store_true", help="events")
    parser.add_argument("-l", "--labels", action="store_true", help="labels")
    parser.add_argument("-L", "--limits", action="store_true", help="limits")
    parser.add_argument("-T", "--triggers",
                        action="store_true", help="triggers")
    parser.add_argument("-G", "--gvar", action="store_true", help="gvars")
    parser.add_argument("-S", "--status", action="store_true", help="Status")
    parser.add_argument("-s", "--sms", action="store_true", help="sms")
    parser.add_argument("-V", "--variables",
                        action="store_true", help="variables")
    parser.add_argument('node', nargs="?", type=str)
    # print("#DBG:", parser)
    return parser


def get_username():
    return pwd.getpwuid(os.getuid())[0]


if __name__ == "__main__":
    import pwd
    node = os.getenv("ECF_HOST", "localhost")
    port = os.getenv("ECF_PORT", def_port())
    parsed = pars().parse_args()
    Ls(parsed)

"""
aaa
ECF_HOST=vsms1 ECF_PORT=31415 ./ls.py -NRv /eda
ECF_HOST=vsms1 ECF_PORT=31415 ./ls.py -p 31415 -n vsms1 -NRv /eda

ECF_HOST=vsms1 ECF_PORT=32222 python ls.py /seas_mm/main
ECF_HOST=vsms1 ECF_PORT=43333 python t3.py e_sapp main
ecflow_client --port 32222 --host vsms1 --get_state /seas_mm
ecflow_client --port 32222 --host vsms1 --stats | grep Stat
cdp -c "od; sms -s " | grep Status
rcp ls.py emos@ibis:/home/ma/emos/bin/ls.py

ls.py -h vsmsrdna -p 314122 -s -n /map
python ~/git/suites/o/def/tkinter/ls.py -h vsmsrdna -p 314122 -s -n /map

python ./cli_proc.py -c -r vsms1:32112 o /o eurus:1630
python ./cli_proc.py -c -r vecfrdda1:11001 dag /dag/gsup eurus:1630

./ls.py -V -t -R -P /USER
./ls.py -T -R -V -v -t -d -D -e -m -l -P /USER
/ls.py -T -p $ECF_PORT -h $ECF_HOST -NRVvtdDemlT /

"""
