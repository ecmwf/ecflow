ecflow.DState
/////////////


.. py:class:: DState
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

:code:`ecflow.DState` is an enumerate, similar to :py:class:`ecflow.State`, with an additional
:code:`suspended` state.

The values in this enumerate are used to create a :py:class:`ecflow.Defstatus` attribute,
which controls the default state of a task when it *begins* or is *requeued*.

Note that :code:`suspended` is not one of the states a node goes through, and has no
:py:class:`ecflow.State` counterpart. A suspended node retains its state, which is reported
again once the node is resumed.

Usage::

   task = ecflow.Task('t1')
   task.add_defstatus(ecflow.DState.complete)
   task = ecflow.Task('t2')
   task += Defstatus('complete')
   task = Task('t3',
               Defstatus('complete')) # create in place


Members:

  unknown

  complete

  queued

  aborted

  submitted

  suspended

  active


.. py:attribute:: DState.aborted
   :module: ecflow
   :value: ecflow.DState.aborted


.. py:attribute:: DState.active
   :module: ecflow
   :value: ecflow.DState.active


.. py:attribute:: DState.complete
   :module: ecflow
   :value: ecflow.DState.complete


.. py:property:: DState.name
   :module: ecflow


.. py:attribute:: DState.names
   :module: ecflow
   :value: {'aborted': ecflow.DState.aborted, 'active': ecflow.DState.active, 'complete': ecflow.DState.complete, 'queued': ecflow.DState.queued, 'submitted': ecflow.DState.submitted, 'suspended': ecflow.DState.suspended, 'unknown': ecflow.DState.unknown}


.. py:attribute:: DState.queued
   :module: ecflow
   :value: ecflow.DState.queued


.. py:attribute:: DState.submitted
   :module: ecflow
   :value: ecflow.DState.submitted


.. py:attribute:: DState.suspended
   :module: ecflow
   :value: ecflow.DState.suspended


.. py:attribute:: DState.unknown
   :module: ecflow
   :value: ecflow.DState.unknown


.. py:property:: DState.value
   :module: ecflow


.. py:attribute:: DState.values
   :module: ecflow
   :value: {0: ecflow.DState.unknown, 1: ecflow.DState.complete, 2: ecflow.DState.queued, 3: ecflow.DState.aborted, 4: ecflow.DState.submitted, 5: ecflow.DState.active, 6: ecflow.DState.suspended}

