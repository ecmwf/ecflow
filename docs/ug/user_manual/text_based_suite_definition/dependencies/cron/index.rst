.. _text_based_def_cron:

cron
////

A :term:`cron` defines a time dependency for a :term:`node`, similar to :term:`time`,
but defined in terms of a time series at which the node is allowed to execute.

.. warning::

    A suite holding a node with a :term:`cron` attribute will **never** reach status *complete*.

    A node with a :term:`cron` attribute is immediately set to :term:`queued` after completion.
    In practice, this means that nodes containing a :term:`cron` attribute, and enveloping suites,
    do not reach status *complete*, and their output is not be directly accessible through :term:`ecflow_ui`.

    It is **highly discouraged** to create completion :term:`triggers <trigger>` on nodes/suites containing :term:`cron` attributes.

Consider combining the cron attribute together with event and complete attributes to avoid the suite never completing
-- see more about this usage pattern :ref:`here <cookbook-acquisition_task_pattern>`.

If the task associated with a cron aborts, the :term:`ecflow_server` will not schedule it again.

A :term:`cron` can be parameterised as follows:

.. code-block:: text

   cron -w <weekdays> -d <days_of_month> -m <months> <start_time> <end_time> <increment>

Where

- :code:`-w <weekdays>`, defines the days of the week to run the task.

  Valid values for :code:`<weekdays>` are:

    a) 0 → 6, where 0 is Sunday, 1 is Monday, etc
    b) 0L → 6L, where 0L is last Sunday of the month, 1L is the last Monday, etc

  Example: :code:`-w 0,3,6`

  It is possible to combine values from a) and b), but without overlaying values.
  In case of overlay, such as :code:`cron -w 0,1,2,1L,2L,3L 23:00` an exception
  will be thrown.

  .. warning::

     Support for last weekday of the month is only available in ecFlow 5 or later.

- :code:`-d <days_of_month>`, defines the days of the month to run the task.

  Valid values for :code:`<days_of_month>` are in the range [0, 31] or 'L'.
  'L' is used to specify the last day of the month.

  Example: :code:`-d 1,2,20,31`

  Note: the day of month is ignored if not part of the month, e.g. February 31st.

  .. warning::

     Support for last day of the month is only available in ecFlow 5 or later.

- :code:`-m <months>`, defines the months to run the task.

  Valid values are in the range [1, 12], where 1 is January, 2 is February, etc.

- :code:`<start_time>`, defines the lower bound time to run the task.

  Format :code:`hh:mm` (in 24 hour format).

  A :code:`+` prefix can be used to specify a time (maximum of 24 hours)
  relative to suite start time or task requeue time (as part of repeat).

  If no :code:`+` prefix is used, the time is interpreted as an absolute wall-clock time.

- :code:`<end_time>`, an optional argument, defines the (inclusive) upper bound time to run the task.

  Format :code:`hh:mm` (in 24 hour format).

  Necessary when defining multiple steps, must be specified together with <increment>.

  Notice that, even though it is never combined with a :code:`+` prefix, the :code:`<end_time>`
  is considered absolute or relative by alignment with the :code:`<start_time>`.

- :code:`<increment>`, an optional argument, defines the time duration between task runs.

  Format :code:`hh:mm` (in 24 hour format).

  Necessary when defining multiple steps, must be specified together with <end_time>.


Here are some examples of :term:`cron` usage:

.. code-block:: shell

   cron 11:00
       # Run every day, at 11am
       #   Python: cron = Cron("11:00")

   cron 10:00 22:00 00:30
       # Run every day, every 30 minutes, from 10:00 until (and including) 22:00
       #   Python: cron = Cron("10:00 22:00 00:30")

   cron +00:20 23:59 00:30
       # Considering <start_time> the suite start time or task requeue time
       # run at <start_time>+20 minutes, and then every 30 minutes until (and including) <start_time>+23:59.
       # After this the cron will not run again, unless reset.
       #   Python: cron = Cron("+00:20 23:59 00:30")

   cron -w 0,1 10:00 11:00 01:00
       # Run every Sunday(0) & Monday(1), at 10am and 11am
       #   Python: cron = Cron("10:00 11:00 01:00", days_of_week=[0, 1])

   cron -d 15,16 -m 1 10:00 11:00 01:00
       # Run January 15th and 16th, at 10am and 11am
       #   Python: cron = Cron("10:00 11:00 01:00", days_of_month=[15, 16])

   cron -w 5L 23:00
       # Run on *last* Friday(5L) of each month, at 11pm
       #   Python: cron = Cron("23:00",last_week_days_of_the_month=[5])

   cron -w 0,1L 23:00
       # Run every Sunday(0) and *last* Monday(1L) of the month, at 11pm
       #   Python: cron = Cron("23:00", days_of_week=[0], last_week_days_of_the_month=[1])

   cron -w 0L,1L,2L,3L,4L,5L,6L 10:00
       # Run on the last Monday(1), Tuesday(2), ..., Sunday(0) of the month, at 10am
       #   Python: cron = Cron("10:00", last_week_days_of_the_month=[0, 1, 2, 3, 4, 5, 6])

   cron -d 1,L 23:00
       # Run on the first and last of the month, at 11pm
       #   Python: cron = Cron("23:00", days_of_week=[1], last_day_of_the_month=True)


