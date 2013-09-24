#!/usr/bin/env python
# This software is provided under the ECMWF standard software license agreement.
""" a layer over raw ecflow api """

import unittest

from ecflow import JobCreationCtrl as JobCreationCtrl
# from ecflow import Client as Client
from ecflow import ChildCmdType, TimeSlot
from ecflow import ZombieType, ZombieAttr, ZombieUserActionType
import ecflow, argparse

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
ECF_MODE = "ecflow"
# ECF_MODE = "sms"

if DECORATE == "ONLY_TRIGGER":
    USE_TRIGGER = True 
    USE_LIMIT = False 
    USE_EVENT = False
elif DECORATE == "NO_ATTRIBUTE":
    USE_TRIGGER = False
    USE_LIMIT = False
    USE_EVENT = False
elif DECORATE == "ONLY_EVENT":
    USE_TRIGGER = False
    USE_LIMIT = False
    USE_EVENT = True
elif DECORATE == "NO_TRIGGER":
    USE_TRIGGER = False
    USE_LIMIT = True
    USE_EVENT = True
elif DECORATE == "ALL":
    USE_TRIGGER = True
    USE_LIMIT = True
    USE_EVENT = True
else:
    raise BaseException


def translate(name, value=None):
    """ Translate from sms to ecflow """
    import sms2ecf
    if ECF_MODE != "sms":
        return name, value
    else: sms2ecf.ECF_MODE = ECF_MODE
    return sms2ecf.translate(name, value)

class State:
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
        """ when == is used, we should care about task name starting with 0-9"""
        if type(arg) == str:
            add = ""
            if type(arg[0]) == int:
                add = "./"                
            return add + arg + " == " + self.state
        elif isinstance(arg, ecflow.Node):
            return arg.get_abs_node_path() + " == " + self.state
        return False

    def __ne__(self, arg):
        """ aka != """
        if type(arg) == str:
            add = ""
            if type(arg[0]) == int:
                add = "./"                
            return add + arg + " != " + self.state
        elif isinstance(arg, ecflow.Node):
            return arg.get_abs_node_path() + " != " + self.state
        return False

    def value(self):
        """ return state """
        return self.state

    def eval(self, node):
        """ return state """
        return self.state == node.get_state()



SUBMITTED = State("submitted")
ACTIVE    = State("active")
SUSPENDED = State("suspended")
ABORTED   = State("aborted")
QUEUED    = State("queued")
COMPLETE  = State("complete")
UNKNOWN   = State("unknown")

class Attribute(object):
    """ generic attribute to be attached to a node"""
    def add_to(self, node):
        """ use polymorphism to attach attribute to a node"""
        pass

class Variable(Attribute, ecflow.Variable):
    """ filter variables to be attached to a node, may alter its name SMS-ECF"""
    def add_to(self, node):
        """ add_variable"""
        keyt, valt = translate(self.name(), self.value())
        node.add_variable(keyt, valt)

class Label(Attribute, ecflow.Label):
    """ wrap around label"""
    def add_to(self, node):
        """ add_label"""
        node.add_label(self)

class Meter(Attribute, ecflow.Meter):
    """ wrap around meter"""
    def add_to(self, node):
        """ add_meter"""
        node.add_meter(self)

class Event(Attribute, ecflow.Event):
    """ wrap around event"""
    def add_to(self, node):
        """ add_event"""
        node.add_event(self)

class InLimit(Attribute):
    """ a class to host a path for a limit
        silently ignore if USE_LIMIT is False, 
        (in debug mode) """

    def __init__(self, fullpath):
        self.data = None
        if USE_LIMIT: 
            try: path, name = fullpath.split(":")
            except: name = fullpath; path = ""
            if name is None: 
                raise BaseException
            self.data  = ecflow.InLimit(name, path)
            self.path_ = path
            self.name_ = name

    def add_to(self, node):
        """ add_inlimit"""
        if USE_LIMIT and self.data is None: 
            raise BaseException
        if  USE_LIMIT and self.data is not None:
            node.add_inlimit(self.data)

    def value(self):        
        """ get limit fullpath-name """
        return self.path_ + ":" + self.name_

    def name(self):
        """ get limit name """
        return self.name_

