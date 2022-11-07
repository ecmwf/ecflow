ecflow.TimeSlot
///////////////


.. py:class:: TimeSlot
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Represents a time slot.

It is typically used as an argument to a :py:class:`TimeSeries` or
other time dependent attributes of a node.


Constructor::

   TimeSlot(hour,min)
      int hour:   represent an hour:
      int minute: represents a minute:

Usage::

   ts = TimeSlot(10,11)


.. py:method:: TimeSlot.empty( (TimeSlot)arg1) -> bool
   :module: ecflow


.. py:method:: TimeSlot.hour( (TimeSlot)arg1) -> int
   :module: ecflow


.. py:method:: TimeSlot.minute( (TimeSlot)arg1) -> int
   :module: ecflow

