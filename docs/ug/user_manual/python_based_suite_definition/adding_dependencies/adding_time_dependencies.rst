.. _adding_time_dependencies:

Adding Time Dependencies
////////////////////////

The following suite definition is **NOT** a real design, it just demonstrates all the variety of way of adding time dependencies.

.. code-block:: python

   from ecflow import Defs, Suite, Task, Day, Date, Cron, Time, TimeSlot, TimeSeries

   start = TimeSlot(0, 0)
   finish = TimeSlot(23, 0)
   incr = TimeSlot(0, 30)
   time_series = TimeSeries(start, finish, incr, True)

   defs = Defs(
      Suite(
         "s1",
         Task(
               "date",
               Date(1, 0, 0),  # first of every month and every year
               Date("2.*.*"),  # second of every month and every yea
               Date(28, 2, 2026),
         ),  # 28 February 2026
         Task("day", Day("monday"), Day(Days.tuesday)),
         Task(
               "time",
               Time("+00:30"),  # 30 minutes after suite has begun
               Time("+00:30 20:00 01:00"),  # 00:30,01:30,02:30....19:30 after suite start
               Time(0, 59, True),  # 00:59 - 59 minutes past midnight
               Time(TimeSlot(20, 10)),  # 20:10 - 10 minutes past eight
               Time(
                  TimeSlot(20, 20), True
               ),  # +20:20 - 20 minutes and 20 hours, after suite start
               Time(time_series),
               Time(0, 10),
               Time("+00:40"),
               Time("+00:40 20:00 01:00"),
         ),
         Task(
               "cron",
               Cron(
                  "+00:00 23:00 00:30",
                  days_of_week=[0, 1, 2, 3, 4, 5, 6],
                  days_of_month=[1, 2, 3, 4, 5, 6],
                  months=[1, 2, 3, 4, 5, 6],
               ),
         ),
      )
   )


The following shows an alternative example that produces the same definition:

.. code-block:: python

   start = TimeSlot(0, 0)
   finish = TimeSlot(23, 0)
   incr = TimeSlot(0, 30)
   time_series = TimeSeries(start, finish, incr, True)

   cron = Cron()
   cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
   cron.set_days_of_month([1, 2, 3, 4, 5, 6])
   cron.set_months([1, 2, 3, 4, 5, 6])
   cron.set_time_series("+00:00 23:00 00:30")

   defs = Defs().add(
      Suite("s1").add(
         Task("date").add(Date(1, 0, 0), Date("2.*.*"), Date(28, 2, 2026)),
         Task("day").add(Day("monday"), Day(Days.tuesday)),
         Task("time").add(
               Time("+00:30"),
               Time("+00:30 20:00 01:00"),
               Time(0, 59, True),
               Time(TimeSlot(20, 10)),
               Time(TimeSlot(20, 20), True),
               Time(time_series),
               Time(0, 10),
               Time("+00:40"),
               Time("+00:40 20:00 01:00"),
         ),
         Task("cron").add(cron),
      )
   )

.. code-block:: python

   make_list_table(tables_raw[0])

   start = TimeSlot(0, 0)
   finish = TimeSlot(23, 0)
   incr = TimeSlot(0, 30)
   time_series = TimeSeries(start, finish, incr, True)

   defs = Defs() + (Suite("s1") + Task("date") + Task("day") + Task("time") + Task("cron"))
   defs.s1.date += [Date(1, 0, 0), Date("2.*.*"), Date(28, 2, 2026)]
   defs.s1.day += [Day("monday"), Day(Days.tuesday)]
   defs.s1.time += [
      Time("+00:30"),
      Time("+00:30 20:00 01:00"),
      Time(0, 59, True),
      Time(TimeSlot(20, 10)),
      Time(TimeSlot(20, 20), True),
      Time(time_series),
      Time(0, 10),
      Time("+00:40"),
      Time("+00:40 20:00 01:00"),
   ]
   defs.s1.cron += Cron(
      "+00:00 23:00 00:30",
      days_of_week=[0, 1, 2, 3, 4, 5, 6],
      days_of_month=[1, 2, 3, 4, 5, 6],
      months=[1, 2, 3, 4, 5, 6],
   )

.. warning::

   In the example above we use 'defs.s1.date' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs IF the node names are changed.
