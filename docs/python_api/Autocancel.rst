ecflow.Autocancel
/////////////////


.. py:class:: Autocancel
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Provides a way to automatically delete/remove a node which has completed

See :term:`autocancel`

Constructor::

   Autocancel(TimeSlot,relative)
      TimeSlot single: A time
      bool relative:   Relative to completion. False means delete the node at the real time specified.

   Autocancel(hour,minute,relative)
      int hour:        hour in 24 hrs
      int minute:      minute <= 59
      bool relative:   Relative to completion. False means delete the node at the real time specified.

   Autocancel(days)
      int days:        Delete the node 'days' after completion

Usage:

.. code-block:: python

   attr = Autocancel( 1,30, true )              # delete node 1 hour and 30 minutes after completion
   attr = Autocancel( TimeSlot(0,10), true )    # delete node 10 minutes after completion
   attr = Autocancel( TimeSlot(10,10), false )  # delete node at 10:10 after completion
   attr = Autocancel( 3  )                      # delete node 3 days after completion

   t1 = Task('t1',
              Autocancel(2,0,true))             # delete task 2 hours after completion


.. py:method:: Autocancel.days( (Autocancel)arg1) -> bool :
   :module: ecflow

Returns a boolean true if time was specified in days


.. py:method:: Autocancel.relative( (Autocancel)arg1) -> bool :
   :module: ecflow

Returns a boolean where true means the time is relative


.. py:method:: Autocancel.time( (Autocancel)arg1) -> TimeSlot :
   :module: ecflow

returns cancel time as a TimeSlot

