.. _repeat_(for_loop):

repeat
//////

A node can be automatically **repeated** in a number of different ways.
All nodes can be repeated based on the following (where ``<variable>`` is the name of the variable that will be
generated for each iteration):

- a sequence of integer values

.. code-block:: shell

  repeat integer <variable> <start> <end> [<delta>]
    # <start>, <end>, and <delta> are integer values
    # if <delta> is not provided, the default value is '1'

- a sequence of enumerated (string) values

.. code-block:: shell

  repeat enumerated <variable> <value-1> [<value-2> [<value-3> [... <value-N>]]]
    # <value-X> are string values

- a sequence of string values

.. code-block:: shell

  repeat string <variable> <string-1> [<string-2> [<string-3> [... <string-N>]]]
    # <string-X> are string values

- a sequence of dates, determined by [begin, end] and a delta

.. code-block:: shell

  repeat date <variable> <begin> <end> [<delta>]
    # <begin> and <end> are given as YYYYMMDD
    # <delta> is given as an integer number of days
    # if <delta> is not provided, the default value is '1'
    # date arithmetic is used when evaluating trigger/complete expressions

- a sequence of dates, determined by an arbitrary list of dates

.. code-block:: shell

  repeat datelist <variable> <date-1> [<date-2> [<date-3> [... <date-N>]]]
    # <date-X> are given as YYYYMMDD
    # date arithmetic is used when evaluating trigger/complete expressions

- a sequence of time instants, determined by [begin, end] and a delta

.. code-block:: shell

  repeat datetime <variable> <begin> <end> [<delta>]
    # <begin> and <end> are given as YYYYMMDDTHHMMSS
    # <delta> is given as a duration in the format HH:MM:SS
    # if <delta> is not provided, the default value is '24:00:00'

.. code-block:: shell

  repeat day [<delta>]
    # only for suites
    # <delta> is given as an integer number of days
    # if <delta> is not provided, the default value is '1'

.. important::

  The day repeats, available only for suites, are tied to the clock used by the suite.

  A hybrid clock must be used in order to stop the iteration; a suite using a real-time clock cannot be stopped by
  means of end time.

.. note::

  Completing a node, with **force complete**, will only force the current running job to complete.
  This means that, if the repetition is not finished, the next value of the iteration is used to immediately
  continue with the *next* job, provided any children of the repeated node are also complete.

.. tip::

  The common practice at ECMWF is to use the repeat date when designing a suite.
  This approach is considered superior (compared to using the repeat day on the suite),
  as it allows to easily adjust the date the suite is running.

.. warning::

  When a repeat is associated to a family/suite, this repeat will *only* continue iterating (and thus automatically
  re-queue all children) if all the children are complete.

  Care must be taken when designing a Suite that uses a repeat on a family/suite and the children have triggers or
  attributes that can prevent them from completing (e.g. a cron attribute will immediately re-queue the node once it
  finishes, preventing the iteration of the repeat from moving forward).

Generated variables
-------------------

Each kind of repeat attribute type defines a set of generated variables, which can be used in the generation of job
scripts and included in trigger and complete expressions.

The value of these variables is automatically updated when the node associated with the repeat attribute
completes and the node is re-queued -- except, of course, when the iteration reaches the end.

The set of generated variables depends on the type of repeat, but in general includes the current value of the
iteration, and for date and datetime repeats, components of the date/datetime. The following table specifies the
generated variables for each repeat type:

.. list-table:: Generated variables for each repeat type
   :header-rows: 1
   :widths: 13 87

   * - Repeat Type
     - Variable Name, and Value
   * - ``integer``
     - ``<variable>``, the current integer value of the loop.
   * - ``string``
     - ``<variable>``, the current string value (the item from the list for this iteration).
   * - ``enumerated``
     - ``<variable>``, the current string value (the item from the list for this iteration).

   * - ``date``
     - ``<variable>``, the current date value, formatted as ``YYYYMMDD``.

       ``<variable>_YYYY``, the Year component of the current date (four digits, ``YYYY``).

       ``<variable>_MM``, the Month component of the current date (two digits, ``MM``).

       ``<variable>_DD``, the Day component of the current date (two digits, ``DD``).

       ``<variable>_DOW``, the Day of week of the current date.

       ``<variable>_JULIAN``, the Julian day corresponding to the current date.
   * - ``datelist``
     - ``<variable>``, the current date value from the list, formatted as ``YYYYMMDD``.

       ``<variable>_YYYY``, the Year component of the current date (four digits, ``YYYY``).

       ``<variable>_MM``, the Month component of the current date (two digits, ``MM``).

       ``<variable>_DD``, the Day component of the current date (two digits, ``DD``).

       ``<variable>_DOW``, the Day of week of the current date.

       ``<variable>_JULIAN``, the Julian day corresponding to the current date.
   * - ``datetime``
     - ``<variable>``, the current datetime value, formatted as ``YYYYMMDDThhmmss``.

       ``<variable>_DATE``, the Date of the current datetime, formatted as ``YYYYMMDD``.

       ``<variable>_YYYY``, the Year component of the current datetime (four digits, ``YYYY``).

       ``<variable>_MM``, the Month component of the current datetime (two digits, ``MM``).

       ``<variable>_DD``, the Day component of the current datetime (two digits, ``DD``).

       ``<variable>_DOW``, the Day of week of the current datetime.

       ``<variable>_TIME``, the Time of the current datetime, formatted as ``hhmmss``.

       ``<variable>_HOURS``, the Hour component of the current datetime (two digits, ``hh``).

       ``<variable>_MINUTES``, the Minute component of the current datetime (two digits, ``mm``).

       ``<variable>_SECONDS``, the Seconds component of the current datetime (two digits, ``ss``).

       ``<variable>_JULIAN``, the Julian day corresponding to the date of the current datetime.

When used in trigger or complete expressions, for integer arithmetic, the interpretation of the variables value depends
on the repeat type -- see the following table for details.

  .. list-table:: Interpretation of repeat variable's value in trigger/complete expressions for integer arithmetic
   :header-rows: 1
   :widths: 13 87

   * - Repeat Type
     - Variable Interpretation
   * - ``integer``
     - The value of ``<variable>`` is interpreted as the current integer value of the loop.
   * - ``string``
     - The value of ``<variable>`` is interpreted as the current index in the list (0-based).
   * - ``enumerated``
     - The value of ``<variable>`` is interpreted as the current enumeration value converted to integer, if convertible; otherwise, the value is the current index in the list (0-based).
   * - ``date``
     - The value of ``<variable>`` is interpreted as the current date value, formatted as ``YYYYMMDD``.
       This value will be subject to date arithmetic (e.g ``20090101 - 1 == 20081231``).
   * - ``datelist``
     - The value of ``<variable>`` is interpreted as the current date value, formatted as ``YYYYMMDD``.
       This value will be subject to date arithmetic (e.g ``20090101 - 1 == 20081231``).
   * - ``datetime``
     - The value of ``<variable>_DATE`` is interpreted as the current date value, formatted as ``YYYYMMDD``.
       This value will be subject to date arithmetic (e.g ``20090101 - 1 == 20081231``).

Used in combination with relative time
--------------------------------------

Relative time attributes can be used in combination with a repeat, and will automatically reset when the repeat loops.
This allows to easily delay the starting of a task for a certain amount of time after the previous iteration completes.

.. important::

  When using a relative time attribute, e.g. ``time +02:00``, below a repeat attribute, then this time is considered
  relative to the repeat iteration value.

The following example shows how to delay the starting of task `a` by 1 minute after the previous iteration completes,
using a repeat integer. When task `a` and task `b` complete, the repeat is incremented, and any relative time attributes
are reset. In this case, effectively delaying the starting of task `a` by 1 minute.

.. code-block:: shell

  suite s
    family f
      repeat integer YEAR 1993 2017
      time +00:01    # when the repeat loops delay starting task a, for 1 minute
      task a
      task b
        trigger a  == complete
      endfamily
  endsuite

See also:

.. list-table::

 * - :ref:`ecflow_cli`
   - :ref:`add_cli`, :ref:`alter_cli`
 * - :ref:`python_api`
   - :py:class:`ecflow.Node.add_repeat`, :py:class:`ecflow.Repeat`, :py:class:`ecflow.RepeatDate`, :py:class:`ecflow.RepeatEnumerated`, :py:class:`ecflow.RepeatInteger`, :py:class:`ecflow.RepeatDay`
 * - :ref:`grammar`
   - :token:`repeat`
 * - :ref:`glossary`
   - :term:`repeat`
