ecflow.Queue
////////////


.. py:class:: Queue
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Queue allows specification of queue on Task, Family and Suite nodes


.. py:method:: Queue.empty(self: ecflow.Queue) -> bool
   :module: ecflow

Return true if the Queue is empty. Used when returning a NULL Queue, from a find


.. py:method:: Queue.index(self: ecflow.Queue) -> int
   :module: ecflow

Return the queue current index as a integer


.. py:method:: Queue.name(self: ecflow.Queue) -> str
   :module: ecflow

Return the queue name as string


.. py:method:: Queue.value(self: ecflow.Queue) -> str
   :module: ecflow

Return the queue current value as string

