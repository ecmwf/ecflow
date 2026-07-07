ecflow.Late
///////////


.. py:class:: Late
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Sets the :term:`late` flag.

When a Node is classified as being late, the only action :term:`ecflow_server` can take
is to set a flag. The GUI will display this alongside the :term:`node` name as a icon.
Only one Late attribute can be specified on a Node.

Constructor::

   Late()
   Late(kwargs)

Usage:

.. code-block:: python

   # This is interpreted as: The node can stay :term:`submitted` for a maximum of 15 minutes
   # and it must become :term:`active` by 20:00 and the run time must not exceed 2 hours
   late = Late()
   late.submitted( 0,15 )
   late.active(   20,0 )
   late.complete(  2,0, true )

   late = Late(submitted='00:15',active='20:00',complete='+02:00')
   t = Task('t1',
            Late(submitted='00:15',active='20:00'))


.. py:method:: Late.active(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. active(self: ecflow.Late, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex) -> None

active(hour,minute): The time the node must become :term:`active`. If the node is still :term:`queued` or :term:`submitted`
by the time specified, the late flag is set

2. active(self: ecflow.Late, arg0: ecflow.TimeSlot) -> None

active(TimeSlot):The time the node must become :term:`active`. If the node is still :term:`queued` or :term:`submitted`
by the time specified, the late flag is set

3. active(self: ecflow.Late) -> ecflow.TimeSlot

Return the active time as a TimeSlot


.. py:method:: Late.complete(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. complete(self: ecflow.Late, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: bool) -> None

complete(hour,minute):The time the node must become :term:`complete`. If relative, time is taken from the time
the node became :term:`active`, otherwise node must be :term:`complete` by the time given

2. complete(self: ecflow.Late, arg0: ecflow.TimeSlot, arg1: bool) -> None

complete(TimeSlot): The time the node must become :term:`complete`. If relative, time is taken from the time
the node became :term:`active`, otherwise node must be :term:`complete` by the time given

3. complete(self: ecflow.Late) -> ecflow.TimeSlot

Return the complete time as a TimeSlot


.. py:method:: Late.complete_is_relative(self: ecflow.Late) -> bool
   :module: ecflow

Returns a boolean where true means that complete is relative


.. py:method:: Late.is_late(self: ecflow.Late) -> bool
   :module: ecflow

Return True if late


.. py:method:: Late.submitted(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. submitted(self: ecflow.Late, arg0: ecflow.TimeSlot) -> None

submitted(TimeSlot):The time node can stay :term:`submitted`. Submitted is always relative. If the node stays
submitted longer than the time specified, the :term:`late` flag is set


2. submitted(self: ecflow.Late, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex) -> None

submitted(hour,minute) The time node can stay submitted. Submitted is always relative. If the node stays
submitted longer than the time specified, the late flag is set


3. submitted(self: ecflow.Late) -> ecflow.TimeSlot

Return the submitted time as a TimeSlot

