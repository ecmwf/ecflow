ecflow.RepeatString
///////////////////


.. py:class:: RepeatString
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Allows a :term:`node` to be repeated using a string list.

A :term:`node` can only have one :term:`repeat`.
The repeat can be referenced in :term:`trigger` expressions.

Constructor::

   RepeatString(variable,list)
      string variable:     The name of the repeat. The current index of the string list can be
                           referenced in trigger expressions using the variable name
      vector list:         The list of enumerations

Usage:

.. code-block:: python

   t = Task('t1',
            RepeatString('COLOR', [ 'red', 'green', 'blue' ] ))

Accessor methods::

   current_index() -> int
      Zero-based index into the string list.
   current_value() -> str
      The string at the current index, or '' if out of bounds.


.. py:method:: RepeatString.current_index(self: ecflow.RepeatString) -> int
   :module: ecflow

Return the zero-based index into the string list.


.. py:method:: RepeatString.current_value(self: ecflow.RepeatString) -> object
   :module: ecflow

Return the string at the current index.


.. py:method:: RepeatString.end(self: ecflow.RepeatString) -> int
   :module: ecflow


.. py:method:: RepeatString.name(self: ecflow.RepeatString) -> str
   :module: ecflow

Return the name of the :term:`repeat`.


.. py:method:: RepeatString.start(self: ecflow.RepeatString) -> int
   :module: ecflow


.. py:method:: RepeatString.step(self: ecflow.RepeatString) -> int
   :module: ecflow

