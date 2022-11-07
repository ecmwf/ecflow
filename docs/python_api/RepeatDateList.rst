ecflow.RepeatDateList
/////////////////////


.. py:class:: RepeatDateList
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Allows a :term:`node` to be repeated using arbitrary list of yyyymmdd integers

A node can only have one :term:`repeat`.
The repeat name can be referenced in :term:`trigger` expressions.
The trigger expression referencing a RepeatDateList will use date arithmetic
Here (/suite/family:YMD + 1) uses date arithmetic only, the result is still an integer

   trigger /suite/family:YMD + 1 > 20190101

Constructor::

   RepeatDateList(variable,list)
      string variable:     The name of the repeat. The current date can referenced in
                           in trigger expressions using the variable name
      list list_of_int:    Arbitrary list of integers following a yyyymmdd format

Exception:

- Throws a RuntimeError if list of integers are not valid dates

Usage::

   rep = RepeatDateList('YMD', [20050130, 20050203] )
   t = Task('t1',
            RepeatDateList('YMD',[20050130, 20050203] ))


.. py:method:: RepeatDateList.end( (RepeatDateList)arg1) -> int :
   :module: ecflow

Return the end date as an integer in yyyymmdd format


.. py:method:: RepeatDateList.name( (RepeatDateList)arg1) -> str :
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatDateList.start( (RepeatDateList)arg1) -> int :
   :module: ecflow

Return the start date as an integer in yyyymmdd format

