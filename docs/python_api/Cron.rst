ecflow.Cron
///////////


.. py:class:: Cron
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

:term:`cron` defines a repeating time dependency for a node.

Crons are repeated indefinitely.

Avoid having a cron and :term:`repeat` at the same level,as both provide looping functionality

Constructor::

   Cron()
   Cron(string time_series,
        days_of_week=list of ints,   # 0-6, Sunday-Saturday
        days_of_month=list of ints,  # 1-31
        months=list of ints)         # 1-12
   Cron(TimeSeries time_series,
        days_of_week=list of ints,
        days_of_month=list of ints,
        months=list of ints)

Exceptions:

- raises IndexError || RuntimeError when an invalid cron is specified

Usage::

    cron = Cron('+00:00 23:00 00:30', days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2,3,4,5,6])

    # Here '+' means relative to begin or re-queue time
    cron = Cron('+01:30',days_of_week=[0,1,2,3,4,5,6])

    cron = Cron('+00:00 23:00 00:30', days_of_week=[0,1,2],days_of_month=[4,5,6], months=[1,2,3])

    start = TimeSlot(0 , 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    ts = TimeSeries(start, finish, incr, True)  # True means relative to suite start
    cron = Cron(ts, days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2])

    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6 ])
    cron.set_months([1, 2, 3, 4, 5, 6])
    cron.set_time_series(ts)

    cron1 = Cron()
    cron1.set_time_series(1, 30, True)  # same as cron +01:30

    cron2 = Cron()
    cron2.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron2.set_time_series('00:30 01:30 00:01')

    cron3 = Cron()
    cron3.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron3.set_time_series('+00:30')


.. py:property:: Cron.days_of_month
   :module: ecflow

returns a integer list of days of the month


.. py:method:: Cron.last_day_of_the_month( (Cron)arg1) -> bool :
   :module: ecflow

Return true if last day of month is enabled


.. py:property:: Cron.last_week_days_of_the_month
   :module: ecflow

returns a integer list of last week days of the month


.. py:property:: Cron.months
   :module: ecflow

returns a integer list of months of the year


.. py:method:: Cron.set_days_of_month( (Cron)arg1, (list)arg2) -> None :
   :module: ecflow

Specifies days of the month. Expects a list of integers with integer range 1-31


.. py:method:: Cron.set_last_day_of_the_month( (Cron)arg1) -> None :
   :module: ecflow

Set cron for the last day of the month


.. py:method:: Cron.set_last_week_days_of_the_month( (Cron)arg1, (list)arg2) -> None :
   :module: ecflow

Specifies last week days of the month. Expects a list of integers, with integer range 0==Sun to 6==Sat


.. py:method:: Cron.set_months( (Cron)arg1, (list)arg2) -> None :
   :module: ecflow

Specifies months. Expects a list of integers, with integer range 1-12


.. py:method:: Cron.set_time_series( (Cron)arg1, (int)hour, (int)minute [, (bool)relative=False]) -> None :
   :module: ecflow

time_series(hour(int),minute(int),relative to suite start(bool=false)), Add a time slot

set_time_series( (Cron)arg1, (TimeSeries)arg2) -> None :
    Add a time series. This will never complete

set_time_series( (Cron)arg1, (TimeSlot)arg2, (TimeSlot)arg3, (TimeSlot)arg4) -> None :
    Add a time series. This will never complete

set_time_series( (Cron)arg1, (str)arg2) -> None :
    Add a time series. This will never complete


.. py:method:: Cron.set_week_days( (Cron)arg1, (list)arg2) -> None :
   :module: ecflow

Specifies days of week. Expects a list of integers, with integer range 0==Sun to 6==Sat


.. py:method:: Cron.time( (Cron)arg1) -> TimeSeries :
   :module: ecflow

return cron time as a TimeSeries


.. py:property:: Cron.week_days
   :module: ecflow

returns a integer list of week days

