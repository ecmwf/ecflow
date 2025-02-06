.. _text_based_def_cron:

cron
////

A :term:`cron` defines a time dependency for a :term:`node`, similar to :term:`time`,
but one that will be repeated indefinitely.

.. warning::

    A suite holding a node with a :term:`cron` attribute will **never** reach status *complete*.

    A node with a :term:`cron` attribute is immediately set to :term:`queued` after completion.
    In practice, this means that nodes containing a :term:`cron` attribute, and enveloping suites,
    do not reach status *complete*, and their output is not be directly accessible through :term:`ecflow_ui`.

    It is **highly discouraged** to create completion :term:`triggers <trigger>` on nodes/suites containing :term:`cron` attributes.

Consider combining the cron attribute together with event and complete attributes to avoid the suite never completing
-- see more about this usage pattern :ref:`here <cookbook-acquisition_task_pattern>`.

If the task aborts, the :term:`ecflow_server` will not schedule it again.

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
  A '+' prefix can be used to specify a time (maximum of 24 hours)
  relative to suite start time or task requeue time (as part of repeat).

- :code:`<end_time>`, an optional argument, defines the upper bound time to run the task.

  Necessary when defining multiple steps, must be specified together with <increment>.
  Format :code:`hh:mm` (in 24 hour format).

- :code:`<increment>`, an optional argument, defines the time duration between task runs.

  Necessary when defining multiple steps, must be specified together with <end_time>.
  Format :code:`hh:mm` (in 24 hour format).

Here are some examples of :term:`cron` usage:

.. code-block:: shell

   cron 11:00
       # run every day at 11am
       #   Python: cron = Cron("11:00")

   cron 10:00 22:00 00:30
       # run every 30 minutes from 10:00 until (and including) 22:00
       #   Python: cron = Cron("10:00 22:00 00:30")

   cron +00:20 23:59 00:30
       # run 20 minutes after the suite start time or task requeue time
       #   Python: cron = Cron("+00:20 23:59 00:30")

   cron -w 0,1 10:00 11:00 01:00
       # run every Sunday & Monday at 10am and 11am
       #   Python: cron = Cron("10:00 11:00 01:00", days_of_week=[0, 1])

   cron -d 15,16 -m 1 10:00 11:00 01:00
       # run January 15th and 16th at 10am and 11am
       #   Python: cron = Cron("10:00 11:00 01:00", days_of_month=[15, 16])

   cron -w 5L 23:00
       # run on *last* Friday(5L) of each month at 11pm
       #   Python: cron = Cron("23:00",last_week_days_of_the_month=[5])

   cron -w 0,1L 23:00
       # run every Sunday(0) and *last* Monday(1L) of the month at 11pm
       #   Python: cron = Cron("23:00", days_of_week=[0], last_week_days_of_the_month=[1])

   cron -w 0L,1L,2L,3L,4L,5L,6L 10:00
       # run on the last Monday, Tuesday, ..., Sunday of the month at 10am
       #   Python: cron = Cron("10:00", last_week_days_of_the_month=[0, 1, 2, 3, 4, 5, 6])

   cron -d 1,L 23:00
       # Run on the first and last of the month at 11pm
       #   Python: cron = Cron("23:00", days_of_week=[1], last_day_of_the_month=True)


If the time the job takes to complete is longer than the interval a “slot” is missed,
e.g.:

.. code-block:: shell

   cron 10:00 20:00 01:00

if the 10:00 run takes more than an hour, the 11:00 run will never occur.

If the cron defines months, days of the month, or week days or a single time slot
the it relies on a day change, hence if a :term:`hybrid clock` is defined,
then it will be set to :term:`complete` at  the beginning of the :term:`suite`,
without running  the corresponding job.
Otherwise under a hybrid clock the :term:`suite` would never :term:`complete`.
