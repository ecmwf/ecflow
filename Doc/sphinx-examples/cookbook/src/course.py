#!/usr/bin/env python
""" suite builder example for 2013 ecFlow course """
import sys, os
sys.path.append('/home/ma/emos/def/ecflow')

import ecf as ec
from ecf import *
import inc_emos as ie # provides Seed class + system related dependencies 

# cd ~map/course/201303/ecflow; python course.py # task wrappers underneath
# consume: choices for a family matching the producer-consumer pattern
#   local family + remote family + BEWARE ECF_OUT + log-server example
# barber:  an example of a "dynamical suite", with a "family producer task"
# perl + python: example that task wrapper may not be ksh/bash scripts

import time
from datetime import date
import argparse

today = str(date.today()).replace('-', '')

############################################################################

class GenericFamily(object):
    """ provide structure for derived classes"""

    def make(self, node): 
        return node
    def main(self, node): 
        return BaseException # return node
    def arch(self, node): 
        return node

############################################################################

class NativePerl(GenericFamily):
    """ ksh is not the only language for task wrappers"""

    def __init__(self):
        self.name = "perl"

    def main(self, node):
        tsk = Task(self.name).add(
                Variables(ECF_MICRO= "^",
                          ECF_JOB_CMD= "^ECF_JOB^ > ^ECF_JOBOUT^ 2>&1"),
                Meter("step", -1, 100),
                Event("1"),
                Event("2"),
                Label("info", "none"),            
                )
        node.add(tsk)

############################################################################

class NativePython(NativePerl):
    """ ksh is not the only language for task wrappers"""

    def __init__(self):
        super(NativePerl, self).__init__()
        self.name = "python"

############################################################################

def _kind(prod=1, cons=1): return Variables(CONSUME= cons, PRODUCE=prod)
def _evt(): return (Event("p"), Event("c"))
def _leaf(name="produce", init=0, stop=100, step=1):
    add = None
    if type(stop) == int:
        add = Meter("step", -1, int(stop))
    return Task("%s" % name).add(
        _evt(),
        add,
        Variables(INIT= init,
                  STOP= stop,
                  STEP= step),)               

############################################################################
class FallBack(GenericFamily):    
    """ in some situation, user may want its family to continue, and
    repeat to increment, even when some tasks abort """
    
    def main(self, node=None):
        return Family("daily").repeat(name="YMD", 
                                      start=today, end=DATE_STOP).add(
            Task("action").add(Time("10:00")),
            Family("loop").add(
                Time("11:00"),
                Task("dummy").add(
                    TriggerImpossible(),
                    Complete("1==1"))),
            Task("fallback").add(
                Label("info", "force complete when action is aborted"),
                Time("10:55"),
                Trigger("action eq aborted"),
                Complete("action eq complete")))

class DailyInc(GenericFamily):    
    """ anopther method to have daily repeat increment, with aborted tasks"""

    def main(self, node=None):
        return Family("daily_inc").repeat(name="YMD", 
                                          start=today, end=DATE_STOP).add(
            Label("info", "requeue will reset repeat attribute!"),
            Complete("daily_inc/loop/dummy eq complete"),
            Task("action").add(Time("10:00")),
            Family("loop").add(
                Time("11:00"),
                Task("dummy").add(TriggerImpossible(),
                                  Complete("1==1"))))            

