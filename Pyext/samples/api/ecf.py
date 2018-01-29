#!/usr/bin/env python
# coding=utf-8
# This software is provided under the ECMWF standard software license agreement.
""" a layer over raw ecflow api

use 'export ECF_DEBUG_LEVEL=10' to remove warning message related to
variables overwrite

specialisation module for ecFlow nodes/attributes:
initially it was simple derivation
then it is composition

"""
from __future__ import print_function    # (at top of module)
import sys
import pwd
import os
import unittest
# try:
#     from ecflow import TimeSlot
#     from ecflow import JobCreationCtrl as JobCreationCtrl
#     from ecflow import ChildCmdType
#     from ecflow import ZombieType, ZombieAttr, ZombieUserActionType
# except:
#     print "# ecf.py cannot import few types"
try:
    import ecflow
except:
    loc = "/usr/local/apps/ecflow/current/lib/python2.7/site-packages/ecflow"
    sys.path.append(loc)
    loc = "/usr/local/lib/python2.7/site-packages"
    sys.path.append(loc)
    import ecflow
# import ecflow; 
ecflow.Ecf.set_debug_level(3)

DEBUG = 0
DECORATE = "ALL"
# DECORATE = "ONLY_TRIGGER"
# DECORATE = "NO_TRIGGER"
# DECORATE = "ONLY_EVENT"
# DECORATE = "NO_ATTRIBUTE"

USE_TIME = True
USE_LATE = False
USE_TRIGGER = True
USE_LIMIT = True
CPUTIME = True

if DECORATE == "NO_TRIGGER":
    USE_TRIGGER = False
    USE_LIMIT = True
    USE_EVENT = True
elif DECORATE == "ALL":
    USE_TRIGGER = True
    USE_LIMIT = True
    USE_EVENT = True
else: raise Exception


def get_username():
    return pwd.getpwuid(os.getuid())[0]


def get_uid():
    return pwd.getpwnam(get_username()).pw_uid

def obsolete():
    if 1: return
    if 1: raise Exception

def raise_int(arg):
    if 0: return
    if "/none" in arg: raise Exception("None!!!", arg)
    if arg in ("1==0", "0==1", "1==1", "0==0"): return
    if arg[0] in ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9"):
        raise Exception("please add ./", arg)

def set_raw_mode(kind=True):
    import sms2ecf
    sms2ecf.RAW_MODE = kind


LINKTASK = False  # FD
# LINKTASK = True  # RD mode v we turn task into families,
                   # with a leaf a task matching script name
def linktask(name, real="get", add=None, fam=None, task=True):
    """ give a change to avoid UNIX links time consuming for maintenance, update
    perforce git
    name: original name
    real: script/task wrapper name
    add: attributes that shall be added to the leaf task, eg Label, Event, Meter
    """
    if fam is None: fam = name
    try:
        import parameters as ip
        # if ip.USER == "rdx": global LINKTASK; LINKTASK = True  # acceptance
        # if ip.USER == "emos": global LINKTASK; LINKTASK = False  # acceptance
    except: pass
    if LINKTASK: return Task(name)  # RD???
    return Family(fam).add(
        Task(real).add(add, If(task, Edit("TASK", name))))


def translate(name, value=None):
    """ Translate from sms to ecflow """
    try: import sms2ecf
    except: return name, value
    def is_sms():
        if sys.argv[-1] in ("sms", "od3", "ode", "map", "od", "od2", "ligarius"):
            sms2ecf.ECF_MODE = "sms"
            return True
        return sms2ecf.ECF_MODE == "sms"
    if is_sms():
        sms2ecf.ECF_MODE = "sms"
        return sms2ecf.translate(name, value)
    return name, value


DEFS = None  # general case when not in parameters.py
# USE_EXTERN = 1
# NO_EXTERN_ALONE = 0 # set to 1 for test/activate
class Extern(object):
    """ extern may be collected and added as soon as Defs is created """

    def __init__(self, path):
        if type(path) in (tuple, list, str): pass
        else: raise Exception

        try:
            import parameters as ip
            if type(path) in (tuple, list):
                for item in path: Extern(item)
            elif type(path) in (str, ):
                ip.DEFS.load.add_extern(path)
            return
        except:
            global DEFS
            if DEFS is None: DEFS = Defs()

        if type(path) == tuple or type(path) == list:
            for ppp in path: Extern(ppp)

        elif type(path) == str:
            if DEBUG: print("#MSG: extern", path)
            if ":" in path: Extern(path.split(":")[0])
            elif DEFS: DEFS.load.add_extern(path)

        elif path is None: pass

        else:
            raise Exception(type(path), path)

    def add_to(self, node): return None


class State(object):
    """ this class aims at affording a user the possibility to add Triggers as
    t1 = Task("t1")
    Task("ts").add(Trigger(t1 == COMPLETE))

    SUBMITTED, ACTIVE, SUSPENDED, ABORTED, QUEUED, COMPLETE, UNKNOWN
    are instance of this class.
    """

    def __init__(self, state):
        """ store the status """
        self.state = str(state)

    def __str__(self):
        """ translate into string """
        return "%s" % self.state

    def __eq__(self, arg):
        """ when == is used,
        we should care about task name starting with 0-9"""
        if type(arg) == str:
            if type(arg[0]) == int:
                add = "./"
            else: add = ""
            return add + arg + " == " + self.state
        elif isinstance(arg, Node):
            return arg.fullname() + " == " + self.state
        elif isinstance(arg, ecflow.Node):
            return arg.get_abs_node_path() + " == " + self.state
        return False

    def __ne__(self, arg):
        """ aka != """
        if type(arg) == str:
            if type(arg[0]) == int:
                add = "./"
            else: add = ""
            return add + arg + " != " + self.state
        elif isinstance(arg.load, ecflow.Node):
            return arg.load.get_abs_node_path() + " != " + self.state
        return False

    def value(self):
        """ return state """
        return self.state

    def eval(self, node):
        """ return state """
        return self.state == node.get_state()

