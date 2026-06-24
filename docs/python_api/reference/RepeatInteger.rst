ecflow.RepeatInteger
////////////////////


.. py:class:: RepeatInteger
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

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

Accessor methods::

   current_index() -> int
      Zero-based position: (value - start) / step.
   current_value() -> int
      The current integer value.


.. py:method:: RepeatInteger.current_index(self: ecflow.RepeatInteger) -> int
   :module: ecflow

Return the zero-based index of the current value: (value - start) / step.


.. py:method:: RepeatInteger.current_value(self: ecflow.RepeatInteger) -> object
   :module: ecflow

Return the current integer value.


.. py:method:: RepeatInteger.end(self: ecflow.RepeatInteger) -> int
   :module: ecflow


.. py:method:: RepeatInteger.name(self: ecflow.RepeatInteger) -> str
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatInteger.start(self: ecflow.RepeatInteger) -> int
   :module: ecflow


.. py:method:: RepeatInteger.step(self: ecflow.RepeatInteger) -> int
   :module: ecflow