class Inlimit(InLimit):
    def __init__(self, fullpath):
        super(Inlimit, self).__init__(fullpath)

class Trigger(Attribute):
    """ add trigger (string, list of task names, or directly
           expression and: and'ed (True) or or'ed (False) unk: add or
           [name]==unknown for RD
    """
    def __init__(self, expr, unk=False, anded=True):
        self.expr = None
        if expr is None: 
            return
        if expr == "": 
            return
        if type(expr) == str:
            # self.expr = ecflow.Expression(expr)
            self.expr = expr
            return

        if type(expr) == tuple:
            prep = list(expr)
            expr = prep

        if type(expr) == list:
            for index, name in enumerate(expr):
                if name == None: 
                    continue
                pre = ""

                if type(name) in (Node, Task, Family, Suite):
                    fullname = name.fullname()
                    name = fullname
                elif type(name) in (ecflow.Task, ecflow.Family):
                    name = name.name()

                if name[0].isdigit(): # in "0123456789":
                    pre = "./"

                if ':' in name:
                    item = pre + name
                else:
                    item = pre + "%s == complete" % name
                    if unk:
                        item += " or %s%s==unknown" % (pre, name)

                if index == 0:
                    self.expr = ecflow.Expression(item)
                else:
                    self.expr.add(ecflow.PartExpression(item,anded))

        elif type(expr) in (ecflow.Expression, 
                            ecflow.PartExpression):
            self.expr = ecflow.Expression(str(item))
        else:
            print type(expr)
            raise Exception("what? trigger?")

    def add_to(self, node):
        if not USE_TRIGGER:
            return
        if self.expr is None:
            return
        if node.get_trigger() is None:
            node.add_trigger(self.expr)
        else:
            node.add_part_trigger(ecflow.PartExpression(self.expr, True))

class TriggerAnd(Trigger):
    def __init__(self, expr, unk=False, anded=True):
        # super(TriggerAnd, self).__init__(expr, unk, anded)
        self.expr = expr
        if not ":" in expr and not " eq " in expr and not "==" in expr:
            self.expr += " eq complete"
    def add_to(self, node):        
        node.add_part_trigger(ecflow.PartExpression(self.expr, True))

class TriggerImpossible(Trigger):
    """ attribute to be added to node when it is not expected to run
    any task"""

    def __init__(self):
        """ add an 'impossible trigger', for a task not to run """
        super(TriggerImpossible, self).__init__("1==0")

class TriggerAlways(Trigger):
    """ attribute to be added to node when it is not expected to run
    any task"""

    def __init__(self):
        """ add an 'impossible trigger', for a task not to run """
        super(TriggerAlways, self).__init__("1==1")

class Complete(Trigger):
    """ class to host complete expression, added later to a node"""
    def __init__(self, expression, unk=False, anded=False):
        super(Complete, self).__init__(expression, unk, anded)

    def add_to(self, node):
        if USE_TRIGGER and self.expr is not None:
            node.add_complete(self.expr)

class Clock(Attribute):
    """ wrapper to add clock """
    def __init__(self, arg):
        if type(arg) == str:
            hybrid = "hybrid" in arg
            hhh, mmm, sss = [0, 0, 0]
            try:
                hhh, mmm, sss = arg.split(':')
                self.data = ecflow.Clock(hhh, mmm, sss, hybrid)
            except:
                self.data = ecflow.Clock(hybrid)
        else:
            self.data = ecflow.Clock(arg)

    def add_to(self, node):
        if type(node) != Suite:
            print "WAR: clock can only be attached to suite node, "
            print "WAR: clock is ignored"
            return
        node.add_clock(self.data)

