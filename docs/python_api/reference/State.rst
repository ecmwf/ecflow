ecflow.State
////////////


.. py:class:: State
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

:code:`ecflow.State` is an enumerate with the possible states of a Node

Each :term:`node` has a state, which reflects the life cycle of a node.

An overview of the life cycle of a node is as follows:

- When the definition file is loaded to the :term:`ecflow_server`, the :term:`task` state is :term:`unknown`

- After the begin command, the :term:`task` is either :term:`queued`, :term:`complete`, :term:`aborted`

  Note: the initial :term:`task` state is determined by the :term:`defstatus` attribute.
  See :py:class:`ecflow.Defstatus`

- Once the :term:`dependencies` are resolved, a task is moved to the :term:`submitted` state, and the
  submission process starts

- if the submission fails, the task is moved to the :term:`aborted` state.

- if the submission succeeds, the task is moved to the :term:`active` state

- While the job runs, it may send other message to the server such as:

  - Set an :term:`event`
  - Change a :term:`meter`
  - Change a :term:`label`
  - Send a message to log file

- The job termination is indicated by a complete or aborted message,
  causing the task to move to :term:`complete` or :term:`aborted` state, accordingly.

Members:

  unknown

  complete

  queued

  aborted

  submitted

  active


.. py:attribute:: State.aborted
   :module: ecflow
   :value: ecflow.State.aborted


.. py:attribute:: State.active
   :module: ecflow
   :value: ecflow.State.active


.. py:attribute:: State.complete
   :module: ecflow
   :value: ecflow.State.complete


.. py:property:: State.name
   :module: ecflow


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


.. py:property:: State.value
   :module: ecflow


.. py:attribute:: State.values
   :module: ecflow
   :value: {0: ecflow.State.unknown, 1: ecflow.State.complete, 2: ecflow.State.queued, 3: ecflow.State.aborted, 4: ecflow.State.submitted, 5: ecflow.State.active}