If the time the job takes to complete is longer than the interval a “slot” is missed,
e.g.:

.. code-block:: shell

   cron 10:00 20:00 01:00

if the 10:00 run takes more than an hour, the 11:00 run will never occur.

Absolute vs relative time series
-------------------------------------

The :code:`<start_time>` (and, when present, :code:`<end_time>`) of a :term:`cron` can be given
as an **absolute** wall-clock time (:code:`hh:mm`), or as a time **relative** to the
suite start time or the last :term:`repeat` increment, using the :code:`+` prefix
(:code:`+hh:mm`). The two forms repeat very differently once a day boundary is crossed.

Absolute time series
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An absolute time series automatically restarts itself every day at midnight: as soon as the
day changes, the series rewinds back to :code:`<start_time>` and starts matching wall-clock time
slots again. No extra attribute is required for this — it happens on its own, indefinitely.

For example, given :code:`cron 00:00 23:00 01:00` on a suite that begins at 22:01:

.. list-table::
   :header-rows: 1

   * - Day
     - Wall-clock times the node is freed
   * - Day 1 (suite begins 22:01)
     - 23:00 only — earlier slots (00:00-22:00) had already passed by the time the suite began
   * - Day 2
     - 00:00, 01:00, 02:00, ..., 23:00 (24 times)
   * - Day 3 onward
     - Same as Day 2 — repeats indefinitely, automatically

Relative (+) time series
~~~~~~~~~~~~~~~~~~~~~~~~~~

A relative time series has no such automatic reset. Its elapsed countdown starts at the suite
begin time (or the last :term:`repeat` increment/re-queue) and keeps counting up; it does **not**
rewind at midnight. Once the elapsed time exceeds :code:`<end_time>`, the cron stops becoming
free and will never become free again on its own.

For example, given :code:`cron +00:00 23:00 01:00` on a suite that begins at 22:01:

.. list-table::
   :header-rows: 1

   * - Elapsed since suite begin
     - Wall-clock
     - Node freed?
   * - +00:00
     - 22:01 (day 1)
     - Yes — immediately, since elapsed time is already at the start slot
   * - +01:00
     - 23:01 (day 1)
     - Yes
   * - +02:00, +03:00, ..., +22:00
     - 00:01, 01:01, ..., 20:01 (day 2)
     - Yes — every hour, regardless of the day change; only the elapsed time matters
   * - +23:00
     - 21:01 (day 2)
     - Yes — last valid slot, since elapsed time has reached the :code:`<end_time>` bound
   * - +24:00 (the next slot would be)
     - 22:01 (day 2)
     - No — exceeds the 23-hour :code:`<end_time>` bound; the cron is now permanently inactive

.. note::

   To keep a relative :term:`cron` running indefinitely across days, it must be paired with a
   :term:`repeat` attribute.


Keeping a relative cron alive across days
-----------------------------------------

A **relative** :term:`cron` (one whose :code:`<start_time>` uses the :code:`+` prefix, see
:ref:`text_based_def_cron`) is anchored once, at suite begin time, and its elapsed countdown
never rewinds at midnight. Once the elapsed time runs past :code:`<end_time>`, the cron stops
becoming free and stays inactive forever — unless something resets it.

The way to reset a :term:`cron`, and so keep a relative cron running indefinitely day after day, is to
pair it with a :term:`repeat` attribute placed on the **same** node as the cron, with a plain
dummy :term:`task` underneath (a task with no time attributes of its own). Each time the repeat
increments, the relative cron's elapsed window is reset.

.. code-block:: shell

   family example
      repeat day 1
      cron +00:00 23:00 01:00
      task dummy
   endfamily

The cron must **not** be placed directly on the dummy task instead of the family: a node with a
cron attribute is immediately requeued as soon as it completes, so it never reports completion
up to a parent's :term:`repeat` — thus, the :term:`repeat` would then never advance.
