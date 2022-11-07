ecflow.DState
/////////////


.. py:class:: DState
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

A DState is like a ecflow.State, except for the addition of SUSPENDED

Suspended stops job generation, and hence is an attribute of a Node.
DState can be used for setting the default state of node when it is
begun or re queued. DState is used for defining :term:`defstatus`.
See :py:class:`ecflow.Node.add_defstatus` and :py:class:`ecflow.Defstatus`
The default state of a :term:`node` is :term:`queued`.

Usage::

   task = ecflow.Task('t1')
   task.add_defstatus(ecflow.DState.complete)   task = ecflow.Task('t2')
   task += Defstatus('complete')
   task = Task('t3',
               Defstatus('complete')) # create in place


.. py:attribute:: DState.aborted
   :module: ecflow
   :value: ecflow.DState.aborted


.. py:attribute:: DState.active
   :module: ecflow
   :value: ecflow.DState.active


.. py:attribute:: DState.complete
   :module: ecflow
   :value: ecflow.DState.complete


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


.. py:attribute:: DState.values
   :module: ecflow
   :value: {0: ecflow.DState.unknown, 1: ecflow.DState.complete, 2: ecflow.DState.queued, 3: ecflow.DState.aborted, 4: ecflow.DState.submitted, 5: ecflow.DState.active, 6: ecflow.DState.suspended}

