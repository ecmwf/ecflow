ecflow.Defstatus
////////////////


.. py:class:: Defstatus
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

A :term:`node` can be set with a default status other the :term:`queued`

The default state of a :term:`node` is :term:`queued`.
This defines the state to take at 'begin' or 're-queue' time
See :py:class:`ecflow.Node.add_defstatus` and :py:class:`ecflow.DState`


.. py:method:: Defstatus.state( (Defstatus)arg1) -> DState
   :module: ecflow

