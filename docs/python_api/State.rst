ecflow.State
////////////


.. py:class:: State
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

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


.. py:attribute:: State.aborted
   :module: ecflow
   :value: ecflow.State.aborted


.. py:attribute:: State.active
   :module: ecflow
   :value: ecflow.State.active


.. py:attribute:: State.complete
   :module: ecflow
   :value: ecflow.State.complete


.. py:attribute:: State.names
   :module: ecflow
   :value: {'aborted': ecflow.State.aborted, 'active': ecflow.State.active, 'complete': ecflow.State.complete, 'queued': ecflow.State.queued, 'submitted': ecflow.State.submitted, 'unknown': ecflow.State.unknown}


.. py:attribute:: State.queued
   :module: ecflow
   :value: ecflow.State.queued


.. py:attribute:: State.submitted
   :module: ecflow
   :value: ecflow.State.submitted


.. py:attribute:: State.unknown
   :module: ecflow
   :value: ecflow.State.unknown


.. py:attribute:: State.values
   :module: ecflow
   :value: {0: ecflow.State.unknown, 1: ecflow.State.complete, 2: ecflow.State.queued, 3: ecflow.State.aborted, 4: ecflow.State.submitted, 5: ecflow.State.active}