class AutoCancel(Attribute):
    """ wrapper to add time """
    def __init__(self, arg):
        if type(arg) == str:
            hhh, mmm = arg.split(':')
            rel = '+' in arg
            self.data = ecflow.Autocancel(int(hhh), int(mmm), rel)
        else:
            self.data = ecflow.Autocancel(arg)

    def add_to(self, node):
        node.add_autocancel(self.data)

class Time(Attribute):
    """ wrapper to add time """
    def __init__(self, arg):
        self.data = arg

    def add_to(self, node):
        if USE_TIME and self.data is not None:
            node.add_time(self.data)

class Today(Time):
    """ wrapper to add time """
    def __init__(self, arg):
        self.data = arg

    def add_to(self, node):
        if USE_TIME and self.data is not None:
            node.add_today(self.data)

class Cron(Time):
    """ wrapper to add time """
    def __init__(self, bes, wdays=None, days=None, months=None):
        if type(bes) == ecflow.Cron:
            self.data = arg
            return

        # parser = argparse.ArgumentParser()
        # parser.add_argument("-w", default=0, help= "weekdays")
        # parser.add_argument("-d", default=0, help= "days")
        # parser.add_argument("-m", default=0, help= "months")
        # parser.add_argument("bes", nargs='*', type=str, help= "begin end step")
        # parsed = parser.parse_args(arg)

        # self.data = ecflow.Cron()
        # if parsed.w: self.data.set_week_days(parsed.w)
        # if parsed.d: self.data.set_days_of_month(parsed.d)
        # if parsed.m: self.data.set_months(parsed.m)
        # cli = ' '.join(''.join(parsed.bes).split(' '))
        # print cli, parsed.bes, parsed.w, parsed.d, parsed.m
        # self.data.set_time_series(cli)
        self.data = ecflow.Cron()
        if wdays: 
            self.data.set_week_days([int(x) for x in wdays.split(',')])
        if days: 
            self.data.set_days_of_month([int(x) for x in days.split(',')])
        if months: 
            self.data.set_months([int(x) for x in months.split(',')])
        self.data.set_time_series(bes)
    def add_to(self, node):
        if USE_TIME and self.data is not None:
            node.add_cron(self.data)

class Date(Time):
    """ wrapper to add date """

    def __init__(self, arg, mask=False):
        super(Date, self).__init__(arg)
        self.mask = mask

    def add_to(self, node):
        if USE_TIME and self.data is not None:
            ### ??? FIX, emos avoids dates, datasvc would not
            if self.mask:
                node.add_variable("DATEMASK", self.data)
            else:
                ddd, mmm, yyy = self.data.rsplit('.')
                if ddd == '*': ddd = 0
                if mmm == '*': mmm = 0
                if yyy == '*': yyy = 0

                node.add_date(int(ddd), int(mmm), int(yyy))
                # node.add_date(self.data)

class Day(Date):
    """ wrapper to add day """
    def add_to(self, node):
        if USE_TIME and self.data is not None:
            # node.add_date(self.data) ### FIX
            node.add_variable("WEEKDAY", self.data)

class Defcomplete(Attribute):
    """ wrapper to add defstatus complete """
    def __init__(self):
        pass

    def add_to(self, node):
        node.defstatus("complete")

class Defstatus(Defcomplete):
    """ add defstatus attribute"""
    def __init__(self, kind):
        if type(kind) == str:
            kinds = {"suspended": ecflow.DState.suspended,
                     "aborted": ecflow.DState.aborted,
                     "complete": ecflow.DState.complete,
                     "active": ecflow.DState.active,
                     "submitted": ecflow.DState.submitted,
                     "unknown": ecflow.DState.unknown,
                     "queued": ecflow.DState.queued,
                     }
            self.data = kinds[kind]
        else:
            self.data = kind

    def add_to(self, node):
        node.add_defstatus(self.data)

