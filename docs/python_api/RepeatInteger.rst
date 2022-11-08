ecflow.RepeatInteger
////////////////////


.. py:class:: RepeatInteger
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Allows a :term:`node` to be repeated using a integer range.

A node can only have one :term:`repeat`.
The repeat can be referenced in :term:`trigger` expressions.

Constructor::

   RepeatInteger(variable,start,end,step)
      string variable:     The name of the repeat. The current integer value can be
                           referenced in trigger expressions using the variable name
      int start:           Start integer value
      int end:             End end integer value
      int step<optional>:  Default = 1, The step amount

Usage:

.. code-block:: python

   t = Task('t1',
            RepeatInteger('HOUR', 6, 24, 6 ))


.. py:method:: RepeatInteger.end( (RepeatInteger)arg1) -> int
   :module: ecflow


.. py:method:: RepeatInteger.name( (RepeatInteger)arg1) -> str :
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatInteger.start( (RepeatInteger)arg1) -> int
   :module: ecflow


.. py:method:: RepeatInteger.step( (RepeatInteger)arg1) -> int
   :module: ecflow

