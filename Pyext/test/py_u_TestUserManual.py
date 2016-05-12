#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2016 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

# This code is used in the user manual 
#

from ecflow import Suite, Task, RepeatDate
      
if __name__ == "__main__":
 
    class ExperimentalSuite(object):
        def __init__(self, start, end) :
            self.start_ = start
            self.end_ = end
            self.start_cycle_ = 12
            self.end_cycle_ = 12
            
        def generate(self) :
            x_suite = Suite("x")
            make_fam = x_suite.add_family("make")
            make_fam.add_task("build")
            make_fam.add_task("more_work")
            
            main_fam = x_suite.add_family("main")
            main_fam.add_repeat( RepeatDate("YMD",self.start_,self.end_) )
            main_fam.add_trigger( "make == complete" )
           
            previous = 0
            for FAM in ( 0, 6, 12, 18 ) :
                fam_fam = x_suite.add_family(str(FAM))
                if FAM > 0 :
                    fam_fam.add_trigger( "./" + str(previous) + " == complete " )
                   
                self.add_complete(fam_fam,FAM)
                
                fam_fam.add_task("run")
                fam_fam.add_task("run_more").add_trigger( "run == complete")
                previous = FAM
            return x_suite
           
        def add_complete(self, family, fam):
            if fam < self.start_cycle_ and fam > self.end_cycle_ :
                family.add_complete("../main:YMD eq " + str(self.start_) + " or ../main:YMD ge " + str(self.end_))  
            elif fam < self.start_cycle_  :
                family.add_complete("../main:YMD eq " + str(self.start_))            
            elif fam > self.end_cycle_    :
                family.add_complete("../main:YMD ge " + str(self.end_))
            return
 
    print str( ExperimentalSuite(20050601,20050605).generate() )
   
# ==========================================================================
 
# Control structure and looping
    var = "aa"
    if var in ( "a", "aa", "aaa" ) :     print "it is a kind of a "
    elif var in ( "b", "bb", "bb" ) :    print "it is a kind of b "
    else :                               print "it is something else "
   
# ==========================================================================

    task = Task("task")
    the_time = 0
    if the_time == 0 :
        task.add_today(17, 30)
        task.add_variable("ANTIME", str(the_time))
    elif the_time == 6 :
        task.add_today(17, 30)
        task.add_variable("ANTIME", str(the_time))
    elif the_time == 12 :
        task.add_today(19, 15)
        task.add_variable("ANTIME", str(the_time))
    elif the_time == 18 :
        task.add_time(1, 30)
        task.add_variable("ANTIME", str(the_time))
    elif the_time == 24 :
        task.add_time(3, 0)
        task.add_variable("ANTIME", "0")
        task.add_variable("DELTA_DAY", "1")
        task.add_variable("EXPVER", "0002")
              
# ====================================================================================

    # Reuseable class for adding synoptic times
    class VarAdder(object):
        def __init__(self, node):
            self.node = node
          
        def add(self, time):
            {
               6:  lambda  self : self.add6(time),
               12: lambda  self : self.add12(time),
               18: lambda  self : self.add18(time),
               24: lambda  self : self.add24(time) 
             }.get(time, self.errorHandler)(self)
      
        def add6(self, time):  
            print "add6 " + str(time)
            self.node.add_today(17, 30)
            self.node.add_variable("ANTIME", str(time))

        def add12(self, time):  
            print "add12 " + str(time)
            self.node.add_today(19, 15)
            self.node.add_variable("ANTIME", str(time))

        def add18(self, time):  
            print "add18 " + str(time)
            self.node.add_time(1, 30)
            self.node.add_variable("ANTIME", str(time))

        def add24(self, time):  
            print "add24 " + str(time)
            self.node.add_time(3, 0)
            self.node.add_variable("ANTIME", "0")
            self.node.add_variable("DELTA_DAY", "1")
            self.node.add_variable("EXPVER", "0002")
         
        def errorHandler(self, ignore): print "invalid time " + str(ignore)

#for i in (0, 6 ,12, 18, 24):
for i in (0, 6):
    task = Task("t" + str(i))
    print task.name()
    varAdder = VarAdder(task)
    varAdder.add(114)
    for var in task.variables: 
        print str(var) + "\n"
    for the_time in task.times : 
        print str(the_time) + "\n"
    for today in task.todays : 
        print str(today) + "\n"
         
# ==========================================================================

suite = Suite("x")
previous_time = 0
for i in (0,6,12,18,24) :
    the_fam = suite.add_family(str(i))
    if i != 0:
        the_fam.add_trigger("./" + previous_time + " == complete ")
    the_fam.add_task("t1")
    the_fam.add_task("t2").add_trigger("t1 == complete")
    previous_time = str(i)

# print str(suite)
