ecflow.Time
///////////


.. py:class:: Time
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Is used to define a :term:`time` dependency

This can then control job submission.
There can be multiple time dependencies for a node, however overlapping times may
cause unexpected results. The time dependency can be made relative to the beginning
of the suite or in repeated families relative to the beginning of the repeated family.

Constructor::

   Time(string)
     string: i.e '00:30' || '00:30 20:00 00:30'   Time(hour,minute,relative<optional> = false)
      int hour:               hour in 24 clock
      int minute:             minute <= 59
      bool relative<optional>: default = False, Relative to suite start or repeated node.

   Time(single,relative<optional> = false)
      TimeSlot single:         A single time
      bool relative:           Relative to suite start or repeated node. Default is false

   Time(start,finish,increment,relative<optional> = false)
      TimeSlot start:          The start time
      TimeSlot finish:         The finish/end time
      TimeSlot increment:      The increment
      bool relative<optional>: default = False, relative to suite start or repeated node

   Time(time_series)
      TimeSeries time_series:Similar to constructor above

Exceptions:

- raises IndexError when an invalid Time is specified

Usage::

   time1 = Time( 10,10 )                                                   #  time 10:10 
   time2 = Time( TimeSlot(10,10), true)                                    #  time +10:10 
   time2 = Time( TimeSlot(10,10), TimeSlot(20,10),TimeSlot(0,10), false )  #  time 10:10 20:10 00:10 

   t = Task('t1',
            time1,time2,time3,
            Time('10:30 20:10 00:10')) # Create time in place


.. py:method:: Time.time_series( (Time)arg1) -> TimeSeries :
   :module: ecflow

Return the Time attributes time series

