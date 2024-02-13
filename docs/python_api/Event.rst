ecflow.Event
////////////


.. py:class:: Event
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

:term:`event`\ s are used as signal mechanism.

Typically they would be used to signal partial completion of a :term:`task`
and to be able to :term:`trigger` another job, which is waiting for this partial completion.
Only tasks can have events that are automatically set via a :term:`child command`\ s, see below.
Events are cleared automatically when a :term:`node` is re-queued or begun.
Suites and Families can have events, but these events must be set via the Alter command
Multiple events can be added to a task.
An Event has a number and a optional name. Events are typically used
in :term:`trigger` and :term:`complete expression` , to control job creation.
Event are fired within a script/:term:`job file`, i.e.:

.. code-block:: shell

   ecflow_client --init=$$
   ecflow_client --event=foo
   ecflow_client --complete

Hence the defining of an event for a :term:`task`, should be followed with the addition of ecflow_client --event
:term:`child command` in the corresponding :term:`ecf script` file.

Constructor::

   Event(number, optional<name = ''>)
      int number            : The number must be >= 0
      string name<optional> : If name is given, can only refer to Event by its name

Usage:

.. code-block:: python

   event = Event(2,'event_name')
   task.add_event(event)
   task1.add_event('2')          # create an event '2' and add to the task
   task2.add_event('name')       # create an event 'name' and add to task

   # Events can be created in the Task constructor, like any other attribute
   t = Task('t3',
            Event(2,'event_name'))


.. py:method:: Event.empty( (Event)arg1) -> bool :
   :module: ecflow

Return true if the Event is empty. Used when returning a NULL Event, from a find


.. py:method:: Event.initial_value( (Event)arg1) -> bool :
   :module: ecflow

Return events initial value, This is value taken for begin/re-queue


.. py:method:: Event.name( (Event)arg1) -> str :
   :module: ecflow

Return the Events name as string. If number supplied name may be empty.


.. py:method:: Event.name_or_number( (Event)arg1) -> str :
   :module: ecflow

Returns name or number as string


.. py:method:: Event.number( (Event)arg1) -> int :
   :module: ecflow

Return events number as a integer. If not specified return max integer value


.. py:method:: Event.value( (Event)arg1) -> bool :
   :module: ecflow

Return events current value