SUBMITTED = State("submitted")
ACTIVE = State("active")
SUSPENDED = State("suspended")
ABORTED = State("aborted")
QUEUED = State("queued")
COMPLETE = State("complete")
UNKNOWN = State("unknown")


class Attribute(object):
    """ generic attribute to be attached to a node"""

    def __init__(self): 
        self.load = None

    def add_to(self, node):
        """ use polymorphism to attach attribute to a node"""
        raise Exception("ERR: virtual class")

    def __get_attr__(self, attr):
        return getattr(self.load, attr)

    def real(self): 
        return self.load


class Label(Attribute):
    """wrap around label"""

    def __init__(self, name, msg=""):
        self.load = ecflow.Label(name, msg)

    def add_to(self, node):
        """add_label"""
        node.load.add_label(self.load)
        return node

class Meter(Attribute):
    """wrap around meter"""

    def __init__(self, name, beg, end, thr=None):
        if thr is None: thr = end
        self.load = ecflow.Meter(name, beg, end, thr)

    def add_to(self, node):
        """ add_meter"""
        node.load.add_meter(self.load)
        return node


class Event(Attribute):
    """ wrap around event"""

    def __init__(self, name):
        self.load = ecflow.Event(name)

    def add_to(self, node):
        """ add_event"""
        node.load.add_event(self.load)
        return node


class Inlimit(Attribute):
    """ a class to host a path for a limit
        silently ignore if USE_LIMIT is False,
        (in debug mode) """

    def __init__(self, fullpath):
        self.load = None
        if USE_LIMIT:
            try:
                path, name = fullpath.split(":")
            except:
                name = fullpath
                path = ""
            if name is None:
                raise Exception
            if '-prod' in name: name = name.split('-')[0]  # vdiss-prod
            if '-' in name: name = name.replace('-', '_')
            if "None" in path or "None" in name:
                raise Exception(path, name, fullpath)
            self.load = ecflow.InLimit(name, path)
            self.path_ = path
            self.name_ = name

    def add_to(self, node):
        """ add_inlimit"""
        if not USE_LIMIT:
            return
        if self.load is None:
            raise Exception
        node.load.add_inlimit(self.load)
        return node

    def value(self):
        """ get limit fullpath-name """
        return self.path_ + ":" + self.name_

    def name(self):
        """ get limit name """
        return self.name_


class InLimit(Inlimit): pass


def item_to_string(prev, index, name):
    """ index not used... """
    if name is None: return ""
    if name is "": return prev
    if "<main>" in name: raise Exception
    if "==" in name: return name
    if len(name) > 0:
        if name[0].isdigit():
            pre = "./"  # "0123456789":
            name = pre + name
    if prev is None or prev == "":
        if ':' in name or name == "":
            return name
        else: return "%s == complete" % name
    if ' or ' in name:
        if "complete" in name:
            return prev + "%s" % name
        elif ":" in name:
            return prev + "%s" % name
        else:
            return prev + "%s == complete" % name
    elif ':' in name:  # event: and x:1
        return prev + " and %s" % name
    elif 'complete' in name:  # complete: and x == complete
        return prev + " and %s" % name
    else:
        return prev + " and %s == complete" % name