class Consume(GenericFamily):   
    """ producer-consumer pattern can be implemented in many ways"""

    def __init__(self):
        self.init = 0
        self.stop = 48
        self.step = 3

    def main(self, node):
        """ pass the parent node, so that absolute paths can be used 
        with triggers""" 
        path = node.fullname()

        top = node.family("consume").add(
            Variables(SLEEP= 10,
                      PRODUCE= 1, # default: tasks will do both
                      CONSUME= 1),
            Family("limits").add(Defcomplete(),
                                Limit("consume", 7)),
            Task("leader").add(
                Label("info", "set event to get produce1 leader"),
                Event("1"), # set/cleared by user
                Defcomplete()),

            # task does both, ie serial ###########################
            _leaf("produce", self.init, self.stop, self.step).add(
                Label("info", "both produce and consume in a task")),

            # meter will report about producer progress ###########
            Family("produce0").add(
                Label("info", "only produce"),
                _kind(1, 0),
                _leaf("produce", self.init, self.stop, self.step)),

            # serialy produced, create a new task for each step ###
            Family("produce1").add(
                    _kind(1, 0),
                    Label("info", "repeat, one job per step"),
                    _leaf("produce", init="%STEP%", stop="%STEP%", step=1).add(
                    Meter("step", -1, 100)))\
                    .repeat(kind="integer", name="STEP",
                            start=self.init,
                            end =self.stop,
                            step =self.step).add()
            )

        top.defstatus("suspended")
        fam = Family("produce2").add(         # parallel
            _kind(1, 0),
            Label("info", "limited, one task per step, step by 3"),
            Limit("prod", 5),
            InLimit("produce2:prod"))
        top.add(fam)
        for step in xrange(self.init, self.stop, self.step):
            fam.add(Family("%02d" % step).add(
                    Variables(STEP= step),
                    _leaf("produce", step, step, 1)))
        
        ######################
        lead = path + "/consume/leader:1"
        prod = path + "/consume/produce"
        
        top.add( ### trigger may be inside a task
            _leaf("consume", self.init, self.stop, self.step).add(
                Label("info", "trigger may be inside a task"),
                _kind(0, 1),
                InLimit("limits:consume"),
                Variables(CALL_WAITER= 1,
                          SLEEP= 3, # sleep less than producer
                          TRIGGER_EXPRESSION= prod + ":step ge $step or " +
                          prod + " eq complete",)),
            Family("consume1").add(
                Label("info", "explicit trigger, follow faster"),
                _kind(0, 1),
                Trigger("(consume1:STEP lt %s1:STEP and %s) or " % 
                        (prod, lead) + 
                        "(consume1:STEP lt %s0/produce:step and not %s) or " % 
                        (prod, lead) + # lt while both are repeat
                        "(%s1 eq complete and %s0 eq complete)" %
                        (prod, prod)
                        ),
                InLimit("limits:consume"),
                _leaf("consume", "%STEP%", "%STEP%", 1),
                ).repeat(kind="integer", name="STEP", 
                         start=self.init, end=self.stop, step=self.step))
        
        fam = Family("consume2").add( # parallel
            Label("info", "one task per step, step by three"),
            _kind(0, 1),
            Limit("consume", 5),
            InLimit("consume2:consume"))
        top.add(fam)
        for step in xrange(self.init, self.stop, self.step):
            fam.add(Family("%02d" % step).add(
                    Variables(STEP = step),
                    Trigger("(%02d:STEP le %s1:STEP and %s) or " %
                            (step, prod, lead) +                    
                            "(%02d:STEP le %s0/produce:step and not %s)" %
                            (step, prod, lead)),
                    _leaf("consume", init=step, stop=step, step=1)))

############################################################################

class Barber(GenericFamily):
    """ a 'barber shop' example with families created by a task """

    def _passer_by(self):
        """ generator """
        return Task("passby").add(
                Time("00:00 23:59 00:05"),
                Variables(ID=0),
                Label("info", ""),
                Label("rem", "this task alters its variable ID, " +
                      "aliases won't work natively"),
                InLimit("limits:passby"))
                
    def _client(self, node, position):
        """ python version of the family created initialy
        attention: raw definition file is located in passby task wrapper"""
        path = node.fullname() + "/limits"
        fam = node.family("list").family("%s" % position).add(
            AutoCancel(1),
            Task("cut").inlimit(path + ":barbers"),
            Task("pay").add(
                Trigger("cut eq complete"),
                InLimit(path + ":barbers"),
                InLimit(path + ":cashiers")),
            Task("leave").add(
                Label("info", ""),
                Trigger(["cut", "pay"])))
              
        fam.defstatus("complete")

    def _shop(self, node):
        fam = node.family("shop").defstatus("suspended").add(
            Variables(NB_CHAIRS= 4),
            Family("limits").add(Defcomplete(),
                                 Limit("passby", 1),
                                 Limit("barbers", 2),
                                 Limit("cashiers", 1)),
            self._passer_by(),
            )
        self._client(fam, 1),
                

    def main(self, node):
        self._shop(node)

