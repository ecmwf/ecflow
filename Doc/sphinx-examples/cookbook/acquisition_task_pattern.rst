
.. index:: single: cookbook-acquisition_task_pattern
   
.. _cookbook-acquisition_task_pattern:

Acquisition task pattern example
--------------------------------

An acquisition task may be needed to initiate the processing. The
following example shows such a pattern where a wait task is created to
check the presence of input data in the acquisition time window 10:00
to 12:00. A task is dedicated to raise a red alarm, by 11:00, if data
are not yet arrived, to have operators and analysts aware in advance
and careful about this. wait task runs every 5 minutes and set the
data event when input data are found ; it shall sleep 60 seconds,
after setting the event, before completing, to ensure the server
receives the event and starts the data task, before wait task
completion and immediate requeue, thanks to cron. data task
(validation and preprocessing) will then set the ready event and acq
family will become complete, provided underneath tasks are complete or
queued, so that rt/wait and late_alert do not run any further.

..image:: check_file.png

::

  family acq
    complete acq/data eq complete
    task data
      trigger rt/wait:data
      event ready
    family rt
      complete data:ready
      task wait
        event data
        cron 10:00 12:00 00:05
    endfamily
    task late_alter
      trigger not data:ready
      time 11:00
  endfamily

In some situation, we may want the acquisition family to detect if
data are already available (catchup mode), and start the processing
immediately, before reaching the real-time mode, when the wait task is
submitted regularly to check data arrival during the time interval.

..image:: check_rtnrt.png

::

  family acquisition
  complete acq/data eq complete
  family rt
    complete data:ready or nrt eq complete
    task wait
      event data
      cron 10:00 12:00 00:05
  endfamily
  task late
    trigger not data:ready
    time 11:00
  family nrt
    complete data:ready
    task wait
      event data
  endfamily
  task data
    trigger rt/wait:data or nrt/wait:data
    event ready
  endfamily
