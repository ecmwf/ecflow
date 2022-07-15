#!/usr/bin/env python3
# Copyright 2009- ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
"""
dump server content, or a suite, line by line, fullname, for eas
"""
from __future__ import print_function
import pwd
import sys
import os
assert sys.version_info >= (3, )
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
    port = os.getenv("ECF_PORT", None)
    if port is None: return 1500 + int(pwd.getpwnam(get_username()).pw_uid)
    else: return int(port)


def get_defs(host=None, port=None, path=None):
    if host is None:
        host = os.getenv("ECF_HOST", "localhost")
    if port is None:
        port = os.getenv("ECF_PORT", def_port())

    client = ecflow.Client(host, port)
    reg = False
    if path:
        if '/' in path and '/' != path:
            reg = str(path.split('/')[1])
            client.ch_register(False, [reg, ])
            print(reg)
            reg = True
        elif path == '/': pass
        else:
            raise Exception(path)
    else:
        raise Exception(path)
    client.sync_local()
    if reg:
        client.ch_drop()
    return client.get_defs()


def print_trigger(full, kind, expr):
    if not expr:
        return
    expr = "'%s'" % expr
    print(full, kind, expr.replace(
        " and ", " AND ").replace(
            " or ", " OR ").replace(
                " eq ", " == "))


class Ls(object):

    def __init__(self, parsed=None):
        # print(parsed)
        if parsed is None:
            parsed.path = '/'
            parsed.recursive = True
            parsed.triggers = True
            parsed.variables = True
            parsed.status = True
            parsed.display = True

        if parsed.path is None:
            parsed.path = '/'
            if parsed.node != '/':
                parsed.path = parsed.node

        if parsed.all:
            parsed.recursive = True
            parsed.verbose = True
            parsed.display = True
            parsed.dates = True
            parsed.times = True
            parsed.meters = True
            parsed.events = True
            parsed.labels = True
            parsed.limits = True
            parsed.inlimits = True
            parsed.triggers = True
            # parsed.gvar = True
            parsed.status = True
            # parsed.sms = True
            parsed.variables = True

        self.parsed = parsed

        # path = parsed.path
        if parsed.path is not None:
            filename = parsed.path.replace("/", "_") + ".tmp"

        if parsed.sms:
            user = os.getlogin()
            log = "set SMS_PROG %s; login %s %s 1;" % (
                parsed.port, parsed.host, user)
            grep = "|grep -v 'trigger ( an eq complete or an eq unknown)'"
            grep = "|grep -v 'fsobs eq complete or fsobs eq unknown'"
            get = "get %s; show %s %s > %s; " % (
                parsed.path, parsed.path, grep, filename)
            cdp = "/usr/local/apps/sms/bin/cdp"
            cmd = cdp + "<<EOF\n" + log + get + "exit 0\nEOF"
            print("#cmd:", cmd)
            os.system(cmd)
        elif parsed.file:
            defs = ecflow.Defs(parsed.file)
            # defs.add(ecflow.Suite("o").add(ecflow.Family("lag")))
            num = 0
            for s in defs.suites:
                name = s.name()
                num += 1
            if parsed.path is None and num == 1:
                parsed.path = "/" + name
        else:
            defs = get_defs(parsed.host, parsed.port, parsed.path)
            defs.auto_add_externs(True)
            try: defs.save_as_defs(filename)
            except: print("cannot save", filename)

        # defs = ecflow.Defs(filename)
        # defs.auto_add_externs(True)
        for s in defs.suites:
            # print(s)
            if s.get_abs_node_path() is None:
                continue
            if parsed.path is None:
                continue
            if s.get_abs_node_path() in parsed.path or parsed.path == "/":
                self.process(s, parsed.path)

    def process(self, node, path):
        full = node.get_abs_node_path()
        # print(node.name(), path)
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

        status = None        
        if nam is None or self.parsed.display:
            if status is None: status = "%s" % node.get_state()
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
                # return
                num = 13
                kind = "alias"
            else:
                print(type(node))
                raise BaseException("node kind?", type(node))
            print(node.get_abs_node_path(), num, kind, status, end="\n")

        elif self.parsed.display:
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

        elif self.parsed.status:
            status = "%s" % node.get_state()
            print(status)

        else:
            print()
        # print(self.parsed)
        printer(self.parsed.events, node.events, full)
        printer(self.parsed.meters, node.meters, full)
        printer(self.parsed.labels, node.labels, full)

        printer(self.parsed.limits, node.limits, full)
        printer(self.parsed.inlimits, node.inlimits, full)

        printer(self.parsed.dates, node.dates, full)
        printer(self.parsed.dates, node.days, full)

        printer(self.parsed.times, node.times, full)
        printer(self.parsed.times, node.crons, full)
        printer(self.parsed.times, node.todays, full)
        if self.parsed.triggers:
            print_trigger(full, ":trigger", node.get_trigger())
            print_trigger(full, ":complete", node.get_complete())
        # for item in node.zombies: print(full + ":%s" % item, end="")
        if self.parsed.variables or ":" in path:
            item = node.get_repeat()
            if not item.empty():
                if nam == item.name() or not nam:
                    print(full + ":repeat", item.name(), item.value(), "'%s'" % item, end="\n")

            # for item in sorted(node.variables):
            for item in node.variables:
                if 1: #  and nam == item.name() or not nam:
                    if " " in item.value() and not "'" in item.value() and not "\"" in item.value():
                        print(full + ":%s" % item.name(), "'%s'" % item.value(), end="\n")
                    else: print(full + ":%s" % item.name(), item.value(), end="\n")
                    # if self.parsed.verbose: print(" " + item.value())
                    # else: print()
                    if nam == item.name():
                        sys.exit(0)

            gvar = ecflow.VariableList()
            if self.parsed.gvar:
                node.get_generated_variables(gvar)
            for item in gvar:
                if 1 and nam == item.name() or not nam:
                    try:
                        print(full + ":gedit", item.name(), )
                        if self.parsed.verbose:
                            print(" %s" % item.value())
                        else:
                            print()
                    except:
                        pass
                    if nam == item.name():
                        sys.exit(0)

        item = node.get_late()
        if item and self.parsed.verbose:
            print(full, ":late", item)
        #item = node.get_defstatus();
        # if item:
        #    item = "%s" % item
        #    if "queued" not in item: print(full, ":defstatus", item)
        item = node.get_autocancel()
        if item and self.parsed.verbose:
            print(full, ":autocancel", item)

        # #liner(full, "late", node.get_late)
        # liner(full, "clock", node.get_clock)
        if self.parsed.verbose:
            liner(full, "defstatus", node.get_defstatus)
        # liner(full, "autocancel", node.get_autocancel)
        # liner(full, "complete", node.get_complete)
        # liner(full, "trigger", node.get_trigger)

        if type(node) == ecflow.Alias:
            return node

        if self.parsed.recursive:
            for curr in node.nodes:
                self.process(curr, path)
        return node


