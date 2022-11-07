.. _text_based_def_day:

day
///

This defines a day dependency for a current node. The parameters are the
names of weekdays (lowercase): **sunday, monday, tuesday, wednesday,
thursday, friday** or **saturday** . Any combination is acceptable.
Names must be typed in full and at least one must be given. Giving the
same weekday more than once is not treated as an error. The names can be
shortened as long as there is no ambiguity.

There can be multiple day dependencies, but it is more convenient to
define just one dependency, listing the weekdays the node is to run.

Combined with a date, you can specify more particular dependencies such
as the first Monday in a month:

.. code-block:: shell

    task x
        date 1-7.*.*    # This will not work
        day monday


Will be listed by the show as

.. code-block:: shell

    task x
        date 1.*.*
        date 2.*.*
        ...
        date 7.*.*
        day monday

Since the number of days in a month varies, there is no direct means to
specify, say, every last Sunday of each month. A list of dates must be
provided. If a hybrid clock is defined, any node held by a day
dependency will be set to complete at the beginning of the suite,
without the node ever being despatched. Otherwise the suite would never
complete.
