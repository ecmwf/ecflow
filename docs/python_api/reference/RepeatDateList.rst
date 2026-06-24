ecflow.RepeatDateList
/////////////////////


.. py:class:: RepeatDateList
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Allows a :term:`node` to be repeated using arbitrary list of yyyymmdd integers

A node can only have one :term:`repeat`.
The repeat name can be referenced in :term:`trigger` expressions.
The trigger expression referencing a RepeatDateList will use date arithmetic
Here (/suite/family:YMD + 1) uses date arithmetic only, the result is still an integer:

.. code-block:: shell

   trigger /suite/family:YMD + 1 > 20190101

Constructor::

   RepeatDateList(variable,list)
      string variable:     The name of the repeat. The current date can referenced in
                           in trigger expressions using the variable name
      list list_of_int:    Arbitrary list of integers following a yyyymmdd format

Exception:

- Throws a RuntimeError if list of integers are not valid dates

Usage:

.. code-block:: python

   rep = RepeatDateList('YMD', [20050130, 20050203] )
   t = Task('t1',
            RepeatDateList('YMD',[20050130, 20050203] ))

Accessor methods::

   current_index() -> int
      Zero-based index of the current date in the list.
   current_value() -> int
      The current date as an integer in yyyymmdd format.


.. py:method:: RepeatDateList.current_index(self: ecflow.RepeatDateList) -> int
   :module: ecflow

Return the zero-based index of the current date in the list.


.. py:method:: RepeatDateList.current_value(self: ecflow.RepeatDateList) -> object
   :module: ecflow

Return the current date as an integer in yyyymmdd format.


.. py:method:: RepeatDateList.end(self: ecflow.RepeatDateList) -> int
   :module: ecflow

Return the end date as an integer in yyyymmdd format


.. py:method:: RepeatDateList.name(self: ecflow.RepeatDateList) -> str
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatDateList.start(self: ecflow.RepeatDateList) -> int
   :module: ecflow

Return the start date as an integer in yyyymmdd format

