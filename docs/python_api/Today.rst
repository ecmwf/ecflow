ecflow.Today
////////////


.. py:class:: Today
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

:term:`today` is a time dependency that does not wrap to tomorrow.

If the :term:`suite`\ s begin time is past the time given for the Today,
then the node is free to run.

Constructor::

   Today(hour,minute,relative<optional> = false)
      int hour               : hour in 24 clock
      int minute             : minute <= 59
      bool relative<optional>: Default = false,Relative to suite start or repeated node.

   Today(single,relative<optional> = false)
      TimeSlot single        : A single time
      bool relative          : Relative to suite start or repeated node. Default is false

   Today(start,finish,increment,relative<optional> = false)
      TimeSlot start         : The start time
      TimeSlot finish        : The finish/end time. This must be greater than the start time.
      TimeSlot increment     : The increment
      bool relative<optional>: Default = false, Relative to suite start or repeated node.

   Today(time_series)
      TimeSeries time_series: Similar to constructor above

Exceptions:

- raises IndexError when an invalid Today is specified

Usage:

.. code-block:: python

   today1 = Today( 10,10 )                                                   #  today 10:10 
   today2 = Today( TimeSlot(10,10) )                                         #  today 10:10 
   today3 = Today( TimeSlot(10,10), true)                                    #  today +10:10 
   today4 = Today( TimeSlot(10,10), TimeSlot(20,10),TimeSlot(0,10), false )  #  time 10:10 20:10 00:10 
   t = Task('t1',
            today1,today2,today3,today4,
            Today('10:30 20:10 00:10')) # Create today in place


.. py:method:: Today.time_series( (Today)arg1) -> TimeSeries :
   :module: ecflow

Return the Todays time series