############################################################################
def user(): 
    return os.getlogin()

def locate_scripts():
    pwd = os.getcwd()

    return Variables(
        ECF_HOME= "/tmp/%s/ecflow/" % user(), # pwd,
        ECF_FILES= pwd + "/scripts",
        ECF_INCLUDE= pwd + "/include", )

DATE_STOP = 20300115

class Course(ie.Seed):
    """ host families together """

    def __init__(self):
        super(Course, self).__init__()
        self.name = "course"

    def suite(self):
        """ define limits (empty) """
        
        node = Suite(user())
        node.defstatus("suspended").add(
            Variables(USER= user()),
            locate_scripts()) # 
        self.top(node)
        
        fp = open("/tmp/%s/" % user() + self.name + ".def", "w")
        print >> fp, node
        return node

    def top(self, node):
        barber_shop = Barber()
        perl = NativePerl()
        python = NativePython()
        consume = Consume()

        with node.family(self.name) as node:
            node.add(FallBack().main(),
                     DailyInc().main())
            barber_shop.main(node)
            perl.main(node)
            python.main(node)
            consume.main(node)
            return node

###############################################################################

class Admin(Course):
    """ host newlog task + logsvr start/stop/check task
        -- server logs can be renewed with a ecflowview menu command also
    """

    def __init__(self):
        self.name = "admin"

    def top(self, node):
        with node.family("admin") as node:
            node.add(self.main()).repeat(name="YMD", start=today, end=DATE_STOP)
            node.defstatus("suspended")
            return node

    def main(self): 
        """ return self contained Family/Task, without absolute node references
        or with relative path triggers"""
        remote_submit = "rsh -l %USER% %HOST% %ECF_JOB% > %ECF_JOBOUT% 2>&1"
        logpath = "/home/ma/map/course/201303/ecflow"
        return (
            Task("newlog").add(
                Label("info", "renew server log-file"),
                Time("08:00")),
            
            Task("logsvr").add(
                Defcomplete(),
                Variables(HOST= "pikachu",
                          ECF_LOGPORT=9316,
                          ECF_LOGPATH= logpath,
                          ECF_LOGMAP= logpath + ":" + logpath,
                          ECF_JOB_CMD= remote_submit),                          
                Label("info", "(re)start the logsvr on HOST"),
                Time("08:00")),

            Family("loop").add(
                Time("08:30"),
                Family("dummy").add(# TriggerImpossible(),
                    Complete("1==1"))))

###############################################################################

class EcEvents(Admin):
    """ connecting to third party software as event generator to update
    a suite variable, and enable daily family run
    """

    def top(self, node):
        node = node.family("ecevents")
        node.add(
            Label("info", "use web... menu"),
            Defcomplete(),
            Variables(
                URL= "http://eccmd.ecmwf.int:8090/#Mainpanel",
                ECF_URL_CMD= "${BROWSER:=firefox} -remote 'openURL(%URL%"))
        self.main(node)
        return node
    
    def main(self, node):        
        for mode in ["list", 
                     "register", "delete", 
                     "register_all", "delete_all"]:
            added = None
            if "list" in mode:
                added = Label("regs", "")

            fam = Family(mode).add(
                    Variables(MODE= mode),
                    Task("ecjobs").add(Label("info", "none"), 
                                       added))
            node.add(fam)
            if "_all" in mode:
                fam.add(Variables(EVENT= "_all_"))
            elif mode in ("register", "delete"):
                fam.add(Variables(EVENT= "an12h000"),
                         Label("info", "update EVENT variable"))

        event = "an00h000"
        node.family("ref").defstatus("complete").add(
            Task(event).add(Variables(YMD= today),
                            Label("YMD", today)))

        node.family("user").family(event).repeat(
            name="YMD", start=today, end=DATE_STOP).add(
            Label("info", "extern cannot be used anymore for " +
                  "intra-suite reference triggers"),
            Variables(SLEEP= 1),
            Trigger(event + ":YMD le %s/ref/%s:YMD" % (node.fullname(), event)),
            _leaf("consume"))
        return node

