ecflow.Repeat
/////////////


.. py:class:: Repeat
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Represents one of RepeatString, RepeatEnumerated, RepeatInteger, RepeatDate,
RepeatDateTime, RepeatDateList, RepeatDateTimeList, or RepeatDay.

Accessor methods::

   current_value() -> None # if empty
   current_value() -> int  # if holding RepeatDate/DateList/Integer/Day
   current_value() -> str  # if holdind RepeatDateTime/DateTimeList/Enumerated/RepeatString


.. py:method:: Repeat.current_index(self: ecflow.Repeat) -> int
   :module: ecflow

The current index of the repeat (as an integer)


.. py:method:: Repeat.current_value(self: ecflow.Repeat) -> object
   :module: ecflow

The current value of the repeat (as a string for RepeatDateTime, RepeatDateTimeList, RepeatEnumerated, RepeatString; as an integer for RepeatDate, RepeatDateList, RepeatInteger and RepeatDay)


.. py:method:: Repeat.empty(self: ecflow.Repeat) -> bool
   :module: ecflow

Return true if the repeat is empty.


.. py:method:: Repeat.end(self: ecflow.Repeat) -> int
   :module: ecflow

The last value of the repeat, as an integer


.. py:method:: Repeat.increment(self: ecflow.Repeat) -> None
   :module: ecflow

Increment the repeat to the next value

n.b. this modifies the in-memory local object only --it does not update the ecFlow server.


.. py:method:: Repeat.name(self: ecflow.Repeat) -> str
   :module: ecflow

The :term:`repeat` name, can be referenced in :term:`trigger` expressions


.. py:method:: Repeat.start(self: ecflow.Repeat) -> int
   :module: ecflow

The start value of the repeat, as an integer


.. py:method:: Repeat.step(self: ecflow.Repeat) -> int
   :module: ecflow

The increment for the repeat, as an integer


.. py:method:: Repeat.value(self: ecflow.Repeat) -> int
   :module: ecflow

The current value of the repeat as an integer

