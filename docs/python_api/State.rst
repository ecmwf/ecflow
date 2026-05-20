ecflow.State
////////////


.. py:class:: State
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Each :term:`node` can have a status, which reflects the life cycle of a node.

It varies as follows:

- When the definition file is loaded into the :term:`ecflow_server` the :term:`task` status is :term:`unknown`
- After begin command the :term:`task` s are either :term:`queued`, :term:`complete`, :term:`aborted` or :term:`suspended` ,
  a suspended task means that the task is really :term:`queued` but it must be resumed by
  the user first before it can be :term:`submitted`. See :py:class:`ecflow.DState`
- Once the :term:`dependencies` are resolved a task is submitted and placed into the :term:`submitted` state,
  however if the submission fails, the task is placed in a :term:`aborted` state.
- On a successful submission the task is placed into the :term:`active` state
- Before a job ends, it may send other message to the server such as:
  Set an :term:`event`, Change a :term:`meter`, Change a :term:`label`, send a message to log file

Jobs end by becoming either :term:`complete` or :term:`aborted`

Members:

  unknown

  complete

  queued

  aborted

  submitted

  active


.. py:attribute:: State.aborted
   :module: ecflow
   :value: <State.aborted: 3>


.. py:attribute:: State.active
   :module: ecflow
   :value: <State.active: 5>


.. py:attribute:: State.complete
   :module: ecflow
   :value: <State.complete: 1>


.. py:property:: State.name
   :module: ecflow


.. py:attribute:: State.queued
   :module: ecflow
   :value: <State.queued: 2>


.. py:attribute:: State.submitted
   :module: ecflow
   :value: <State.submitted: 4>


.. py:attribute:: State.unknown
   :module: ecflow
   :value: <State.unknown: 0>


.. py:property:: State.value
   :module: ecflow