class DefcompleteIf(Defcomplete):
    """ wrapper to add conditional defstatus complete 
    just change name to make it explicit
    """
    def __init__(self, arg=True):
        # super(DefcompleteIf, self).__init__()
        self.data = arg

    def add_to(self, node):
        if self.data:
            node.defstatus("complete")
        # else: node.defstatus("queued") # in comment to prevent
        # overwrite when using multiple defcomplete

class Limit(Attribute):
    """ wrapper to add limit """

    name = None
    size = 1

    def __init__(self, nam=None, lim=1):
        self.name = nam
        self.size = lim

    def add_to(self, node):
        if USE_LIMIT and self.name is not None:
            node.add_limit(self.name, self.size)

class Late(Attribute):
    """ wrapper around late, to be add'ed to families and tasks """

    def __init__(self, arg):
        self.data = None
        # if not USE_LATE: if 1: raise
        # return
        sub = False
        act = False
        com = False
        rel = False
        self.data = ecflow.Late()
        for item in arg.split(" "):
            if   item == "-s": 
                sub = True
            elif item == "-c": 
                com = True
            elif item == "-a": 
                act = True
            else:
                hour, mins = item.split(":")
                rel = "+" in hour
                if   sub: 
                    self._add_sub(hour, mins)
                elif com: 
                    self._add_com(hour, mins, rel)
                elif act: 
                    self._add_act(hour, mins, rel)
                sub = False
                act = False
                com = False

    def _add_sub(self, hour, mins):
        """ submitted"""
        self.data.submitted(int(hour), int(mins))

    def _add_com(self, hour, mins, rel):
        """ complete"""
        self.data.complete(ecflow.TimeSlot(int(hour), int(mins)), rel)

    def _add_act(self, hour, mins, rel):
        """ active"""
        self.data.active(ecflow.TimeSlot(int(hour), int(mins)), rel)

    def add_to(self, node):
        if USE_LATE and self.data is not None:
            node.add_late(self.data)

class Variables(Attribute):
    """ dedicated class to enable variable addition with different
    syntax """
    
    def _set_tvar(self, key, val):        
        """ facilitate to load a ecflow suite to SMS, translating
        variable names"""
        keyt, valt = translate(str(key), str(val))
        if self.data is None:
            self.data = Variable(keyt, valt)
        else:
            next = self.next
            self.next = Variables(keyt, valt, next)

    def __init__(self, __a=None, __b=None, __next=None, *args, **kwargs):
        self.data = None
        self.next = __next

        if len(args) > 0:
            if type(args) == list:
                for item in args.iteritems():
                    self._set_tvar(item.name(), item.value())
            elif type(args) == tuple:
                for key, val in args.items():
                    self._set_tvar(key, val)
            else:
                raise BaseException()
        if len(kwargs) > 0: 
            for key, val in kwargs.items():
                self._set_tvar(key, val)
        if type(__a) == dict:
            for key, val in __a.items():
                self._set_tvar(key, val)
        elif type(__a) == tuple: 
            raise BaseException()
            # for key, val in __a.items(): self._set_tvar(key, val)
        elif type(__a) == list: raise BaseException()
        elif type(__a) == Variable: self.data = __a
        elif __a is not None and __b is not None:
            self._set_tvar(__a, __b)
        elif __a is None and __b is None: pass
        else:
            print __a, __b, __next, args, kwargs; 
            raise BaseException()

    def add_to(self, node):
        if self.data is not None:
            node.add_variable(self.data)
        if self.next is not None:
            self.next.add_to(node)

    def add(self, what):
        print what.fullname()
        raise baseException

