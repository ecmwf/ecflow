ecflow.RepeatString
///////////////////


.. py:class:: RepeatString
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

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


.. py:method:: RepeatString.end( (RepeatString)arg1) -> int
   :module: ecflow


.. py:method:: RepeatString.name( (RepeatString)arg1) -> str :
   :module: ecflow

Return the name of the :term:`repeat`.


.. py:method:: RepeatString.start( (RepeatString)arg1) -> int
   :module: ecflow


.. py:method:: RepeatString.step( (RepeatString)arg1) -> int
   :module: ecflow

