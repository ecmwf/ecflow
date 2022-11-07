.. _text_based_def_cron:

cron
////

Like **time**, **cron** defines time dependency for a node, but it ignores the suites **clock** and allows the node to be **repeated** indefinitely. This means that nodes, and thus suites with **cron** will never complete.

.. code-block:: shell

    cron 23:00                              # run every day at 23:00
    cron 10:00 20:00 01:00                  # run every hour between 10am and 8pm
    cron +10:00 20:00 01:00                 # relative cron, run at 10 hours after suite start(or when re-queued), up to 20 hours, in increments of 1 hour
    cron -w 0,1 10:00                       # run every Sunday and Monday at 10am
    cron -d 10,11,12 12:00                  # run 10th, 11th and 12th of each month ~ # at noon
    cron -m 1,2,3 12:00                     # run every day in January, February and March at 12 noon
    cron -w 0 -m 5,6,7,8 10:00 20:00 01:00  # run every Sunday, in May,June,July,August from 10am to 8pm, every hour

every day at noon :

.. code-block:: shell

    task x
        cron -w 1,2,3,4,5 10:00
        cron -w 0,6 12:00         # two crons at the same level should be avoided
    task x
        cron -w


When the node becomes complete it will be queued immediately. This means
that the suite will never complete, and output is not directly
accessible through GUI.

If tasks abort, ecFlow will not schedule it again. Also if the time the
job takes to complete is longer than the interval a "slot" is missed,
e.g. **cron 10:00 20:00 01:00** if the **10:00** run takes more than an hour the **11:00**
run will never occur.

With cron you can also specify weekdays, day of the month and month of
the year masks

.. code-block:: shell

    cron -w <weekdays> -d <days> -m <months> <start_time> <end_time> <increment>
    # weekdays:   range [0...6], Sunday=0, Monday=1, etc       e.g. -w, 0,3,6
    # days:       range [1..31]                                e.g. -d 1,2,20,30    if the month does not have a day, i.e. February 21st it is ignored
    # months:     range [1..12]                                e.g. -m 5,6,7,8
    # start_time: The starting time(compulsory) format hh:mm  e.g. 15:21
    # end_time:   The end time, if multiple times used (optional)
    # increment:  The increment in time if multiple times are given(optional, however if end_time is specified then the increment must be specified



.. toctree::
   :maxdepth: 1
   
   cron-last