def pars():
    import argparse
    parser = argparse.ArgumentParser(add_help=False)
    # parser.add_argument("-h", default=0, help="help")
    parser.add_argument("-H", "--host", default="localhost", help="host")
    parser.add_argument("-p", "--port", default="%d" % def_port(), help="port")
    parser.add_argument("-a", "--all", action="store_true", help="all")
    parser.add_argument("-A", "--Alias", action="store_true", help="aliases")
    parser.add_argument("-f", "--file", default=None, help="file")
    parser.add_argument("-F", "--force", action="store_true", help="force")
    # parser.add_argument("-n", "--node", default="/", help="node")
    parser.add_argument("-N", "--node", default=None, help="node")
    parser.add_argument("-P", "--path", default=None, help="path")
    parser.add_argument("-R", "--recursive", action="store_true", help="recursive")
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose")
    parser.add_argument("-d", "--display", action="store_true", help="display")
    parser.add_argument("-D", "--dates", action="store_true", help="Dates")
    parser.add_argument("-t", "--times", action="store_true", help="times")
    parser.add_argument("-m", "--meters", action="store_true", help="meters")
    parser.add_argument("-e", "--events", action="store_true", help="events")
    parser.add_argument("-l", "--labels", action="store_true", help="labels")
    parser.add_argument("-L", "--limits", action="store_true", help="limits")
    parser.add_argument("-i", "--inlimits",
                        action="store_true", help="inlimits")
    parser.add_argument("-T", "--triggers",
                        action="store_true", help="triggers")
    parser.add_argument("-G", "--gvar", action="store_true", help="gvars")
    parser.add_argument("-S", "--status", action="store_true", help="Status")
    parser.add_argument("-s", "--sms", action="store_true", help="sms")
    parser.add_argument("-V", "--variables", action="store_true", help="variables")
    parser.add_argument('node', nargs="?", type=str)
    return parser


def get_username():
    return pwd.getpwuid(os.getuid())[0]


if __name__ == "__main__":
    parsed = pars().parse_args()
    # print("#", parsed)
    Ls(parsed)


"""
aaa
ECF_HOST=localhost ECF_PORT=3141 ./ls.py -Rv --path /eda

./ls.py --host=localhost --port=3141 -n vsms1 -R -v -N /eda

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
