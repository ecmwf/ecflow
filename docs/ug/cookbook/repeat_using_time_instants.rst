.. _repeat_datetime_and_datetimelist:

Repeat using time instants
**************************

:code:`repeat datetime` and :code:`repeat datetimelist` allow iterating a node over a sequence of
date+time instants (formatted as :code:`yyyymmddTHHMMSS`). They complement :code:`repeat date` and
:code:`repeat datelist` when sub-daily precision is needed — for example, iterating over hourly or
six-hourly data.

Generated variables
-------------------

Both :code:`repeat datetime` and :code:`repeat datetimelist` generate the same set of variables,
which can be used in job scripts (:code:`%INSTANT%`, :code:`%INSTANT_DATE%`, …) and in trigger/complete expressions:

.. list-table::
   :header-rows: 1

   * - Variable
     - Content
   * - ``INSTANT``
     - Current instant, formatted as ``YYYYMMDDThhmmss``
   * - ``CYCLE_DATE``
     - Date part, formatted as ``YYYYMMDD``
   * - ``INSTANT_YYYY``
     - Four-digit year
   * - ``INSTANT_MM``
     - Two-digit month
   * - ``INSTANT_DD``
     - Two-digit day of month
   * - ``INSTANT_DOW``
     - Day of week
   * - ``INSTANT_TIME``
     - Time part, formatted as ``hhmmss``
   * - ``INSTANT_HOURS``
     - Two-digit hour
   * - ``INSTANT_MINUTES``
     - Two-digit minute
   * - ``INSTANT_SECONDS``
     - Two-digit second
   * - ``INSTANT_JULIAN``
     - Julian day corresponding to the date

Choosing between the two
------------------------

- Use **repeat datetime** when the instants follow a regular interval: a start, an end, and a fixed step (e.g. every 6 hours).
- Use **repeat datetimelist** when the instants are irregular or explicitly enumerated (e.g. 00Z on Monday, 12Z on Wednesday, 06Z on Friday).

.. code-block:: shell
   :caption: Regular interval — repeat datetime

   family cycles
     repeat datetime INSTANT 20240101T000000 20240103T180000 06:00:00
     task forecast
     task post
       trigger forecast == complete
   endfamily

.. code-block:: shell
   :caption: Irregular instants — repeat datetimelist

   family cycles
     repeat datetimelist INSTANT 20240101T000000 20240101T120000 20240103T060000
     task forecast
     task post
       trigger forecast == complete
   endfamily

Time instance dependency
------------------------

A common pattern with :code:`repeat datetime` and :code:`repeat datetimelist` is to trigger an overall
post-processing task once a specific iteration — identified by its position in the list — has been processed.
Because the repeat variable is compared as seconds since epoch, any datetime literal in a trigger expression
acts as an index threshold: :code:`f:INSTANT >= 20240101T120000` becomes true the moment the repeat advances
to the second instant (index 1) or beyond.
Combining this with :code:`f/post == complete` ensures the per-cycle work has actually finished before the
overall task starts.

.. code-block:: shell
   :caption: Overall post-processing triggered at specific instants

   suite s
     family f
       repeat datetimelist INSTANT 20240101T000000 20240101T120000 20240103T060000
       task forecast
       task post
         trigger forecast == complete
     endfamily
     task overall_1
       # triggered once the index-1 instant (20240101T120000) has completed
       trigger f:INSTANT >= 20240101T120000 and f/post == complete
     task overall_2
       # triggered once the index-2 instant (20240103T060000) has completed
       trigger overall_1 == complete and f:INSTANT >= 20240103T060000 and f/post == complete
   endsuite

Chaining :code:`overall_2` on :code:`overall_1 == complete` guarantees the intermediate
step always precedes the final one, even if the repeat advances through multiple instants quickly.

Trigger expressions and arithmetic
-----------------------------------

When a :code:`repeat datetime` or :code:`repeat datetimelist` variable is referenced in a trigger
expression, its value is the number of seconds since :code:`19700101T000000`.
Integer arithmetic (adding/subtracting seconds) applies:

.. code-block:: shell

   # task starts only after t1 has started processing, at least, 20240101T060000
   trigger /suite/t1:INSTANT >= 20240101T060000

   # task starts only after t1 has started processing at least 1 day past 20240101T060000
   trigger /suite/t1:INSTANT + 86400 > 20240101T000000

.. tip::

   DateTime instant literals in trigger expressions (e.g. :code:`20240101T060000`) are also
   interpreted as seconds since epoch, so comparisons between repeat variables and datetime
   literals work directly without any conversion.

See also:

.. list-table::

 * - :ref:`repeat_(for_loop)`
   - Full description of all repeat types, generated variables, and trigger semantics
 * - :ref:`cron_repeat_day_1_repeat_date`
   - Analogous patterns using date-based repeats
 * - :ref:`python_api`
   - :py:class:`ecflow.RepeatDateTime`, :py:class:`ecflow.RepeatDateTimeList`
