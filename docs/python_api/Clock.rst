ecflow.Clock
////////////


.. py:class:: Clock
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Specifies the :term:`clock` type used by the :term:`suite`.

Only suites can have a :term:`clock`.
A gain can be specified to offset from the given date.

Constructor::

   Clock(day,month,year,hybrid)
      int day              : Specifies the day of the month  1-31
      int month            : Specifies the month 1-12
      int year             : Specifies the year > 1400
      bool hybrid<optional>: Default = False, true means hybrid, false means real
                             by default the clock is not real

      Time will be set to midnight, use set_gain() to alter

   Clock(hybrid)
      bool hybrid: true means hybrid, false means real
                   by default the clock is real
      Time will be set real time of the computer


Exceptions:

- raises IndexError when an invalid Clock is specified

Usage:

.. code-block:: python

   suite = Suite('s1')
   clock = Clock(1,1,2012,False)
   clock.set_gain(1,10,True)
   suite.add_clock(clock)


.. py:method:: Clock.day( (Clock)arg1) -> int :
   :module: ecflow

Returns the day as an integer, range 1-31


.. py:method:: Clock.gain( (Clock)arg1) -> int :
   :module: ecflow

Returns the gain as an long. This represents seconds


.. py:method:: Clock.month( (Clock)arg1) -> int :
   :module: ecflow

Returns the month as an integer, range 1-12


.. py:method:: Clock.positive_gain( (Clock)arg1) -> bool :
   :module: ecflow

Returns a boolean, where true means that the gain is positive


.. py:method:: Clock.set_gain( (Clock)arg1, (int)arg2, (int)arg3, (bool)arg4) -> None :
   :module: ecflow

Set the gain in hours and minutes


.. py:method:: Clock.set_gain_in_seconds( (Clock)arg1, (int)arg2, (bool)arg3) -> None :
   :module: ecflow

Set the gain in seconds


.. py:method:: Clock.year( (Clock)arg1) -> int :
   :module: ecflow

Returns the year as an integer, > 1400