class Trigger(Attribute):
    """ add trigger (string, list of task names, or directly
           expression and: and'ed (True) or or'ed (False) unk: add or
           [name]==unknown for RD """

    def __init__(self, expr, unk=False, anded=True):
        self.expr = ""
        self.load = None
        if expr is None or expr == "":
            return
        if type(expr) == str:
            if "%s" in expr: raise Exception("ERR:", expr)
            if "%d" in expr: raise Exception("ERR:", expr)
            self.expr = expr
            try:
              import parameters as ip
              dim = len(ip.SELECTION)
              for ploc in expr.split():
                if ploc[0] == '/' and ip.SELECTION != ploc[1:dim + 1]:
                    if ploc in ("eq", "ne", "==", "!=", "and", "or",
                                "active", "complete", "queued", ):
                        continue
                    if ploc.isdigit():
                        continue
                    Extern(ploc.replace("==complete", "").replace(")", ""))
            except: pass
            # NO_EXTERN_ALONE = 0 # set to 1 for test/activate
            return
        if type(expr) == tuple:
            prep = list(expr)
            expr = prep

        if type(expr) == Task:
            pnode = expr.fullname()
            if pnode[0] != '/': pnode = expr.name()
            try:
                if ic.psel() not in pnode: pnode = expr.name()
            except: pass
            # if 1: raise Exception("really?", pnode)
            self.expr += item_to_string(None, 0, pnode)

        elif type(expr) in (list, set, tuple):
            for index, name in enumerate(expr):
                if name is None:
                    continue
                pre = ""

                if name is None: continue
                if type(name) in (set, list, tuple):
                    print("#WAR: please avoid list in list", expr, name)
                    if 1: print("#WAR: list in list")
                    elif 1: raise Exception("please avoid list in list", expr, name)
                    name = ""
                    for index, another in enumerate(name):
                        name += item_to_string(name, index, another)
                elif type(name) in (Node, Task, Family, Suite):
                    if DEBUG: print(name.name())
                    fullname = name.fullname()
                    if fullname[0] == '/':
                        try:
                            import inc_common as ic
                            if ic.psel() not in fullname:
                                fullname = name.name()
                        except: print("#WAR: trigger expcept")
                    name = fullname
                elif type(name) in (ecflow.Task, ecflow.Family):
                    name = name.name()
                elif type(name) == str:
                    pass
                else:
                    raise Exception(type(name), name)

                if "==" in name: pass
                elif len(name) > 0:
                    # if "==" in name: pass
                    # if "1==0" in name or "0==1" in name or "1==1" in name: pass
                    if name[0].isdigit(): pre = "./"  # "0123456789":

                self.expr = item_to_string(self.expr, index, name); continue ##

        elif type(expr) in (ecflow.Expression,
                            ecflow.PartExpression):
            self.expr = ecflow.Expression(str(item))

        elif type(expr) in (Family, Task):
            fullname = expr.fullname()
            if fullname[0] == '/':
                try:
                    import inc_common as ic
                    if ic.psel() not in fullname:
                        fullname = name.name()
                except: print("#WAR: trigger expcept")
            self.expr = fullname
            # check full name is really full name, not /name
            # print("#REM:", self.expr)
            if self.expr == "/%s" % expr.name(): self.expr = expr.name()
        else:
            raise Exception("what? trigger?", type(expr))

        if self.expr: self.load = ecflow.Expression(self.expr)
        if "YMD+1" in self.expr: raise Exception(self.expr)
        # NO_EXTERN_ALONE = 0 # set to 1 for test/activate

    def expression(self):
        return self.expr

    def add_to(self, node):
        if self.expr is None or self.expr == "" or not USE_TRIGGER:
            return None
        if 0 and "/main/00/prod/fc/240/prodgen" in "%s" % self.expr:  # help DEBUG
            raise Exception(self.expr, node.fullname())
        if node.load.get_trigger() is None:
            node.load.add_trigger(self.expr)
        else:
            node.load.add_part_trigger(ecflow.PartExpression(self.expr, True))
        return node

class TriggerImpossible(Trigger):
    """attribute to be added to node when it is not expected to run any task"""

    def __init__(self):
        """ add an 'impossible trigger', for a task not to run """
        super(TriggerImpossible, self).__init__("1==0")


class TriggerAlways(Trigger):
    """attribute to be added to node when it is not expected to run any task"""

    def __init__(self):
        """ add an 'impossible trigger', for a task not to run """
        super(TriggerAlways, self).__init__("1==1")


class Complete(Trigger):
    """ class to host complete expression, added later to a node"""

    def __init__(self, expression, unk=False, anded=False):
        super(Complete, self).__init__(expression, unk, anded)

    def add_to(self, node):
        if USE_TRIGGER and self.expr is not None:
            if "%s" in self.expr: raise Exception
            node.load.add_complete(self.expr)
        return node


class CompleteAlways(Complete):
    """ attribute """

    def __init__(self):
        """ always True as soon as evaluated """
        super(CompleteAlways, self).__init__("1==1")


class Clock(Attribute):
    """ wrapper to add clock """

    def __init__(self, arg="24:00", hybrid=0):
        if type(arg) == str:
            hybrid = "hybrid" in arg
            hhh, mmm, sss = [0, 0, 0]
            try:
                hhh, mmm, sss = arg.split(':')
                self.load = ecflow.Clock(hhh, mmm, sss, hybrid)
            except:
                self.load = ecflow.Clock(hybrid)
        else:
            self.load = ecflow.Clock(arg)

    def add_to(self, node):
        if type(node) != Suite:
            print("#WAR: clock can only be attached to suite node,\n",
                  "#WAR: clock is ignored")
            return
        node.load.add_clock(self.load)
        return node


class Autocancel(Attribute):
    """ wrapper to add time """

    def __init__(self, arg):
        if type(arg) == str:
            if ':' in arg:  # hh:mm +hh:mm
                hhh, mmm = arg.split(':')
                rel = '+' in arg
                self.load = ecflow.Autocancel(int(hhh), int(mmm), rel)
            else:
                self.load = ecflow.Autocancel(int(arg))
        else:
            self.load = ecflow.Autocancel(arg)  # days

    def add_to(self, node):
        node.load.add_autocancel(self.load)
        return node


class Autocancel(Autocancel): pass


class Script(Attribute):
    def __init__(self, script):
      self._script = script

    def add_to(self, node):
        print("#WAR: Script attribute, not yet ... ignored")
        return node


class Time(Attribute):
    """ wrapper to add time """

    def __init__(self, arg):
        self.load = arg

    def add_to(self, node):
        if USE_TIME and self.load is not None:
            node.load.add_time(self.load)
        return node

class Today(Time):
    """ wrapper to add time """

    def add_to(self, node):
        if USE_TIME and self.load is not None:
            node.load.add_today(self.load)
        return node


