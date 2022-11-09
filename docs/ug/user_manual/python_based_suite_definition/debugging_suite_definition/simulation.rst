.. _simulation:

Simulation
//////////

The suite definition describes the static structure, it's not until the
definition is loaded in the server, that we see its dynamic behaviour.

With ecFlow python API, the dynamic behaviour of the suite can be
simulated, ( i.e. in the same manner as the server).

Simulation has the following benefits:

- Exercise the suite definition. There is no need for '.ecf' files
- Allows for very easy experimentation.
- Can be done on the client-side, no need for server
- Can help in detecting deadlock's
- Will simulate with both 'real' and 'hybrid' clocks
- A year's simulation can be done in a few seconds
- Can be added as a unit test, to prevent regressions

The simulation relies on you adding simple verification attributes.
(This is similar to c/c++/python asserts). These can be added to a task,
family, and suite nodes. (see below for an example)

There are however restrictions. If the definition has large loops due
to, crons or Repeat attributes, which run indefinitely, then in this
case the simulation will never complete, and will timeout after a year's
worth of run time.

This can be compensated for by adding **start** and **end** clock. If no
start/end clock is specified, the simulator makes the following
assumption about the simulation period.

- No time dependencies - simulate for 24 hours.
- day attributes - simulate for 1 week
- date attributes - simulate for 1 month
- cron attributes - simulate for 1 year
- repeat attributes - simulate for 1 year

Additionally if time base attributes like, time, today,cron has no
minutes, then the simulator will use 1-hour resolution.

Here is an example of a text-based suite definition that uses a verify
attribute, for which we want to check our assumption about the dynamic
behaviour.

.. code-block:: shell
   :caption: cron.def - Run a task every day at 10am for 1 year

   suite year            # use real clock otherwise the date wont change
      clock real 1.1.2017  # define a start date for deterministic simulation
      endclock   1.1.2018  # When to finish. A endclock is *ONLY* for use with the simulator.
      family cronFamily
         task t
            cron -d 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 -m 1,2,3,4,5,6,7,8,9,10,11,12 10:00 # run every day at 10am for a year
            verify complete:365 # verify that this task completes 365 times
      endfamily
   endsuite
      
   suite leap_year       # use real clock otherwise the date wont change
      clock real 1.1.2016  # define a start date for deterministic simulation
      endclock   1.1.2017  # When to finish. A endclock is *ONLY* for use with the simulator.
      family cronFamily
         task t
            cron -d 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 -m 1,2,3,4,5,6,7,8,9,10,11,12 10:00  #  run every day at 10am for a year
            verify complete:366 # verify that this task completes 366 times in a leap year
      endfamily
   endsuite
  
This python segment shows how to load a text-based suite
definition(cron.def) and simulate it in python.

.. code-block:: python
   :caption: How to simulate a text based definitio

   import ecflow

   defs = Defs("cron.def")
   result = defs.simulate()
   assert len(result) == 0, (
      "Expected simulation to return without any errors, but found:\n" + result
   )

If the simulation does not complete it will produce two files, which
will help in the analysis:

- defs.depth: This file shows a depth-first view, of why simulation did not complete. defs.flat: This shows a simple flat view, of why simulation did not complete

- Both files will show which nodes are holding, and include the state of the holding trigger expressions.

Deadlock
============

This simulation is expected to fail since we have a deadlock/race condition:

.. code-block:: shell

   suite dead_lock
      family family
         task t1
            trigger t2 == complete
         task t2
            trigger t1 == complete
      endfamily
   endsuite

.. code-block:: python
   :caption: simulate a deadlock. Create definition in python


   from ecflow import Defs, Suite, Family, Task, Trigger

   defs = Defs(
      Suite(
         "dead_lock",
         Family(
               "family",
               Task("t1", Trigger("t2 == complete")),
               Task("t2", Trigger("t1 == complete")),
         ),
      )
   )

   theResult = defs.simulate() # simulate the definition
   assert len(theResult) != 0, "Expected simulation to return errors"
   print(theResult)
