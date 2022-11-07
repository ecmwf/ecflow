ecflow.Late
///////////


.. py:class:: Late
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Sets the :term:`late` flag.

When a Node is classified as being late, the only action :term:`ecflow_server` can take
is to set a flag. The GUI will display this alongside the :term:`node` name as a icon.
Only one Late attribute can be specified on a Node.

Constructor::

   Late()
   Late(kwargs)

Usage::

   # This is interpreted as: The node can stay :term:`submitted` for a maximum of 15 minutes
   # and it must become :term:`active` by 20:00 and the run time must not exceed 2 hours
   late = Late()
   late.submitted( 0,15 )
   late.active(   20,0 )
   late.complete(  2,0, true )

   late = Late(submitted='00:15',active='20:00',complete='+02:00')
   t = Task('t1',
            Late(submitted='00:15',active='20:00'))


.. py:method:: Late.active( (Late)arg1, (int)arg2, (int)arg3) -> None :
   :module: ecflow

active(hour,minute): The time the node must become :term:`active`. If the node is still :term:`queued` or :term:`submitted`
    by the time specified, the late flag is set

active( (Late)arg1, (TimeSlot)arg2) -> None :
    active(TimeSlot):The time the node must become :term:`active`. If the node is still :term:`queued` or :term:`submitted`
    by the time specified, the late flag is set

active( (Late)arg1) -> TimeSlot :
    Return the active time as a TimeSlot


.. py:method:: Late.complete( (Late)arg1, (int)arg2, (int)arg3, (bool)arg4) -> None :
   :module: ecflow

complete(hour,minute):The time the node must become :term:`complete`. If relative, time is taken from the time
    the node became :term:`active`, otherwise node must be :term:`complete` by the time given

complete( (Late)arg1, (TimeSlot)arg2, (bool)arg3) -> None :
    complete(TimeSlot): The time the node must become :term:`complete`. If relative, time is taken from the time
    the node became :term:`active`, otherwise node must be :term:`complete` by the time given

complete( (Late)arg1) -> TimeSlot :
    Return the complete time as a TimeSlot


.. py:method:: Late.complete_is_relative( (Late)arg1) -> bool :
   :module: ecflow

Returns a boolean where true means that complete is relative


.. py:method:: Late.is_late( (Late)arg1) -> bool :
   :module: ecflow

Return True if late


.. py:method:: Late.submitted( (Late)arg1, (TimeSlot)arg2) -> None :
   :module: ecflow

submitted(TimeSlot):The time node can stay :term:`submitted`. Submitted is always relative. If the node stays
    submitted longer than the time specified, the :term:`late` flag is set
    

submitted( (Late)arg1, (int)arg2, (int)arg3) -> None :
    submitted(hour,minute) The time node can stay submitted. Submitted is always relative. If the node stays
    submitted longer than the time specified, the late flag is set
    

submitted( (Late)arg1) -> TimeSlot :
    Return the submitted time as a TimeSlot

