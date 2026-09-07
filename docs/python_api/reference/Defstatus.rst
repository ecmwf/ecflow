ecflow.Defstatus
////////////////


.. py:class:: Defstatus
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

:code:`ecflow.Defstatus` represents the :term:`defstatus` attribute, and determines the default state of a
:term:`node` when it *begins* or is *requeued*.

Unless explicitly defined by the user, the default state of a :term:`node` is :term:`queued`. 

The default state :term:`suspended` is special, in the sense that the task will adopt the state
:term:`queued` when it *begins* or is *requeued*, but will require an explicit user resume instruction to
eventually move to :term:`submitted` state.

See :py:class:`ecflow.Node.add_defstatus` and :py:class:`ecflow.DState`


.. py:method:: Defstatus.state(self: ecflow.Defstatus) -> ecflow.DState
   :module: ecflow