class Limits(Attribute):
    """ dedicated class to enable limits addition with different syntax """
    
    def _set_tvar(self, key, val):        
        """ append limits """
        if self.data is None:
            self.data = Limit(key, val)
        else:
            next = self.next
            self.next = Limits(key, val, next)

    def __init__(self, __a=None, __b=None, __next=None, *args, **kwargs):
        self.data = None
        self.next = __next

        if len(args) > 0:
            if type(args) == list:
                for item in args.iteritems():
                    self._set_tvar(item.name(), item.value())
            elif type(args) == tuple:
                for key, val in args.items():
                    self._set_tvar(key, val)
        elif len(kwargs) > 0: 
            for key, val in kwargs.items():
                self._set_tvar(key, val)
        elif type(__a) == dict:
            for key, val in __a.items():
                self._set_tvar(key, val)

        if __a is not None and __b is not None:
            self._set_tvar(__a, __b)

    def add_to(self, node):
        if self.data is not None:
            node.add_limit(self.data)
        if self.next is not None:
            self.next.add_to(node)

class Repeat(Attribute):
    def __init__(self, name="YMD", start=20120101, end=20120101, step=1, kind="date"):
       if kind == "date":
           self.data = ecflow.RepeatDate(name, int(start), int(end), 
                                         int(step))
       elif kind == "integer":
           self.data = ecflow.RepeatInteger(name, int(start), int(end), int(step))
       elif kind == "string":
           self.data = ecflow.RepeatString(name, start)
       elif kind == "enumerated":
           self.data = ecflow.RepeatEnumerated(name, start)
       elif kind == "day":
           self.data = ecflow.RepeatDay(step)        
       else:
           self.data = None

    def add_to(self, node):
        if self.data is not None:
            node.add_repeat(self.data)

def If(test=True, then=None, otow=None):
    """ enable Task("t1").add(If(test= (1==1),
                                 then= Variables(ONE=1),
                                 otow= Variables(TWO=2)))
        appreciate that both branches are evaluated, using this If class
        ie there is no 'dead code' as it is with python language 'if' structure

        using If to distinguish od/rd mode request that both users share
        the variables (parameter.py) and ecf.py
                                 
        otow: on the other way?
        """
    if test:
        return then
    return otow

class Root(object): # from where Suite and Node derive
    """ generic tree node """

    def __str__(self):
        if isinstance(self, ecflow.Node):
            return self.fullname()
        return str(self)

    def __eq__(self, node):
        if isinstance(self, ecflow.Node):
            return "%s == " % self.fullname() + str(node)
        return False

    def __ne__(self, node):        
        if isinstance(self, ecflow.Node):
            return "%s != " % self.fullname() + str(node)
        return False

    def __and__(self, node):
        if isinstance(self, ecflow.Node):
            return "%s and " % self.fullname() + str(node)
        return False

    def __or__(self, node):
        if isinstance(self, ecflow.Node):
            return "%s or " % self.fullname() + str(node)
        return False

    def fullname(self):
        """ simple syntax """
        if isinstance(self, ecflow.Node):
            return self.get_abs_node_path()
        return str(self)
    
    def repeat(self, name="YMD", start=20120101, end=20321212, step=1,
               kind="date"):
        """ add repeat attribute"""
        if kind == "date":
            self.add_repeat(ecflow.RepeatDate(name, int(start), int(end), 
                                              int(step)))
        elif kind == "integer":
            self.add_repeat(ecflow.RepeatInteger(name, int(start), int(end), int(step)))
        elif kind == "string":
            self.add_repeat(ecflow.RepeatString(name, start))
        elif kind == "enumerated":
            self.add_repeat(ecflow.RepeatEnumerated(name, start))
        elif kind == "day":
            self.add_repeat(ecflow.RepeatDay(step))
        else:
            raise BaseException
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
                     "queued": ecflow.DState.queued,
                     }
            status = kinds[kind]
        self.add_defstatus(status)
        return self

    def add(self, item=None, *args):
        """ add a task, a family or an attribute """
        if DEBUG: print self.fullname()

        if item is not None:
            if type(item) == tuple:
                for val in item:
                    self.add(val)
            elif type(item) == list:
                for val in item:
                    self.add(val)
            else:
                try:
                    item.add_to(self)
                except Exception, exc:
                    try: print item.fullname(), item.name(), self.name()
                    except: pass
                    print "not yet", self, type(item), item, exc 
                    raise BaseException

        if len(args) > 0:
            if type(args) == tuple:
                for val in args:
                    self.add(val)
            elif type(args) == list:
                for val in args:
                    self.add(val)
            else:
                raise BaseException()

        if not isinstance(self, ecflow.Node):
            print "you don't want that"
            raise BaseException()

        return self
 
    def limit(self, name, size):
        """ add limit attribute"""
        if name is None: 
            raise BaseException
        self.add_limit(name, size)
        return self

    def inlimit(self, full_path):
        """ add inlimit attribute"""
        if not USE_LIMIT: 
            return self

        path, name = full_path.split(":")
        if name is None:
            raise BaseException
        if path is None:
            raise BaseException

        self.add_inlimit(name, path)
        return self