###############################################################################

class SerialTask(object):
    """ add trigger on the previous task """

    def __init__(self):
        self.prev = None

    def add(self, name):
        fam = Family(name).add(
            Variables(MODE= name),
            Task("to_ecflow"))
        if self.prev != None:
            fam.add(Trigger("./%s eq complete" % self.prev))
        self.prev = name
        return fam

class Reload(Admin):
    """ a simple task to download SMS content and translate it to ecFlow """

    def top(self, node):
        node = node.family("reload")
        node.add(
            Label("info", "from sms to ecFlow"),
            Defcomplete(),
            Variables(
                URL= "https://software.ecmwf.int/wiki/display/ECFLOW/Home",
                ECF_URL_CMD= "${BROWSER:=firefox} -remote 'openURL(%URL%"),
            self.main())
        return node
    
    def main(self, node=None):        
        fam = Family("reload")
        serial = SerialTask()
        fam.add(
            serial.add("get").add(
                Variables(
                    SMS_SUITE= "eras",
                    # SMS_PROG= 314159, SMS_NODE= "localhost",
                    # SMS_PROG= 314199, SMS_NODE= "marhaus", # eras
                    SMS_PROG= 314197, SMS_NODE= "marhaus", # eras2
                    ),),
            serial.add("translate"),
            serial.add("edit"),
            serial.add("mail"), 
            serial.add("load"),
            serial.add("bench").add(Defcomplete()))
        if node is not None: 
            return node.add(fam)
        return fam    

############################################################################

if __name__ == '__main__':
    import cli_proc, sys
    parser = argparse.ArgumentParser(
        description=DESC,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--host", default="localhost",
                        help= "server hostname")
    parser.add_argument("--port", default="3141",
                        help= "server port number")

    if len(sys.argv) > 1:
        suites = { "course": Course(),        } 
        argv = cli_proc.process(suites, compare=False)
        sys.exit(0)
    args = parser.parse_args()

    clt = ec.Client(args.host, args.port)
    try:
        clt.ping()
    except:
        clt = ec.Client("localhost", 1000 + os.geteuid())
    try:
        clt.ping()
    except:
        clt = ec.Client("localhost", 31415)
    try:
        clt.ping()
    except:
        clt = ec.Client("localhost", 3199)
    defs = ec.ecflow.Defs()

    course = Course()
    suite = course.suite().add(Reload().main())

    if 1: # enable/disable remote version of 'consume'
        rem = suite.family("remote").add(
            Task("mk_remote").add(
                Defcomplete(),
                Label("info", "do not forget to create directory structure" +
                      "on remote host for job output creation"),
                ie.onws(host = "class01")),
            Variables(ECF_HOME= "/tmp/%s/ecflow" % user()),
            ie.onws(host = "class02"))
        course.top(rem)

    # print clt.stats()

    Admin().top(suite)

    EcEvents().top(suite)

    defs.add_suite(suite)
    
    path = "/%s" % user()
    # path = "/%s/course" % user()
    # path = "/%s/admin" % user()
    # path = "/%s/ecevents" % user()
    # print defs
    
    clt.replace(path, defs)

    sys.exit (0) 
