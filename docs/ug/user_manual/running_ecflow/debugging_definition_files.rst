.. _debugging_definition_files:

Debugging definition files
//////////////////////////

You can check definition for valid trigger expression and in-limits for
existing definition file.

.. code-block:: shell
    :caption: Command line 

    ecflow_client --load=/my/home/exotic.def check_only

.. code-block:: python
    :caption: Python, Load from disk and check

    from ecflow import Defs
    defs = Defs('/my/home/exotic.def')
    print(defs.check())


This will check that the suite definition is correct and can be loaded
into the server.

However, typically Definition files are built using the python API,
where most checks are done whilst the definition is being built. (i.e.
duplicate node names at the same level).

.. code-block:: python
    :caption: Check defs built with the ecFlow Python API
    
    import os
    from ecflow import Defs,Suite,Task,Edit
     
    home = os.path.join(os.getenv("HOME"),  "course")
    defs = Defs(
        Suite('test',
            Edit(ECF_HOME=home),
            Task('t1')))
    print(defs.check())
                                            

Simulation and Verification
===========================

You can also use the simulator, allowing you to predict/verify the
behaviour of your suite in a few seconds. The simulator is available
with the python API.

The simulator will analyse the definition, and simulate the ecFlow
server. Allowing time dependencies that span several months, to be
simulated in a few seconds.

ecFlow allows the use of 'verify' attributes. This example shows how we
can verify the number of times a task should run, given a
start(optional, but required when using time-based attributes) and end
time(optional).

.. code-block:: shell
    :caption: suite.def
    
    suite cron3               # use real clock otherwise clock starts when the simulations starts
        clock real  1.1.2006  # define a start date for deterministic behaviour
        endclock   13.1.2006  # When to finish. end clock is *only* used for the simulator
        family cronFamily
            task t
                cron -d 10,11,12   10:00 11:00 01:00  # run on 10,11,12 of the month at 10am and 11am
                verify complete:6                     # task should complete 6 times between 1.1.2006 -> 13.1.2006
        endfamily
    endsuite                                             

Please note, for deterministic behaviour, the start and end clock
should be specified when using time-based attributes. However, if no 'endclock' is specified the simulation run will assume
the following defaults:

-  No time dependencies: 24 hours

-  time || today: 24 hours

-  day: 1 week

-  date: 1 month

-  cron: 1 year

-  repeat: 1 year

The simulator resolution is determined by analysing the suite. If we find time dependencies with a minute resolution, then this is used. (Otherwise, we use 1-hour resolution). If there no time
dependencies then the simulator will by default use **1-hour
resolution**.

This needs to be taken into account when specifying the verify attribute. If the simulation does not complete it creates defs.flat and defs.depth files. This provides clues as to the state of the definition at the end of the
simulation. 

    .. code-block:: python
        :caption: Calling the simulator in Python

        import ecflow
        defs = Defs('suite.def')        # specify the defs we want to simulate
        theResults = defs.simulate()    # call the simulator
        print theResults                # check the results.

If the definition was created directly with the python API, then we
need only call 'defs.simulate()