class Cron(Time):
    """ wrapper to add time """

    def __init__(self, bes, wdays=None, days=None, months=None):
        import argparse
        self.load = ecflow.Cron()

        if not ("-w" in bes or "-m" in bes or "-d" in bes):
            self.load.set_time_series(bes)
            return

        parser = argparse.ArgumentParser()
        parser.add_argument("-w", nargs='?', default=0, help="weekdays")
        parser.add_argument("-d", nargs='?', default=0, help="days")
        parser.add_argument("-m", nargs='?', default=0, help="months")
        parser.add_argument("arg", type=str, help="begin end step")
        parsed = parser.parse_args(bes.split())

        if parsed.w:
            self.load.set_week_days([int(x) for x in parsed.w.split(',')])
        if parsed.d:
            self.load.set_week_days([int(x) for x in parsed.d.split(',')])
        if parsed.m:
            self.load.set_months([int(x) for x in parsed.m.split(',')])
        self.load.set_time_series(parsed.arg)

    def add_to(self, node):
        if USE_TIME and self.load is not None:
            node.load.add_cron(self.load)
        else: print("#WAR: ignoring: %s" % self.load)
        return node


class Date(Time):
    """ wrapper to add date """

    def __init__(self, arg, mask=False):
        super(Date, self).__init__(arg)
        self.mask = mask

    def add_to(self, node):
        if USE_TIME and self.load is not None:
            # ??? emos avoids dates, datasvc would not
            if self.mask:
                node.load.add_variable("DATEMASK", self.load)
            else:
                ddd, mmm, yyy = self.load.split('.')
                if ddd == '*':
                    ddd = 0
                if mmm == '*':
                    mmm = 0
                if yyy == '*':
                    yyy = 0

                node.load.add_date(int(ddd), int(mmm), int(yyy))
                # node.add_date(self.load)
        return node


class Day(Date):
    """ wrapper to add day """

    def add_to(self, node):
        if not USE_TIME or self.load is None:
            return
        if isinstance(self.load, str):
            days = {"monday": ecflow.Days.monday,
                    "sunday": ecflow.Days.sunday,
                    "tuesday": ecflow.Days.tuesday,
                    "wednesday": ecflow.Days.wednesday,
                    "thursday": ecflow.Days.thursday,
                    "saturday": ecflow.Days.saturday,
                    "friday": ecflow.Days.friday,

                    "mon": ecflow.Days.monday,
                    "sun": ecflow.Days.sunday,
                    "tue": ecflow.Days.tuesday,
                    "wed": ecflow.Days.wednesday,
                    "thu": ecflow.Days.thursday,
                    "sat": ecflow.Days.saturday,
                    "fri": ecflow.Days.friday, }
            node.load.add_day(ecflow.Days(days[self.load]))
        else:
            node.load.add_day(ecflow.Days(self.load))
        return node


class Defstatus(Attribute):
    """ add defstatus attribute"""

    def __init__(self, kind):
        if type(kind) == str:
            kinds = {"suspended": ecflow.DState.suspended,
                     "aborted": ecflow.DState.aborted,
                     "complete": ecflow.DState.complete,
                     "active": ecflow.DState.active,
                     "submitted": ecflow.DState.submitted,
                     "unknown": ecflow.DState.unknown,
                     "queued": ecflow.DState.queued, }
            self.load = ecflow.Defstatus(kinds[kind])
        else: self.load = kind

    def add_to(self, node):
        if type(node) in (Suite, Family, Task):
            node.load.add_defstatus(self.load)
        else: node.add_defstatus(self.load)
        return node


# class Defcomplete(Defstatus):
#     """ wrapper to add defstatus complete """

#     def __init__(self):
#         pass

#     def add_to(self, node):
#         node.add(Defstatus("complete"))
#         return node


# class DefcompleteIf(Defcomplete):
#     """ wrapper to add conditional defstatus complete
#     just change name to make it explicit """

#     def __init__(self, arg=True):
#         self.load = arg

#     def add_to(self, node):
#         if self.load:
#             node.add(Defstatus("complete"))
#         # else: node.defstatus("queued") # in comment to prevent
#         # overwrite when using multiple defcomplete
#         return node


class Limit(Attribute):
    """ wrapper to add limit """

    def __init__(self, name=None, size=1, inlimit=0):
        self.name = name
        self.size = size
        self.addi = inlimit

    def add_to(self, node):
        if USE_LIMIT and self.name is not None:
            if type(self.name) is dict:
                if LINKTASK: items = self.name.items()   # 45r1 COMPARE
                else: items = sorted(self.name.items())  # EMOS prefers
                for name, size in items:
                # for name in sorted(self.name.keys()):
                    size = self.name[name]
                    node.load.add_limit(name, size)
                    if self.addi:
                        node.load.add_inlimit(name)
                    else:
                        pass
            else:
                node.load.add_limit(self.name, self.size)
                if self.addi: node.load.add_inlimit(name)
        return node

# class Limits(Limit): pass

class Late(Attribute):
    """ wrapper around late, to be add'ed to families and tasks """

    def __init__(self, arg):
        self.load = None
        if not USE_LATE:
            # print("#MSG: late is disabled")
            return
        sub = False
        act = False
        com = False
        rel = False
        self.load = ecflow.Late()
        for item in arg.split(" "):
            if item == "-s":
                sub = True
            elif item == "-c":
                com = True
            elif item == "-a":
                act = True
            else:
                hour, mins = item.split(":")
                rel = "+" in hour
                if "+" in hour:
                    hour = hour[1:]
                if sub:
                    self._add_sub(hour, mins)
                elif com:
                    self._add_com(hour, mins, rel)
                elif act:
                    self._add_act(hour, mins)
                sub = False
                act = False
                com = False

    def _add_sub(self, hour, mins):
        """ submitted"""
        self.load.submitted(ecflow.TimeSlot(int(hour), int(mins)))

    def _add_com(self, hour, mins, rel):
        """ complete"""
        self.load.complete(ecflow.TimeSlot(int(hour), int(mins)), rel)

    def _add_act(self, hour, mins):
        """ active"""
        self.load.active(ecflow.TimeSlot(int(hour), int(mins)))

    def add_to(self, node):
        if USE_LATE and self.load is not None:
            try: node.load.add_late(self.load)
            except:
                if DEBUG: print("#WAR: late is already attached")
        return node


