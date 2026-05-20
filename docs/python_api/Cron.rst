ecflow.Cron
///////////


.. py:class:: Cron
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

A :term:`cron` defines a repeating time dependency for a node.


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

Usage (see the :ref:`cron definition<text_based_def_cron>` for more examples):

.. code-block:: python

    # Run every day at 2pm
    cron = Cron('14:00')

    # Run every 30 minutes between 0:00 and 20:00, the first 6 days of the month from January until July
    cron = Cron('+00:00 20:00 00:30', days_of_month=[1,2,3,4,5,6], months=[1,2,3,4,5,6])

    # Run relative to suite start time or task requeue time
    cron = Cron('+01:30',days_of_week=[0,1,2,3,4,5,6])

    # Run relative to suite start time or task requeue time
    cron = Cron('+00:15 23:00 00:30', days_of_week=[0,1,2],days_of_month=[4,5,6], months=[1,2,3])

    # Define Cron based on start/end/increment
    start = TimeSlot(0 , 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    ts = TimeSeries(start, finish, incr, True)  # True means relative to suite start
    cron = Cron(ts, days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2])

    # Use Cron methods to set weekdays, days of month and month parameters
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6 ])
    cron.set_months([1, 2, 3, 4, 5, 6])
    cron.set_time_series(ts)

    # Use Cron methods to set time series
    cron = Cron()
    cron.set_time_series(1, 30, True)  # same as cron +01:30

    # Use Cron methods to set time series over all days of the week
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_time_series('00:30 01:30 00:01')

    # Use Cron methods to set relative time series over all days of the week
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_time_series('+00:30')


.. py:property:: Cron.days_of_month
   :module: ecflow

returns a integer list of days of the month


.. py:method:: Cron.last_day_of_the_month(self: ecflow.Cron) -> bool
   :module: ecflow

Return true if last day of month is enabled


.. py:property:: Cron.last_week_days_of_the_month
   :module: ecflow

returns a integer list of last week days of the month


.. py:property:: Cron.months
   :module: ecflow

returns a integer list of months of the year


.. py:method:: Cron.set_days_of_month(self: ecflow.Cron, arg0: list) -> None
   :module: ecflow

Specifies days of the month. Expects a list of integers with integer range 1-31


.. py:method:: Cron.set_last_day_of_the_month(self: ecflow.Cron) -> None
   :module: ecflow

Set cron for the last day of the month


.. py:method:: Cron.set_last_week_days_of_the_month(self: ecflow.Cron, arg0: list) -> None
   :module: ecflow

Specifies last week days of the month. Expects a list of integers, with integer range 0==Sun to 6==Sat


.. py:method:: Cron.set_months(self: ecflow.Cron, arg0: list) -> None
   :module: ecflow

Specifies months. Expects a list of integers, with integer range 1-12


.. py:method:: Cron.set_time_series(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. set_time_series(self: ecflow.Cron, hour: typing.SupportsInt | typing.SupportsIndex, minute: typing.SupportsInt | typing.SupportsIndex, relative: bool = False) -> None

time_series(hour(int),minute(int),relative to suite start(bool=false)), Add a time slot

2. set_time_series(self: ecflow.Cron, arg0: ecflow.TimeSeries) -> None

Add a time series. This will never complete

3. set_time_series(self: ecflow.Cron, arg0: ecflow.TimeSlot, arg1: ecflow.TimeSlot, arg2: ecflow.TimeSlot) -> None

Add a time series. This will never complete

4. set_time_series(self: ecflow.Cron, arg0: str) -> None

Add a time series. This will never complete


.. py:method:: Cron.set_week_days(self: ecflow.Cron, arg0: list) -> None
   :module: ecflow

Specifies days of week. Expects a list of integers, with integer range 0==Sun to 6==Sat


.. py:method:: Cron.time(self: ecflow.Cron) -> ecflow.TimeSeries
   :module: ecflow

return cron time as a TimeSeries


.. py:property:: Cron.week_days
   :module: ecflow

returns a integer list of week days

