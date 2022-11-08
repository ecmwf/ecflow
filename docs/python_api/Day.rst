ecflow.Day
//////////


.. py:class:: Day
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Defines a :term:`day` dependency.

There can be multiple day dependencies. If a hybrid :term:`clock` is defined
on a suite, any node held by a day dependency will be set to :term:`complete` at the
beginning of the :term:`suite`, without the task ever being dispatched otherwise
the suite would never complete.

Constructor::

   Day(string)
      string: 'sunday','monday',etc
   Day(Days)
      Days day: Is an enumerator with represent the days of the week

Usage:

.. code-block:: python

   day1 = Day(Days.sunday)
   t = Task('t1',
           Day('tuesday'))


.. py:method:: Day.day( (Day)arg1) -> Days :
   :module: ecflow

Return the day as enumerator