def python_true(key, val):
    if "%s" % val == "True":
        if key in("HYPERTHREADING"
                  "USE_HUGEPAGE"): return  # OK
        print("#WAR: really???", key, val)


class Edit(Attribute):
    """ dedicated class to enable variable addition with different
    syntax """

    def __init__(self, name, value=""):
        self.load = ecflow.Edit(name, value)

    def _set_tvar(self, key, val):
        """ facilitate to load a ecflow suite to SMS, translating
        variable names"""
        keyt, edit = translate(str(key), str(val))
        # if keyt == "SCHOST" and val == "ccb": raise Exception
        if self.load is None:
            self.load = ecflow.Variable(keyt, edit)
        else:
            next = self.next
            self.next = Edit(keyt, edit, next)

    def __init__(self, __a=None, __b=None, __next=None, *args, **kwargs):
        self.load = None
        self.next = __next

        if len(args) > 0:
            if type(args) == list:
                for item in args.iteritems():
                    key = item.name()
                    val = item.value()
                    python_true(key, val)
                    self._set_tvar(item.name(), item.value())
            elif type(args) == tuple:
                for key, val in args.items():
                    python_true(key, val)
                    self._set_tvar(key, val)
            else:
                raise Exception
        if len(kwargs) > 0:
            # for key, val in kwargs.items():
            # for key in sorted(kwargs.keys()):  # FIXME
            for key in kwargs.keys():
                val = kwargs[key]
                python_true(key, val)
                self._set_tvar(key, val)
        if type(__a) == dict:
            # for key, val in __a.items():
            # for key in sorted(__a.keys()):
            for key in __a.keys():  # FIXME
                val = __a[key]
                python_true(key, val)
                self._set_tvar(key, val)
        elif type(__a) == tuple:
            raise Exception
        elif type(__a) == list:
            raise Exception
        elif type(__a) == Variable:
            self.load = __a
        elif __a is not None and __b is not None:
            python_true(__a, __b)
            self._set_tvar(__a, __b)
        elif __a is None and __b is None:
            pass
        else:
            raise Exception(__a, __b, __next, args, kwargs)

    def add_to(self, node):
        if self.load is not None:
            edit = "%s" % self.load
            if "CPUTIME" in edit and not CPUTIME: pass
            elif node in (Suite, Family, Task):
                node.load.add_variable(self.load)
            else: 
                node.add_variable(self.load)
            if node.name() == "o" and "QUEUE 'emos" in edit:
                raise Exception(node.name(), edit)

            if 1:  # try:  # Operators' request
                labels = {"WSHOST": "infopws",
                          "SCHOST":"infopsc",
                          "HOST": "infophs", }
                # if "seas" in ip.SELECTION: pass
                for key in labels.keys():
                  try:
                    if "ECF_JOB_CMD" in edit:
                        find = "%" + key + "%"
                        if find in edit: node.add_label("infopcmd", key)
                    else:
                        find = "edit %s " % key
                        if find in edit: node.add_label(
                                labels[key], edit.replace(find, ""))
                  except: pass
        if self.next is not None: self.next.add_to(node)
        return node

    def add(self, what):
        print("add???", "%s" % self.load)
        if type(what) in (Suite, Family, Task):
            raise Exception(what.fullname())
        else:
            raise Exception(type(what))


class Variable(Edit): pass


class Variables(Edit): pass


class Repeat(Attribute):
    """ repeat date/int/string/enum """

    def __init__(self, name="YMD", start=20120101, end=21010101,
                 step=1, kind="date"):
        if kind in "date":
            self.load = ecflow.RepeatDate(name, int(start), int(end), int(step))
        elif "int" in kind:
            self.load = ecflow.RepeatInteger(
                name, int(start), int(end), int(step))
        elif kind == "string":
            self.load = ecflow.RepeatString(name, start)
        elif "enum" in kind:
            self.load = ecflow.RepeatEnumerated(name, start)
        elif kind == "day":
            self.load = ecflow.RepeatDay(step)
        else:
            self.load = None

    def add_to(self, node):
        if self.load is not None:
            node.load.add_repeat(self.load)
        return node


def If(test=True, then=None, otow=None, *args):
    """ enable Task("t1").add(If(test=(1==1),
                                 then=Edit(ONE=1),
                                 otow=Edit(TWO=2)))
        appreciate that both branches are evaluated, using this If class
        ie there is no 'dead code' as it is with python language 'if' structure

        using If to distinguish od/rd mode request that both users share
        the variables (parameter.py) and ecf.py

        otow: on the other way?
        """
    if len(args) > 1:
        try:
            print(then.name())
            print(otow.name())
        except: pass
        # print(args[0].name())
        raise Exception("test",
                        test, "\nthen\n",
                        then, "\nelse",
                        otow, "arg", args)
    if test:
        return then
    return otow


