ecflow.RepeatDateTimeList
/////////////////////////


.. py:class:: RepeatDateTimeList
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Allows a :term:`node` to be repeated using an arbitrary list of date+time instants (yyyymmddTHHMMSS format).

A node can only have one :term:`repeat`.
The repeat name can be referenced in :term:`trigger` expressions.

Constructor::

   RepeatDateTimeList(variable, list)
      string variable:          The name of the repeat. The current datetime can be referenced in
                                trigger expressions using the variable name
      list list_of_str:         Arbitrary list of datetime strings in yyyymmddTHHMMSS format

Exception:

- Throws a RuntimeError if any string is not a valid datetime

Usage:

.. code-block:: python

   rep = RepeatDateTimeList('DT', ['20240101T000000', '20240102T120000', '20240103T060000'])
   t = Task('t1',
            RepeatDateTimeList('DT', ['20240101T000000', '20240102T120000']))


.. py:method:: RepeatDateTimeList.end( (RepeatDateTimeList)arg1) -> int :
   :module: ecflow

Return the end instant as seconds since epoch


.. py:method:: RepeatDateTimeList.name( (RepeatDateTimeList)arg1) -> str :
   :module: ecflow

Return the name of the repeat.


.. py:method:: RepeatDateTimeList.start( (RepeatDateTimeList)arg1) -> int :
   :module: ecflow

Return the start instant as seconds since epoch

