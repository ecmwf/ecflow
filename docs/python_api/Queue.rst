ecflow.Queue
////////////


.. py:class:: Queue
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Queue allows specification of queue on Task, Family and Suite nodes


.. py:method:: Queue.empty( (Queue)arg1) -> bool :
   :module: ecflow

Return true if the Queue is empty. Used when returning a NULL Queue, from a find


.. py:method:: Queue.index( (Queue)arg1) -> int :
   :module: ecflow

Return the queue current index as a integer


.. py:method:: Queue.name( (Queue)arg1) -> str :
   :module: ecflow

Return the queue name as string


.. py:method:: Queue.value( (Queue)arg1) -> str :
   :module: ecflow

Return the queue current value as string