class Root(object):  # from where Suite and Node derive
    """ generic tree node """

    def __init__(self): self.load = None  # to be filled with ecFlow item

    def real(self): 
        return self.load

    def __get_attr__(self, attr): return getattr(self.load, attr)

    def get_parent(self): return self.load.get_parent()

    def __str__(self):
        return self.fullname()

    def __repr__(self):
        return "%s" % self.load

    def __eq__(self, node):
        if isinstance(self.load, ecflow.Node):
            return "%s == " % self + str(node)
        return False

    def __ne__(self, node):
        if isinstance(self.load, ecflow.Node):
            return "%s != " % self + str(node)
        return False

    def __and__(self, node):
        if isinstance(self.load, ecflow.Node):
            return "%s and " % self + str(node)
        return False

    def __or__(self, node):
        if isinstance(self.load, ecflow.Node):
            return "%s or " % self + str(node)
        return False

    def get_abs_node_path(self): return self.fullname()

    def fullname(self):
        """ simple syntax """
        if isinstance(self.load, ecflow.Node):
            return self.load.get_abs_node_path()
        return str(self)

    def repeat(self, name="YMD", start=20120101, end=20321212, step=1,
               kind="date"):
        """ add repeat attribute"""
        if kind in ("date", "integer"):
            self.add(Repeat(name, start, end, step, kind))
        elif kind in ("string", "enumerated"):
            self.add(Repeat(name, start, kind=kind))
        elif kind == "day":
            self.add(Repeat(step, kind="day"))
        else: raise Exception
        return self

    def defstatus(self, kind):
        """ add defstatus attribute"""
        status = kind
        if type(kind) == str:
            kinds = {"suspended": ecflow.DState.suspended,
                     "aborted": ecflow.DState.aborted,
                     "complete": ecflow.DState.complete,
                     "active": ecflow.DState.active,
                     "submitted": ecflow.DState.submitted,
                     "unknown": ecflow.DState.unknown,
                     "queued": ecflow.DState.queued, }
            status = kinds[kind]
        elif type(kind) == ecflow.DState: pass
        else: raise Exception("defstatus?")
        self.load.add_defstatus(status)
        return self

    def __add__(self, item): self.add(item)

    def append(self, item=None, *args):
        """ we get compatible with list then """
        return self.add(item, args)

    def add(self, item=None, *args):
        """ add a task, a family or an attribute """
        if DEBUG: print(self.fullname(), item, args)

        if item is not None:
            if type(item) == tuple:
                for val in item:
                    self.add(val)
            elif type(item) == list:
                for val in item:
                    self.add(val)
            elif type(item) == str:
                raise Exception(item)
            else:
                item.add_to(self)

        if len(args) > 0:
            if type(args) == tuple:
                for val in args:
                    self.add(val)
            elif type(args) == list:
                for val in args:
                    self.add(val)
            else: raise Exception
            return self

        if not isinstance(self.load, ecflow.Node):
            raise Exception("please dont", type(self))

        return self

    def limit(self, name, size):
        """ add limit attribute"""
        if name is None:
            raise Exception
        self.load.add_limit(name, size)
        return self

    def inlimit(self, full_path):
        """ add inlimit attribute"""
        if not USE_LIMIT:
            return self

        try:
            path, name = full_path.split(":")
        except ValueError:
            raise ValueError('invalid limit: {}'.format(full_path))

        self.load.add_inlimit(name, path)
        return self

    # follow pyflow
    shape = None
    def draw_tree(self): dot = Dot(fullnames=False); self._tree(dot); return dot

    def _tree(self, dot):
      try: dot.edge(self.load.get_parent(), self)
      except: pass
      for n in self.load.nodes:
        if n.name() != '_': _tree(dot, n)
        # if n.names[0] != '_': n._tree(dot)

    def draw_graph(self):
        dot = Dot(); self._graph(dot); dot.save("test.gv"); return dot

    def _graph(self, dot):
      for n in self.load.nodes:
        if n.name() != '_': _tree(dot, n)
      # for n in self._nodes.values():
      #  if n.names[0] != '_': n._tree(dot)

    # @property
    def to_html(self):
        # from .html import HTMLWrapper
        return "%s" % HTMLWrapper("%s" % self)  # .generate_node()))

    def _repr_html_(self): return str(self.to_html())


def _tree(dot, node):
      if type(node) in (ecflow.Node, ecflow.Family, ecflow.Task,
                        ecflow.ecflow.Family, ecflow.ecflow.Task):
          dot.edge(node.get_parent(), node)
          for n in node.nodes:
              if n.name() != '_': _tree(dot, n)
      elif type(node) in (Node, Family, Task):
          dot.edge(node.load.parent, node)
          _tree(dot, node.load)
      else: raise Exception(type(node))
      #try: dot.edge(node.load.parent, self)
      #except: pass


