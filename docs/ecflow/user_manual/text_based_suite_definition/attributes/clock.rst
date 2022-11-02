.. _clock:

clock
/////

Defines a **clock type** to be used by the suite, and specifies the
clock gain factor. Only suites can have a clock. A clock always runs in
phase with the system clock (UTC in UNIX) but can have an offset from the system clock.
ecFlow generates variables from the current time.

The clock must be either **hybrid** or **real** . Under a hybrid clock,
the date never changes unless specifically altered or unless the suite
restarts, either automatically or from a **begin** command. Under a real clock, the date
advances by one day at midnight. Time and date dependencies work a
little differently under the two clocks. The default clock type is **real** .

Clock gain is expressed in seconds and can be given as an integer, a
time or a date. Seconds and time can have a sign:

.. code-block:: shell
                                                  
    clock real 300              # the clock gains 300 sec from now
    clock real +01:00           # the clock gains 3600 sec from now
    clock real 01:00            # clock is 01:00 in the morning
    clock real 20.1.2012        # many days late but H:M is ok
    clock real 20.1.2012 +01:00 # many days late, time gains a hour

The clock can only be modified using alter command, e.g.

.. code-block:: shell

    ecflow_client --alter=change clock_type real /suite                
