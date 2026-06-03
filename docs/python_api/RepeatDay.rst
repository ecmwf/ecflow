ecflow.RepeatDay
////////////////


.. py:class:: RepeatDay
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

A repeat that is infinite.

A node can only have one :term:`repeat`.

Constructor::

   RepeatDay(step)
      int step:     The step.

Usage:

.. code-block:: python

   t = Task('t1',
             RepeatDay( 1 ))

Accessor methods::

   current_index() -> int
      RepeatDay has no position concept; returns the step value.
   current_value() -> int
      The step value as an integer.


.. py:method:: RepeatDay.current_index(self: ecflow.RepeatDay) -> int
   :module: ecflow

Return the step as an integer value (n.b. RepeatDay has no position concept; the step is its only numeric attribute).


.. py:method:: RepeatDay.current_value(self: ecflow.RepeatDay) -> object
   :module: ecflow

Return the step value as an integer value.

