ecflow.DState
/////////////


.. py:class:: DState
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

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
   :value: <DState.aborted: 3>


.. py:attribute:: DState.active
   :module: ecflow
   :value: <DState.active: 5>


.. py:attribute:: DState.complete
   :module: ecflow
   :value: <DState.complete: 1>


.. py:property:: DState.name
   :module: ecflow


.. py:attribute:: DState.queued
   :module: ecflow
   :value: <DState.queued: 2>


.. py:attribute:: DState.submitted
   :module: ecflow
   :value: <DState.submitted: 4>


.. py:attribute:: DState.suspended
   :module: ecflow
   :value: <DState.suspended: 6>


.. py:attribute:: DState.unknown
   :module: ecflow
   :value: <DState.unknown: 0>


.. py:property:: DState.value
   :module: ecflow