class Node(Root): # from where Task and Family derive
    """ Node class is shared by family and task """

    def add_limits(self, __a = None, __b = None, **kwargs):
        """ add limit dependency"""
        if isinstance(__a, basestring):
            self.add_limit(__a, __b)
        elif isinstance(__a, dict):
            assert __b is None
            for key, val in __a.items():
                self.add_limit(key, val)
        for key, val in kwargs.items():
            self.add_limit(key, val)
        return self

    def meter(self, name, start, end, threshold=None):
        """ add meter attribute"""
        if threshold == None:
            threshold = end
        self.add_meter(name, start, end, threshold)
        return self

    def label(self, name, default=""):
        """ add label attribute"""
        self.add_label(name, default)
        return self

    def event(self, name=1):
        """ add event attribute"""
        if USE_EVENT: 
            self.add_event(name)
        return self

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

        self.add_cron(cron)
        return self

    def today(self, hhmm): 
        """ wrapper around time """
        self.time(hhmm)
        return self # ???

    def time(self, hhmm):
        """ wrapper around time, None argument is silently ignored """
        if hhmm is not None:
            self.add_time(hhmm)
        return self

    def trigger(self, arg):
        """ add trigger attribute"""
        if USE_TRIGGER and arg is not None:
            self.add_trigger(arg)
        return self

    def trigger_and(self, arg):
        """ append to existing trigger"""
        if USE_TRIGGER and arg is not None:
            self.add_part_trigger(ecflow.PartExpression(arg, True))
        return self

    def trigger_or(self, arg):
        """ append to existing trigger"""
        if USE_TRIGGER and arg is not None:
            self.add_part_trigger(ecflow.PartExpression(arg, False))
        return self

    def complete(self, arg):
        """ add complete attribute"""
        if USE_TRIGGER and arg is not None:
            self.add_complete(arg)
        return self

    def complete_and(self, arg):
        """ append to existing complete"""
        if USE_TRIGGER and arg is not None:
            self.add_part_complete(ecflow.PartExpression(arg, True))
        return self

    def complete_or(self, arg):
        """ append to existing complete"""
        if USE_TRIGGER and arg is not None:
            self.add_part_complete(ecflow.PartExpression(arg, False))
        return self

    def up(self):
        """ get parent, one level up"""
        return self.get_parent()

class Defs(ecflow.Defs):
    """ wrapper for the definition """
    def add(self, suite):
        """ add suite """
        self.add_suite(suite)
        return suite

    def suite(self, name):
        """ add suite providing its name """
        suite = Suite(name)
        self.add(suite)
        return suite

class Client(ecflow.Client):
    """ wrapper around client """
    def __init__(self, host="localhost", port="31415"):
        if "@" in host:
            host, port = host.split("@")
            super(Client, self).__init__(host, int(port))            
        else:
            super(Client, self).__init__(host, int(port))
        self.host = host
        self.port = port

    def __str__(self):
        return "#MSG: ecflow client %s@%s v%s" % (self.host, self.port, 
                                                 self.version())

