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

#  Test the simulation works

import os
import ecflow

def simulate_defs_with_time():
    #suite test_time_series
    #  family family
    #      task t1
    #         time 00:00 23:00 04:00  # should run 6 times 00:00 04:00 08:00 12:00 16:00 20:00
    #  endfamily
    #endsuite
    print("simulate_defs_with_time()")
    defs = ecflow.Defs()
    suite = defs.add_suite("test_time_series")
    clock = ecflow.Clock(1, 1, 2011, False)       # day,month, year, hybrid make test start at midnight, otherwise current time used
    suite.add_clock(clock)
    family = suite.add_family("family")
    task = family.add_task("t1")
    ts = ecflow.TimeSeries(ecflow.TimeSlot(0,0), ecflow.TimeSlot(23,0), ecflow.TimeSlot(4,0), True)
    task.add_time( ecflow.Time(ts) )
    task.add_verify( ecflow.Verify(ecflow.State.complete, 6) ) # expect task to complete 6 times
         
    theResult = defs.simulate()
    assert len(theResult) == 0,  "Expected simulation to return without any errors, but found:\n" + theResult
    
    print("  simple check for state change time")
    print("   iso_extended:",task.get_state_change_time())
    print("   iso_extended:",task.get_state_change_time("is0_extended"))
    print("   iso         :",task.get_state_change_time("iso"))
    print("   simple      :",task.get_state_change_time("simple"))
    print("   rubbish     :",task.get_state_change_time("rubbish"))
 
    os.remove("test_time_series.def.log")
    
def simulate_deadlock():
    # This simulation is expected to fail, since we have a deadlock/ race condition
    
    # suite dead_lock
    #  family family
    #    task t1
    #          trigger t2 == complete
    #    task t2
    #          trigger t1 == complete
    #   endfamily
    # endsuite
    
    defs = ecflow.Defs()
    suite = defs.add_suite("dead_lock")
    fam = suite.add_family("family")
    fam.add_task("t1").add_trigger("t2 == complete")
    fam.add_task("t2").add_trigger("t1 == complete")
    
    theResult = defs.simulate()
    assert len(theResult) != 0, "Expected simulation to return errors, but found none" 
    print(theResult)
           
    os.remove("dead_lock.def.log")
    os.remove("defs.depth")
    os.remove("defs.flat")

if __name__ == "__main__":
    print("####################################################################")
    print("Running ecflow version " + ecflow.Client().version() + " debug build(" + str(ecflow.debug_build()) +")")
    print("####################################################################")
 
    simulate_defs_with_time()
    simulate_deadlock()
    print("All Tests pass")