class Node(Root): # from where Task and Family derive
    """ Node class is shared by family and task """

    def __enter__(self): return self
    def __exit__(self, exc_type, exc_val, exc_tb): pass  # return self

    def events(self):
        return [event for event in self.load.events]

    def meters(self):
        return [meter for meter in self.load.meters]

    def name(self):
        return self.load.name()

    # def event(self, name=1):
    #     """ add event attribute"""
    #     if USE_EVENT:
    #         self.load.add_event(name)
    #     return self

    # def meter(self, name, start, end, threshold=None):
    #     """ add meter attribute"""
    #     if threshold is None:
    #         threshold = end
    #     self.load.add_meter(name, start, end, threshold)
    #     return self

    # def label(self, name, default=""):
    #     """ add label attribute"""
    #     self.load.add_label(name, default)
    #     return self

    # def edit(self, name, value=""):
    #     """ add variable attribute"""
    #     self.load.add_variable(name, value)
    #     return self

    def variable(self, name, value=""): return self.edit(name, value)

    def cron(self, time, dom=False, wdays=False, month=False):
        """ wrapper for add_cron """
        cron = ecflow.Cron()
        cron.set_time_series(time)
        if wdays is not False:
            cron.set_week_days(wdays)
        if month is not False:
            cron.set_months(month)
        if dom is not False:
            cron.set_day_of_month(dom)

        self.load.add_cron(cron)
        return self

    def complete(self, arg):
        """ add complete attribute"""
        if USE_TRIGGER and arg is not None:
            self.load.add_complete(arg)
        return self

    def complete_and(self, arg):
        """ append to existing complete"""
        if USE_TRIGGER and arg is not None:
            self.load.add_part_complete(ecflow.PartExpression(arg, True))
        return self

    def complete_or(self, arg):
        """ append to existing complete"""
        if USE_TRIGGER and arg is not None:
            self.load.add_part_complete(ecflow.PartExpression(arg, False))
        return self

    def up(self):
        """ get parent, one level up"""
        return self.load.get_parent()


class Defs(object):
    """ wrapper for the definition """

    def __init__(self):
        self.load = ecflow.Defs()

    def __get_attr__(self, attr): 
        return getattr(self.load, attr)

    def auto_add_externs(self, true): self.load.auto_add_externs(true)
    def check(self): self.load.check()
    def add_extern(self, path): self.load.add_extern(path)  # TODO list
    def suites(self): return [suite for suite in self.load.suites]

    def add_suite(self, node): 
        if type(node) == Suite:
            self.load.add_suite(node.load)  # TODO list
        elif type(node) == ecflow.Suite:
            self.load.add_suite(node)  # TODO list
        else: raise Exception("what?")

    def __str__(self): return "%s" % self.load
    __repr__ = __str__

    def add(self, item):
        """ add suite """
        if type(item) == Suite:
            self.load.add_suite(item.load)
        #elif type(item) == Extern:  # obsolete, Extern uses global DEFS var
        #    for path in item.folk: self.load.add_extern(path)
        elif type(item) == tuple:
            for one in item:
                self.add(one)
        elif type(item) == list:
            for one in item:
                self.add(one)
        elif item is None:
            pass
        else:
            raise Exception("ERR:load addwhat?", type(item))
        return self

    def suite(self, name):
        """ add suite providing its name """
        suite = Suite(name)
        self.add(suite)
        return suite


class Client(object):
    """ wrapper around client """

    def __init__(self, host="localhost", port="31415"):
        if host is None: host = "localhost"
        self.clnt = ecflow.Client(host, port)
        if "@" in host:
            host, port = host.split("@")
            self.clnt.set_host_port(host, int(port))
        else:
            super(Client, self).__init__()
            self.clnt.set_host_port(host, int(port))
        self.host = host
        self.port = port

    def load(self, defs):
        if type(defs) == Defs:
            self.clnt.load(defs.load)
        elif type(defs) == ecflow.Defs:
            self.clnt.load(defs)
        else: raise Exception("defs: really?")

    def replace(self, path, defs=None, parent=True, force=False):
        if defs is None: self.clnt.replace(path, DEFS, parent, force)
        elif type(defs) is Defs:
            self.clnt.replace(path, defs.load, parent, force)
        else: self.clnt.replace(path, defs, parent, force)

    def suites(self): return self.clnt.suites()
    def ping(self): self.clnt.ping()

    def __str__(self):
        return "ecflow client %s@%s v%s" % (
            self.host, self.port, self.version())


class Suite(Root):
    """ wrapper for a suite """

    def __enter__(self): return self
    def __exit__(self, exc_type, exc_val, exc_tb): pass  # return self

    def __init__(self, name):
        self.load = ecflow.Suite(name)

    def name(self): return self.load.name()

    def family(self, name):
        """ add a family """
        if "%" in name: raise Exception(name)
        obsolete()
        fam = Family(str(name))
        self.add(fam)
        return fam

    def task(self, name):
        """ add a task """
        if "%" in name: raise Exception(name)
        obsolete()
        tsk = Task(name)
        self.add(tsk)
        return tsk

    # def __enter__(self): return self

    # def __exit__(self, *args): pass


class Family(Node, Attribute):
    """ wrapper around family """

    def __init__(self, name):
        self.load = ecflow.Family(name)

    def family(self, name):
        """ add a family """
        if "%" in name: raise Exception(name)
        obsolete()
        fam = Family(str(name))
        self.add(fam)
        return fam

    def task(self, name):
        """ add a task """
        if "%" in name: raise Exception(name)
        obsolete()
        tsk = Task(name)
        self.add(tsk)
        return tsk

    def add_to(self, node):
        if DEBUG: print("add: %s %s" % (node.name(), self.name()))
        parent = self.load.get_parent()
        if parent: raise Exception("already attached...",
                                   parent.name(),
                                   node.name(), self.name())
        if type(node) in (Suite, Family, Task):
            node.load.add_family(self.load)
        else: node.add_family(self.load)

    def nodes(self): return [node for node in self.load.nodes]
    # def __enter__(self): return self

    # def __exit__(self, *args): pass


