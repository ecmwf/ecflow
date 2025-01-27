.. _text_based_def_date:

date
////

This defines a date dependency for a node. There can be multiple date
dependencies. The European format is used for dates, which is:
**dd.mm.yyyy** as in ``31.12.2012``. Any of the three numeric fields can be
expressed with a wildcard \* to mean any valid value. For example,
``01.*.*`` can be used to represent the first day of every month of every year.

.. warning::

    A suite holding a node with a :term:`date` attribute may **never** reach status *complete*.

    A node with a :term:`date` attribute is immediately set to :term:`queued` after completion.
    In practice, this means that nodes containing a :term:`date` attribute, and enveloping suites,
    only reach status *complete* when all specified date values have been exhausted -- which might be **never**.

    **Extreme care** is advised when creating completion :term:`triggers <trigger>` on nodes/suites
    containing :term:`date` attributes.


Currently, a range of values cannot be used to specify any of the three numeric fields in a date.
See :ref:`text_based_def_day` for a way to specify the first seven days.

.. code-block:: shell

   task x          # Run the task twice a month
      date 1.*.*   # month, on 1st and 15th
      date 15.*.*

Because ecFlow was designed with ECMWF suites in mind, the date is a
very important notion. ecFlow defines the time using clocks. A clock is
an attribute of a suite. Different suites can have different clocks.
There are two kinds of clocks:

- Real clocks: A suite using a real clock will have its clock matching the clock of the machine.

- Hybrid clocks: A hybrid clock is a complex notion: the date and time are not connected. The date has a fixed value during the complete execution of the suite. This will be mainly used in cases where the suite does not complete in less than 24 hours. This guarantees that all the tasks of this suite are using the same date. On the other hand, the time follows the time of the machine.

The value of the date is contained in the ecFlow variable
``ECF_DATE``, and the value of the time is in variable ``ECF_TIME``.
The variable ``ECF_CLOCK`` contains other information such as the day of week.
A well designed job should always use these ecFlow variables, and never directly
access the system date.

If a hybrid clock is defined for a suite, any node held by a date
dependency will be set to complete at the beginning of the suite,
without the node ever being despatched. Otherwise the suite would never
complete.
