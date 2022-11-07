ecflow.Date
///////////


.. py:class:: Date
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Used to define a :term:`date` dependency.

There can be multiple Date dependencies for a :term:`node`.
Any of the 3 attributes, i.e. day, month, year can be wild carded using a zero
If a hybrid :term:`clock` is defined on a suite, any node held by a date dependency
will be set to :term:`complete` at the beginning of the :term:`suite`, without the
task ever being dispatched otherwise, the suite would never complete.

Constructor::

  Date(string)
     string : * means wild card
  Date(day,month,year)
     int day   : represents the day, zero means wild card. day >= 0 & day < 31
     int month : represents the month, zero means wild card. month >= 0 & month < 12
     int year  : represents the year, zero means wild card. year >= 0

Exceptions:

- raises IndexError when an invalid date is specified

Usage::

  date = Date(11,12,2010)  # represent 11th of December 2010
  date = Date(1,0,0);      # means the first day of every month of every year
  t = Task('t1',
            Date('1.*.*'));  # Create Date in place.


.. py:method:: Date.day( (Date)arg1) -> int :
   :module: ecflow

Return the day. The range is 0-31, 0 means its wild-carded


.. py:method:: Date.month( (Date)arg1) -> int :
   :module: ecflow

Return the month. The range is 0-12, 0 means its wild-carded


.. py:method:: Date.year( (Date)arg1) -> int :
   :module: ecflow

Return the year, 0 means its wild-carded

