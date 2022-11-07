ecflow.Days
///////////


.. py:class:: Days
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

This enum is used as argument to a :py:class:`ecflow.Day` class.

It represents the days of the week

Usage::

   day1 = Day(Days.sunday)
   day2 = Day(Days.monday)
   t = Task('t1',
           day1,
           day2,
           Day(Days.tuesday))


.. py:attribute:: Days.friday
   :module: ecflow
   :value: ecflow.Days.friday


.. py:attribute:: Days.monday
   :module: ecflow
   :value: ecflow.Days.monday


.. py:attribute:: Days.names
   :module: ecflow
   :value: {'friday': ecflow.Days.friday, 'monday': ecflow.Days.monday, 'saturday': ecflow.Days.saturday, 'sunday': ecflow.Days.sunday, 'thursday': ecflow.Days.thursday, 'tuesday': ecflow.Days.tuesday, 'wednesday': ecflow.Days.wednesday}


.. py:attribute:: Days.saturday
   :module: ecflow
   :value: ecflow.Days.saturday


.. py:attribute:: Days.sunday
   :module: ecflow
   :value: ecflow.Days.sunday


.. py:attribute:: Days.thursday
   :module: ecflow
   :value: ecflow.Days.thursday


.. py:attribute:: Days.tuesday
   :module: ecflow
   :value: ecflow.Days.tuesday


.. py:attribute:: Days.values
   :module: ecflow
   :value: {0: ecflow.Days.sunday, 1: ecflow.Days.monday, 2: ecflow.Days.tuesday, 3: ecflow.Days.wednesday, 4: ecflow.Days.thursday, 5: ecflow.Days.friday, 6: ecflow.Days.saturday}


.. py:attribute:: Days.wednesday
   :module: ecflow
   :value: ecflow.Days.wednesday

