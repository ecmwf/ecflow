ecflow.RepeatDay
////////////////


.. py:class:: RepeatDay
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

A repeat that is infinite.

A node can only have one :term:`repeat`.

Constructor::

   RepeatDay(step)
      int step:     The step.

Usage:

.. code-block:: python

   t = Task('t1',
             RepeatDay( 1 ))

