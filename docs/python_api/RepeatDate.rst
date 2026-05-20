ecflow.RepeatDate
/////////////////


.. py:class:: RepeatDate
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Allows a :term:`node` to be repeated using a yyyymmdd format

A node can only have one :term:`repeat`.
The repeat name can be referenced in :term:`trigger` expressions.
The trigger expression referencing a RepeatDate will use date arithmetic
Here (/suite/family:YMD + 1) uses date arithmetic only, the result is still an integer

   trigger /suite/family:YMD + 1 > 20190101

Constructor::

   RepeatDate(variable,start,end,delta)
      string variable:     The name of the repeat. The current date can referenced in
                           in trigger expressions using the variable name
      int start:           Start date, must have format: yyyymmdd
      int end:             End date, must have format: yyyymmdd
      int delta<optional>: default = 1, Always in days. The increment used to update the date

Exception:

- Throws a RuntimeError if start/end are not valid dates

Usage:

.. code-block:: python

   rep = RepeatDate('YMD', 20050130, 20050203 )
   rep = RepeatDate('YMD', 20050130, 20050203, 2)
   t = Task('t1',
            RepeatDate('YMD', 20050130, 20050203 ))

Accessor methods::

   current_index() -> int
      Zero-based position of the current date in the sequence.
      Computed using calendar-day (Julian-day) arithmetic,
      so it is correct even when the sequence crosses a month or year boundary.
      Returns 0 at construction / after reset.

   current_value() -> int
      The current instant as an integer in yyyymmdd format.


.. py:method:: RepeatDate.current_index(self: ecflow.RepeatDate) -> int
   :module: ecflow

Return the zero-based index of the current date position.
Uses calendar-day (Julian-day) arithmetic so the result is correct
even for sequences that cross month or year boundaries.


.. py:method:: RepeatDate.current_value(self: ecflow.RepeatDate) -> object
   :module: ecflow

Return the current date as an integer in yyyymmdd format.


.. py:method:: RepeatDate.end(self: ecflow.RepeatDate) -> int
   :module: ecflow

Return the end date as an integer in yyyymmdd format


.. py:method:: RepeatDate.name(self: ecflow.RepeatDate) -> str
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatDate.start(self: ecflow.RepeatDate) -> int
   :module: ecflow

Return the start date as an integer in yyyymmdd format


.. py:method:: RepeatDate.step(self: ecflow.RepeatDate) -> int
   :module: ecflow

Return the step increment. This is used to update the repeat, until end date is reached

