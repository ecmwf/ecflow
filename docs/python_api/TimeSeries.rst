ecflow.TimeSeries
/////////////////


.. py:class:: TimeSeries
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

A TimeSeries can hold a single time slot or a series.

Time series can be created relative to the :term:`suite` start or start of a repeating node.
A Time series can be used as argument to the :py:class:`ecflow.Time`, :py:class:`ecflow.Today` and :py:class:`ecflow.Cron` attributes of a node.
If a time the job takes to complete is longer than the interval, a 'slot' is missed
e.g time 10:00 20:00 01:00, if the 10.00 run takes more than an hour the 11.00 is missed


Constructor::

   TimeSeries(single,relative_to_suite_start)
      TimeSlot single :  A single point in a 24 clock 
      optional bool relative_to_suite_start : depend on suite begin time or
                                              start of repeating node. Default is false

   TimeSeries(hour,minute,relative_to_suite_start)
      int hour   :  hour in 24 clock 
      int minute :  minute < 59 
      bool relative_to_suite_start<optional> : depend on suite begin time or
                                              start of repeating node. Default is false

   TimeSeries(start,finish,increment,relative_to_suite_start)
      start TimeSlot :     The start time  
      finish TimeSlot :    The finish time, when used in a series. This must greater than the start.
      increment TimeSlot : The increment. This must be less that difference between start and finish
      bool relative_to_suite_start<optional> : The time is relative suite start, or start of repeating node.
                                               The default is false

Exceptions:

- Raises IndexError when an invalid time series is specified

Usage::

   time_series = TimeSeries(TimeSlot(10,11),False)


.. py:method:: TimeSeries.finish( (TimeSeries)arg1) -> TimeSlot :
   :module: ecflow

returns the finish time if time series specified, else returns a NULL time slot


.. py:method:: TimeSeries.has_increment( (TimeSeries)arg1) -> bool :
   :module: ecflow

distinguish between a single time slot and a series. returns true for a series


.. py:method:: TimeSeries.incr( (TimeSeries)arg1) -> TimeSlot :
   :module: ecflow

returns the increment time if time series specified, else returns a NULL time slot


.. py:method:: TimeSeries.relative( (TimeSeries)arg1) -> bool :
   :module: ecflow

returns a boolean where true means that the time series is relative


.. py:method:: TimeSeries.start( (TimeSeries)arg1) -> TimeSlot :
   :module: ecflow

returns the start time