class Suite(ecflow.Suite, Root):
    """ wrapper for a suite """

    def family(self, name):
        """ add family """
        fam = Family(name)
        self.add_family(fam)
        return fam

    def task(self, name):
        """ add family """
        tsk = Task(name)
        self.add_task(tsk)
        return tsk

    # def __enter__(self):
    #     return self

    # def __exit__(self, *args):
    #     pass

class Family(ecflow.Family, Node, Attribute):
    """ wrapper around family """

    def family(self, name):
        """ add a family """
        fam = Family(name)
        self.add_family(fam)
        return fam

    def task(self, name):
        """ add a task """
        tsk = Task(name)
        self.add_task(tsk)
        return tsk

    def add_to(self, node):        
        node.add_family(self)

    # def __enter__(self):
    #     return self

    # def __exit__(self, *args):
    #     pass

class Task(ecflow.Task, Node, Attribute):
    """ wrapper around task """

    def __setattr__(self, key, val):
        # assert key.isupper()
        if key.isupper():
            key, val = translate(key, val)
        self.add_variable(key, val)

    def add_to(self, node):
            node.add_task(self)

def display(defs, fname=None):
    """ print defs"""
    if fname is None:
        print defs
    else:
        fop = open(fname, "w")
        print >> fop, defs


class TestEcf(unittest.TestCase):
    """ a test case """
    def test_xxx(self):
        """ a test """

        suite = Suite ("a_suite")
        suite.defstatus("suspended")

        fam = Family("a_family")
        tsk = Task("a_task")

        ft2 = Task("a_fam")
        ft2.add_to(fam)

        tsk.VAR = "VALUE"                         # edit VAR "VALUE"
        tsk.add(Late("-s 00:05 -c 01:00"))
        fam.add(tsk,

                (Task("1"), Task("2")), 
                [Task("11"), Task("12")], 
                Task("111"), Task("211"),

                Task("t2").add(Trigger(tsk == COMPLETE),
                               Time("01:00"))            )

        fam.add(Task("t3").add(
                If(test= (1==1),
                   then= Variables(ADD_ONE=1),
                   otow= Variables(ADD_TWO=1)),

                If(test= (1==0),
                   then= Variables(ADD_ONE=0),
                   otow= Variables(ADD_TWO=0)),
                Trigger(tsk != ABORTED),
                Complete(tsk == COMPLETE))) # longer

        fam.add(
            Task("2t"),
            Task("t4").add(
                Trigger(tsk.name() != COMPLETE)),
            Late("-s 00:05 -c 01:00"),
            Variables(VAR="VALUE"),
            Task("t5").add(Trigger(["t4", "t3", "t2"])),
            Task("t6").add(Trigger("2t" == COMPLETE)),
            Task("t7").add(Trigger("2t eq complete")),
            )

        tsk.add(Limit("a_limit", 10),
                InLimit("a_task:a_limit"),
                Meter("step", -1, 100),
                Label("info", "none"),
                Event(1),
                Event("a"),
                Defcomplete())
        
        tsk.add(Variables({"A": "a", "B": "b"}))
        tsk.add(Variables(D="d", E="e"))
        tsk.add(Variables("C", "c"))
        suite.add(fam)

        fam.family("another").add(DefcompleteIf(True))

        defs = Defs()
        defs.add(suite)
        another = defs.suite("another")
        another.defstatus("suspended")
        another.task("tn")
        afam = another.family("another_fam")
        afam.task("t2n")

        display(defs, fname="test_ecf.tmp")

if __name__ == '__main__':
    unittest.main()
# http://intra.ecmwf.int/metapps/monitoring/sst/sst_ostia.png
# http://intra.ecmwf.int/metapps/monitoring/sst/sstdiff_ostia.png
