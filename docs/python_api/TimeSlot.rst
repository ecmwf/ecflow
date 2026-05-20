ecflow.TimeSlot
///////////////


.. py:class:: TimeSlot
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Represents a time slot.

It is typically used as an argument to a :py:class:`TimeSeries` or
other time dependent attributes of a node.


Constructor::

   TimeSlot(hour,min)
      int hour:   represent an hour:
      int minute: represents a minute:

Usage::

   ts = TimeSlot(10,11)


.. py:method:: TimeSlot.empty(self: ecflow.TimeSlot) -> bool
   :module: ecflow


.. py:method:: TimeSlot.hour(self: ecflow.TimeSlot) -> int
   :module: ecflow


.. py:method:: TimeSlot.minute(self: ecflow.TimeSlot) -> int
   :module: ecflow

