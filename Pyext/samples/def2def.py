#!/usr/bin/env python
# This software is provided under the ECMWF standard software license agreement.
import sys
try: import ecflow
except:
        sys.path.append("/usr/local/apps/ecflow/current/lib/python2.7/site-packages/ecflow")
        import ecflow 
import ecflow  as ec
import sys
""" a simple program to convert an expanded definition file to 
py script using ecf.py"""

class Indent:
    """This class manages indentation, for use with context manager
    It is used to correctly indent the definition node tree hierarchy
    """
    _pos = 0
    _step = 3
    def __init__(self):
        Indent._pos += Indent._step
    def __del__(self):
        Indent._pos -= Indent._step
    @classmethod
    def indent(cls, loc=''):
        for i in range(Indent._pos):
            if loc is None: 
                print ' ' 
            elif type(loc) == str: 
                loc += ' '
            else: 
                loc.write(' ')
        if type(loc) == str: 
            return loc

def adds(line=0):
    if line is None: 
        return ""

    return Indent.indent() + line + "\n"

def add(line, echo=0):
    if line is None: 
        return
    elif echo: 
        print Indent.indent() + line 
    else: 
        return Indent.indent() + line + "\n"

class DefFormat(object):
    def __init__(self, defs): 
        self.defs = defs

    def process_attr(self, node, end=False):
        res = ""
        ind = Indent()
        defstatus = node.get_defstatus()
        if defstatus: 
            if defstatus != ec.DState.queued:
                res += add("Defstatus('%s')," % defstatus)

        item = node.get_autocancel()
        if item: 
            line = "%s" % item
            line = line.replace("autocancel ", "")
            res += add("Autocancel('%s')," % line)

        item = node.get_repeat()
        if not item.empty(): 
            line = "%s" % item
            full = line.split()
            kind = full[1]
            name = full[2]
            try:
                beg  = full[3]
                try: end  = full[4]
                except: end = beg
                if "#" in end: end = beg
            except: beg = 1; end = 1
            by = 1
            if len(full) == 6: by = full[5]

            if kind in ("integer", "date"):
                res += add(
                    "Repeat(kind='%s', name='%s', start=%s, end=%s, step=%s)," 
                    % ( kind, name, beg, end, by))
            elif kind in ("day", ):
                res += add(
                    "Repeat(kind='%s', name='%s', step=%s)," 
                    % ( kind, name, by))
            elif kind in ("string", "enumerated"):
                line = "%s" % item
                line = line.replace("repeat %s %s" % (kind, name), "")
                line.replace('"', '')
                res += add("Repeat(kind='%s', name='%s', start=%s)," % (
                        kind, name, line.split()))
             # FIXME string enum

        item = node.get_late()
        if item: 
            line = "%s" % item
            line = line.replace("late ", "")
            res +=  add("Late('%s')," % line)

        item = node.get_complete()
        if item: res += add("Complete('%s')," % item)

        item = node.get_trigger()
        if item: 
            res += add("Trigger('%s')," % item)

        for item in node.meters: 
            line = "%s" % item
            dummy, name, beg, end, thr = line.split(" ")
            res += add("Meter('%s', %s, %s, %s)," % (name, beg, end, thr))

        for item in node.events: 
            line = "%s" % item
            line = line.replace("event ", "").strip()
            res += add("Event('%s')," % line)

        for item in node.labels: 
            res += add("Label('%s', '%s')," % (item.name(), item.value()))

        for item in node.limits: 
            val = item.value()
            if val == "0" or val == 0: 
                val = "1"
            res += add("Limit('%s', %d)," % (
                    item.name(), int(val)))

        for item in node.inlimits:
            line = "%s" % item
            line = line.replace("inlimit ", "")
            res += add("InLimit('%s')," % line)

        for item in node.times:  
            line = "%s" % item
            line = line.replace("time ", "")
            res += add("Time('%s')," % line)  

        for item in node.todays: 
            line = "%s" % item
            line = line.replace("today ", "")
            res += add("Today('%s')," % line) 

        for item in node.dates:  
            line = "%s" % item
            line = line.replace("date ", "")            
            res += add("Date('%s')," % line)  

        for item in node.days:   
            line = "%s" % item
            line = line.replace("day ", "")            
            
            res += add("Day('%s')," % item)   
        for item in node.crons:  
            line = "%s" % item
            line = line.replace("cron ", "")            
            res += add("Cron('%s')," % line)  

        var = "Variables("
        vind = Indent()            
        num = 0
        for item in node.variables: 
            sep="'"
            if sep in item.value(): sep="\""
            var += ind.indent("\n") + item.name() + \
                "= %s%s%s," % (sep, item.value(), sep)
            num += 1
        del vind        
        if num: res += add(var + "),")

        del ind
        if len(res) == 0: 
            return None
        return res

    def process(self, node=None, inc=0):
        STREAM = 1
        if node is None:
            num = 0
            print "# ecf.ECF_MODE = 'sms'"
            print "suites = []"
            for suite in self.defs.suites:
                if STREAM: print "suite%d = Suite('%s').add(" % (num, suite.name())
                else: print "s = Suite('%s')"% (suite.name()) + "; suites.append(s);"
                self.process(suite)
                print ")"
                num += 1
            if num > 1: 
                if 0: raise BaseException("no more than one suite at a time")
                print "### WARNING: more than one suite defined"

        elif isinstance(node, ec.Suite): 
            if not STREAM: print "s.add(\n"
            add( self.process_attr(node),1 )
            if STREAM: ind = Indent()
            for kid in node.nodes: self.process(kid)
            if STREAM: del ind

        elif isinstance(node, ec.Family): 
            many = ""; one = ""; post = "" # circumvent limitation to 255 items
            if inc % 200 == 0: 
                one += "("; post = "),"
                if inc > 200: many = "),"
            if STREAM: 
                add( many + one + "Family('%s').add(" % node.name(), 1)
                res = self.process_attr(node)
            else:
                print ")\ncur = Family('%s')" % node.name()
                print "fam.add(cur); fam = cur; fam.add("
                res = self.process_attr(node)

            if STREAM: 
                if res is not None: 
                    print Indent.indent(res)
                ind = Indent()
            num = 0
            for kid in node.nodes: self.process(kid, num)            ; num += 1
            if STREAM: 
                del ind
                add( Indent.indent(post + "), # endfamily %s" % node.name()), 1 )
            print "# ", num
        elif isinstance(node, ec.Task): 
            res = self.process_attr(node)
            if res is None:  
                add( "Task('%s')," % node.name(), 1)
            else: add( "Task('%s').add(\n" % node.name() +
                       "%s%s)," % (res, Indent.indent()), 1)

    def main(self):
        self.header()
        self.process()
        self.footer()

    def header(self):
        print """#!/usr/bin/env python
# NOTICE: this file was originally created with def2def.py
import sys
# sys.path.append('../o/def')
# sys.path.append('.')
sys.path.append('/home/ma/emos/def/o/def')
from ecf import *
import ecf  as ecf
# import inc_emos as ie
"""

    def footer(self):
        print """
if __name__ == '__main__':
    defs = Defs()
    defs.add(suite0); 
    defs.auto_add_externs(True)
    if 0: 
      import cli_proc, ecf
      cli_proc.process(ie.Seed(defs), compare=False)
    else:
       port = 1500 + int(get_uid()) # when started with ecflow_start.sh # ECMWF

       if 0: # test job creation
         job_ctrl = ecflow.JobCreationCtrl()                    
         defs.check_job_creation(job_ctrl)

       print "loading on localhost@%s" % port
       client = ecf.Client("localhost", port)
       client.replace("/%s" % suite0.name(), defs)
"""
if __name__ == '__main__':
    filename = sys.argv[1]
    defs = ec.Defs(filename)
    defs.auto_add_externs(True)
    if 0: print defs
    DefFormat(defs).main()

