ecflow.Repeat
/////////////


.. py:class:: Repeat
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Represents one of RepeatString,RepeatEnumerated,RepeatInteger,RepeatDate,RepeatDay


.. py:method:: Repeat.empty( (Repeat)arg1) -> bool :
   :module: ecflow

Return true if the repeat is empty.


.. py:method:: Repeat.end( (Repeat)arg1) -> int :
   :module: ecflow

The last value of the repeat, as an integer


.. py:method:: Repeat.name( (Repeat)arg1) -> str :
   :module: ecflow

The :term:`repeat` name, can be referenced in :term:`trigger` expressions


.. py:method:: Repeat.start( (Repeat)arg1) -> int :
   :module: ecflow

The start value of the repeat, as an integer


.. py:method:: Repeat.step( (Repeat)arg1) -> int :
   :module: ecflow

The increment for the repeat, as an integer


.. py:method:: Repeat.value( (Repeat)arg1) -> int :
   :module: ecflow

The current value of the repeat as an integer

