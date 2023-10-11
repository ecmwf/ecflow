ecflow.RepeatDateTime
/////////////////////


.. py:class:: RepeatDateTime
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Allows a :term:`node` to be repeated based on date+time instants (using yyyymmddTHHMMSS format).

A node can only have one :term:`repeat`.
The repeat name can be referenced in :term:`trigger` expressions.
The trigger expression referencing a RepeatDate will use date arithmetic
Here (/suite/family:YMD + 1) uses date arithmetic only, the result is still an integer

   trigger /suite/family:YMD + 1 > 20190101

Constructor::

   RepeatDateTime(variable, start, end[, step])
      string variable:     The name of the repeat. The Repeat can referenced in
                           in trigger expressions using the variable name
      int start:           Start date+time instant, formatted as yyyymmddTHHMMSS
      int end:             End date+time instant, formatted as yyyymmddTHHMMSS
      int step:            The increment duration used to update the instant when iterating
                           Formatted as HH:MM.SS, if not provided default delta is '24:00:00'.

Exception:

- Throws a RuntimeError if start/end are not valid date+time instants
- Throws a RuntimeError if delta is not a valid duration

Usage:

.. code-block:: python

   rep = RepeatDateTime('DATETIME', '20050130T000000', '20050203T000000')
   rep = RepeatDateTime('DATETIME', '20050130T000000', '20050203T000000', '48:00:00')
   t = Task('t1',
            RepeatDateTime('DATETIME', '20050130T000000', '20050203T120000', '1:00:00'))


.. py:method:: RepeatDateTime.end( (RepeatDateTime)arg1) -> int :
   :module: ecflow

Return the end date as an integer (i.e. seconds since 19700101T000000)


.. py:method:: RepeatDateTime.name( (RepeatDateTime)arg1) -> str :
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatDateTime.start( (RepeatDateTime)arg1) -> int :
   :module: ecflow

Return the start date as an integer (i.e. seconds since 19700101T000000)


.. py:method:: RepeatDateTime.step( (RepeatDateTime)arg1) -> int :
   :module: ecflow

Return the step increment (in seconds). This is used to update the repeat, until end instant is reached

