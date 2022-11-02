.. _cron-last:

cron-last
/////////

Cron has been extended to support ' **last day of the month** ' and
**last week day of the month in ecflow 5**.

Lets recap the structure of a cron attribute:

.. code-block:: shell

    cron -w <weekdays> -d <days> -m <months> [ time || <start_time> <end_time> <increment> ]
    # weekdays:   range [0...6], Sunday=0, Monday=1, etc    e.g. -w, 0,3,6
    # days:       range [1..31]                             e.g. -d 1,2,20,30    if the month does not have a day, i.e. February 31st it is ignored
    # months:     range [1..12]                             e.g. -m 5,6,7,8
    # time:       The starting time. format hh:mm           e.g. 15:21
    # start_time: The starting time. format hh:mm           e.g. 15:21
    # end_time:   The end time, if multiple times used
    # increment:  The increment in time if multiple times are given


Remember we \*AND\* across -w, -d, -m then \*OR\* for each element in
-w, -d,-m

The new cron format will use L, to represent Last day of the month, or
last week day of the month.

Hence we now support:

* **-w** day of the week. valid values are , 0 > 6 where 0 is Sunday , 1 is Monday etc **AND** 0L > 6L, where 0L means last Sunday of the month, and 1L means the last Monday of the month, etc. It is an **error** to overlay, i.e. **cron -w 0,1,2,1L,2L,3L 23:00** will throw an exception.                                               
* **-d** day of the month, valid values are in range 0-31,L Extended so that we now use 'L' to mean the last day of the month.                                                                               
* **-m** month valid values are in range 0-12                            

.. code-block:: shell

    cron -w 5L 23:00                   # run on *last* Friday(5L) of each month at 23pm,
                                    # Python: cron = Cron("23:00",last_week_days_of_the_month=[5])
    cron -w 0,1L 23:00                 # run every Sunday(0) and *last* Monday(1L) of the month at 23pm
                                    # Python: cron = Cron("23:00",days_of_week=[0],last_week_days_of_the_month=[1])
    cron -w 0L,1L,2L,3L,4L,5L,6L 10:00 # run on the last Monday,Tuesday..Saturday,Sunday of the month at 10 am
                                    # Python: cron = Cron("10:00",last_week_days_of_the_month=[0,1,2,3,4,5,6])
    cron -d 1,L  23:00                 # Run on the first and last of the month at 23pm
                                    # Python: cron = Cron("23:00",days_of_week=[1],last_day_of_the_month=True)

The cron Late attribute can also be combined: Remember we \*AND\* across
-w, -d, -m and \*OR\* for each element in -w, -d,-m .

.. code-block:: shell

    cron -w 4L -d L 10:00             # run on the last Thursday(4L) of the month *AND* Last of day of the month, this only occurs 3 times in a year in 2019
                                    # Python: cron = Cron("10:00",last_week_days_of_the_month=[4],last_day_of_the_month=True)
    cron -w 0,1L -m 1 10:00           # run every Sunday(0) and last Monday(1L) of the month, in January at 10am
                                    # Python: cron = Cron("10:00",days_of_week=[0],last_week_days_of_the_month=[1],months=[1])

