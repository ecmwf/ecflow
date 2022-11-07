ecflow.RepeatEnumerated
///////////////////////


.. py:class:: RepeatEnumerated
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Allows a node to be repeated using a enumerated list.

A :term:`node` can only have one :term:`repeat`.
The repeat can be referenced in :term:`trigger` expressions.

Constructor::

   RepeatEnumerated(variable,list)
      string variable:     The name of the repeat. The current enumeration index can be
                           referenced in trigger expressions using the variable name
      vector list:         The list of enumerations

Usage::

   t = Task('t1',
            RepeatEnumerated('COLOR', [ 'red', 'green', 'blue' ] ))


.. py:method:: RepeatEnumerated.end( (RepeatEnumerated)arg1) -> int
   :module: ecflow


.. py:method:: RepeatEnumerated.name( (RepeatEnumerated)arg1) -> str :
   :module: ecflow

Return the name of the :term:`repeat`.


.. py:method:: RepeatEnumerated.start( (RepeatEnumerated)arg1) -> int
   :module: ecflow


.. py:method:: RepeatEnumerated.step( (RepeatEnumerated)arg1) -> int
   :module: ecflow

