.. _using_dependencies_together:

Using dependencies together
///////////////////////////

The way a combination of different dependencies work together is not
always clear. In order for a node to be scheduled:

- **Parent**: must be free in order for its children to run. A family must be free before any of its tasks can run.
- **Date**: must be free. This is checked when you begin a suite begin(CLI) and at midnight for the suite. There is no midnight for a hybrid clock.
- **Time**: must be free. This is checked every minute (configurable.)
- **Trigger**: must be free. This is checked every time there is a state change in the suite for all potential nodes.
- **Limit**: must not be full. Boolean limits just have slots in them, while integer limits must have enough space for the usage.

Here are a few examples of combinations with their behaviour.

To run task only once, on 17th of February 2012 at 10 am:

.. code-block:: shell

    task x
        time 10:00
        date 17.2.2012


To run task twice, at 10am and 8pm, on both 17th and 19th of February
2012, that is, four times in all. Notice the task is queued in between,
and completes only after the last run. Under a hybrid clock, the task is
run (twice) only if the date is either 17th or 19th at the time the
suite begins:

.. code-block:: shell

    task x
        time 10:00     ; time 20:00
        date 17.2.2012 ; date 19.2.2012
    
To run task after task is complete and if the day is Monday. If the
suite is using a real clock, the task waits for the following Monday
(unless today is Monday) and for task to be complete. Under a hybrid
clock, if today is not Monday when the suite begins or is
auto-restarted, task is marked complete without being submitted:

.. code-block:: shell

    task y
    task x
        trigger ./y == complete
        day monday
    

The next example shows how to run a task after an earlier task has
stopped, either by completing or aborting. It may be useful to continue
after a task has tried a few times but still failed. However, this
technique should only be used if there is logic somewhere to correct for
the missing task. Otherwise, task **y** will fail as well.

.. code-block:: shell

    task x
    task y
        trigger ./x == complete or ./x == aborted
    #
    #The above trigger using named operators:
    #
    # trigger x eq complete or x eq aborted
    #
    task z
        trigger (x==complete or x==aborted) and (y==complete or y==aborted)

To run a task on a series of given days, use the Python API:

.. code-block:: python

    t = ecflow.Task("x")
    for i in [ 1 , 2, 4, 8, 16 ] :
        t.add_date(i,0,0) # 0 means any day,month or year

Will be displayed by the show(CLI) command as:

.. code-block:: shell

    task x
        date 1.*.*
        date 2.*.*
        date 4.*.*

To run a task half after the previous task fc has done half of its work:

.. code-block:: shell

    task fc
        meter hour 0 240
    task half
        trigger fc:hour >= 120
        # trigger fc:hour ge 120
    

There is no guarantee that a task will be sent at the exact moment
requested. At the specified time, ecFlow might be busy processing other
tasks. ecFlow does not check time dependencies constantly, but sweeps through them
once a minute. This makes processing of events in ecFlow much more
stable.
