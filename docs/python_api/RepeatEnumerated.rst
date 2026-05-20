ecflow.RepeatEnumerated
///////////////////////


.. py:class:: RepeatEnumerated
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Allows a node to be repeated using a enumerated list.

A :term:`node` can only have one :term:`repeat`.
The repeat can be referenced in :term:`trigger` expressions.

Constructor::

   RepeatEnumerated(variable,list)
      string variable:     The name of the repeat. The current enumeration index can be
                           referenced in trigger expressions using the variable name
      vector list:         The list of enumerations

Usage:

.. code-block:: python

   t = Task('t1',
            RepeatEnumerated('COLOR', [ 'red', 'green', 'blue' ] ))

Accessor methods::

   current_index() -> int
      Zero-based index of the current enumeration value.
   current_value() -> str
      The enumeration string at the current index, or '' if out of bounds.


.. py:method:: RepeatEnumerated.current_index(self: ecflow.RepeatEnumerated) -> int
   :module: ecflow

Return the zero-based index of the current enumeration value.


.. py:method:: RepeatEnumerated.current_value(self: ecflow.RepeatEnumerated) -> object
   :module: ecflow

Return the enumeration string at the current index.


.. py:method:: RepeatEnumerated.end(self: ecflow.RepeatEnumerated) -> int
   :module: ecflow


.. py:method:: RepeatEnumerated.name(self: ecflow.RepeatEnumerated) -> str
   :module: ecflow

Return the name of the :term:`repeat`.


.. py:method:: RepeatEnumerated.start(self: ecflow.RepeatEnumerated) -> int
   :module: ecflow


.. py:method:: RepeatEnumerated.step(self: ecflow.RepeatEnumerated) -> int
   :module: ecflow

