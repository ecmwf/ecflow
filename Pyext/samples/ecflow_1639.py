from ecflow import *
  
print ("Simulator::")


defs = Defs(
        Suite("ecflow_1639",
            Clock(1,5,2020),
            Task("t1",Day("friday"),Verify(State.complete,1)),
            Task("t2",Date("1.5.2010"),Verify(State.complete,0)),
            Task("t3",Time("23:00"),Verify(State.complete,1)),
            Task("t4",Cron("23:00"),Verify(State.complete,1)),
            Task("t5",Cron("10:00 20:00 01:00"),Verify(State.complete,11)),
            )
        )
defs.ecflow_1639.add_end_clock(Clock(2,5,2020))
print(defs)

theResult = defs.simulate()
assert len(theResult) == 0,  "Expected simulation to return without any errors, but found:\n" + theResult

    