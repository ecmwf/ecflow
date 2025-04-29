.. _repeat_(for_loop):

repeat (for loop)
/////////////////


Any node can be **repeated** in a number of different ways. Only suites can be repeated based on the suite **clock**. The syntax is as follows

.. code-block:: shell

  repeat day step # only for suites
  repeat integer VARIABLE start end [step]
  repeat enumerated VARIABLE first [second [third ...]]
  repeat string VARIABLE str1 [str2 ...]
  repeat file VARIABLE filename
  repeat date VARIABLE yyyymmdd yyyymmdd [step]
        # when used in trigger expression, we use date arithmetic
  repeat datetime VARIABLE yyyymmddTHHMMSS yyyymmddTHHMMSS [step]
        # loop over time instants separated by a delta duration (given as HH:MM:00)
  repeat datelist VARIABLE 20130101 20130102 20130103 20200101 20190101
        # arbitrary list of dates, also uses date arithmetic in trigger expression

The default value for the step depends on the type of the repeat entity.
When a step is not provided, the default step for both repeat integer and
repeat day is 1, while for repeat datetime the default value is 24:00:00.

The idea is that the variable given is advanced when the node
completes and the node is re-queued (except, of course, when the
variable has the last value.)

Day repeats are only available for a suite (tied to the clock) in
which case an ending date can be given. For this to work the clock
type must be hybrid: a real-time suite cannot be stopped by means of
end time.

.. code-block:: shell

  repeat string INPUT str1 str2 str3
  repeat integer HOUR 6 24 6
  repeat date YMD 20200130 20200203
  repeat enumerated ENUM one two three

.. note:: 
  
  Only four-digit years are allowed. Also, that **force
  complete** will only force the current running job to be complete, but
  if the repetition is not finished, the next job will be sent (with the
  variable advanced accordingly.)

.. tip:: 
  
  We prefer to use the repeat date structure in our suites. This allows us to see more easily what date the suite is running.
