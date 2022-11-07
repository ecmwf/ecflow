.. _text_based_def_time:

time
////

This defines a time dependency for a node. Time is expressed in the
format **[h]h:mm** . Only numeric values are allowed. There can be
multiple time dependencies for a node, but overlapping times may cause
unexpected results.

To define a series of times, specify the start time, end time and a time
increment.

If the start time begins with \`+', times are relative to the beginning
of the suite or, in repeated families, relative to the beginning of the
repeated family.

If relative times are being used, the end time is also relative.

.. code-block:: shell

    time 15:00              # at 15:00
    time 10:00 20:00 01:00  # every hour from 10am to 8pm
    time +00:01             # one minute after the suite begins, or one minute after re-queue in the presence of Repeat.
    time +00:10 01:00 00:05 # 10-60 min after begin/re-queue, every 5 min

There is no direct way to specify that a node should be submitted on
different days at different times. To get a task to run at two specific
times you can use two separate time commands.

.. code-block:: shell

    task t1
        time 15:00 # run at 15:00
        time 19:00 # also run at 19:00
    
.. note::
    
    You should take care with tasks using the time command to
    cause the suite to cycle on fast systems. If the task takes less than a
    minute to run then there is a possibility that the trigger will still be
    valid once the suite has cycled. This can be avoided by making sure that
    such tasks take longer than one minute to run, for example, by adding a
    sleep command.