class Task(Node, Attribute):
    """ wrapper around task """

    def __init__(self, name):
        self.load = ecflow.Task(name)

    # def __setattr__(self, key, val):
    #     # assert key.isupper()
    #     if key.isupper():
    #         key, val = translate(key, val)
    #         print(type(self.load), "%s" % self.load)
    #         self.load.add(ecflow.Variable(key, val))

    def add_to(self, node):
        if type(node) in (Family, Suite):
            node.load.add_task(self.load)
            return
        node.add_task(self.load)

    def add_family(self, node):
        raise Exception(self.name(), node.name(), self.fullname())

    def add_task(self, node):
        raise Exception(node.name(), self.name())


def display(defs, fname=None):
    """ print defs"""
    if fname is not None:
        fop = open(fname, "w")
        print(defs, file=fop)


class TestEcf(unittest.TestCase):
    """ a test case aka use-case """

    def test_xxx(self):
        """ a test """

        suite = Suite("Suite").add(Clock("real"),
                                     Autocancel(3))
        suite.defstatus("suspended")

        fam = Family("Family")
        tsk = Task("Task")

        ft2 = Task("Fam")
        ft2.add_to(fam)

        tsk.VAR = "VALUE"  # edit VAR "VALUE"
        tsk.add(Late("-s 00:05 -c 01:00"))

        fam.add(tsk,
                (Task("1"), Task("2")),
                [Task("11"), Task("12")],
                Task("111"), Task("211"),
                Task("t2").add(Trigger(tsk == COMPLETE),
                               Late("-s 00:05 -a 01:00 -c 20:00"),
                               Time("01:00")),
                Task("t21").add(Cron("00:00 23:59 01:00")),
                Task("t22").add(Today("14:00")),
                Task("t23").add(Date("1.*.*")),
                Task("t24").add(Day("monday")),
            )

        fam.add(Task("t3").add(
                If(test=(1 == 1),
                   then=Edit(ADD_ONE=1),
                   otow=Edit(ADD_TWO=1)),

                If(test=(1 == 0),
                   then=Edit(ADD_ONE=0),
                   otow=Edit(ADD_TWO=0)),
                Trigger(tsk != ABORTED),
                Complete(tsk == COMPLETE))) # longer

        fam.add(
            Task("2t"),
            Task("t4").add(
                Trigger(tsk.name() != COMPLETE)),
            Late("-s 00:05 -c 01:00"),
            Edit(VAR="VALUE"),
            Task("t5").add(Trigger(["t4", "t3", "t2"])),
            Task("t6").add(Trigger("2t" == COMPLETE)),
            Task("t7").add(Trigger("2t eq complete")), )

        tsk.add(Limit("a_limit", 10),
                Inlimit("a_task:a_limit"),
                Meter("step", -1, 100),
                Label("info", "none"),
                Event(1),
                Event("a"),
                Defstatus("complete"))

        Extern("/oper/main:YMD")
        tsk.add(Edit({"A": "a", "B": "b"}))  # dict
        tsk.add(Edit(D="d", E="e"))  # list name=value,
        tsk.add(Edit("C", "c"))  # name, value
        suite.add(fam,
                  Family("main").add(
                      Repeat("YMD", 20200202, 20320202, 7, "date"),
                      Task("doer").add(Time("04:55"))))

        fam.family("fam").add(DefcompleteIf(True))

        defs = Defs()
        defs.add(suite)
        another = defs.suite("suite")
        another.defstatus("suspended")
        another.task("tn")
        afam = another.family("family")
        afam.task("t2n")

        display(defs, fname="test_ecf.tmp")
        suite.draw_tree()
        suite.draw_graph()
        suite.to_html()

        print("%s" % suite)  # print name
        print(suite)  # print name
        print(repr(suite))  # print content
        print(defs)  # print content
        cmd = "xdg-open test.gv.pdf;"
        cmd += "dot -Tps test.gv > test.ps && xdg-open test.ps"; os.system(cmd)


# from pyflow, COMPAT
def definition_to_html(d):
    result = []
    for n in d.split("\n"): result.append(n)
    return "<pre>%s</pre>" % ("\n".join(result),)


class HTMLWrapper(object):
    def __init__(self, d): self._def = d
    def _repr_html_(self): return definition_to_html(self._def)
    __str__ = _repr_html_


def shapes(node):
    if type(node) in (ecflow.Task, Task):
        return "triangle"
    elif type(node) in (ecflow.Family, Family):
        return "rect"
    elif type(node) in (ecflow.Suite, Suite):
        return "invtriangle"
    return None


class Dot(object):
  """ follow tracks from pyflow """

  def __init__(self, fullnames=True):
      from graphviz import Digraph
      self._dot = Digraph()
      self._nodes = {}
      self._fullnames = fullnames

  def edge(self, node1, node2):
      self._dot.edge(self.node(node1), self.node(node2))

  def node(self, node):
      full = node.get_abs_node_path()
      if full not in self._nodes:
          self._nodes[full] = '%s' % node.name()
      self._dot.node(self._nodes[full], 
                     # fillcolor="shape",
                     # fontcolor="blue",
                     # fontsize=32,
                     # width=0.5,
                     # style="filled",
                     # fixedsize="shape",
                     # fixedsize="true",
                     # label="xxx",
                     shape=shapes(node))
      return self._nodes[full]

  def save(self, path, view=True):
      if os.path.exists(path): os.unlink(path)
      self._dot.render(path, view=view)

  # For jupyter notbeook
  def _repr_svg_(self):
      return self._dot._repr_svg_()


if __name__ == '__main__':
    unittest.main()
