from ecflow import *
  
print ("Simulator test ecflow_1639")

# the following test fails in ecflow 5.3.1
# Simulation start    at:: 1,5,2020 @ 00:00
# Simulation finished at:: 3,5,2020 @ 00:00
# ie Friday->Saturday(midnight) 2 days
# Hence we expect  task t4; cron 23:00             to complete 2
# Hence we expect  task t5; cron 10:00 20:00 01:00 to complete 22


defs = Defs(
        Suite("ecflow_1639",
            Clock(1,5,2020),
            Task("t2",Date("1.5.2010"),Date("1.5.2011"),Verify(State.complete,0)),
            Task("t3",Time("23:00"),Verify(State.complete,1)),
            Task("t4",Cron("23:00"),Verify(State.complete,2)),
            Task("t5",Cron("10:00 20:00 01:00"),Verify(State.complete,22)),
            )
        )
defs.ecflow_1639.add_end_clock(Clock(3,5,2020))
print(defs)

theResult = defs.simulate()
assert len(theResult) == 0,  "Expected simulation to return without any errors, but found:\n" + theResult

    