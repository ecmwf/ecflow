.. index::
   single: Dependencies, 
   single: time
   single: date
   single: day
   single: cron


Time Dependencies
=================

| Sometimes you want a :term:`task` to run at a given time, or to run every three hours, 
| or to run only on the first of the month, or on Mondays,... 
| For that ecFlow supports :term:`date` and :term:`time` :term:`dependencies`.

.. _time:

time  
---- 

| :term:`time` :term:`dependencies` can be **absolute**, i.e. they will run at the exact time. 
| They can also be **relative**; in this case we provide the time from the moment the :term:`suite` is begun. 
| Time :term:`dependencies` can be repeated at regular intervals

::

   time 23:00                  # at next 23:00
   time 10:00 20:00 01:00      # every hour from 10am to 8pm
   time +00:01                 # one minute after the begin suite
   time +00:10 01:00 00:05     # 10 to 60 minutes after begin every 5 minutes


.. _date-or-day:

date or day  
----------- 

| Date :term:`dependencies` can be specified using the :term:`date` or the :term:`day` keyword. 
| Date :term:`dependencies` are always absolute, but wild cards can be used.

::

   date 31.12.2012             # the 31st of December 2012
   date 01.*.*                 # every first of the month
   date *.10.*                 # every day in October
   date 1.*.2008               # every first of the month, but only in 2008
   day monday                  # every monday
   
   
.. _cron:

cron
----
Cron :term:`dependencies` can be specified using the :term:`cron` keyword::

   cron 23:00                  # every day at 23:00
   cron 08:00 12:00 01:00      # every hour between 8 and 12
   cron -w 0,2                 # every sunday and tuesday
   cron -d 1,15                # every 1st and 15th of each month
   cron -m 1 -d 1              # every first of January


A task can have several :term:`time` and :term:`date` :term:`dependencies`. For example::

   day sunday                   
   day wednesday                
   date 01.*.*                 # The first of every month and year
   date 10.*.*                 # The tenth of every month and year
   time 01:00
   time 16:00

| The :term:`task` will run on sunday's and wednesday's at 1am and 4pm, but only if 
| the day is the 1st or the 10th of the month.

| Like :term:`trigger`'s, :term:`date` and :term:`time` :term:`dependencies` can be set for a :term:`family`. 
| In this case, the tasks of this family will only run according to these :term:`dependencies`.

.. note::

   All time related dependencies(like :term:`cron`, :term:`time`, :term:`today`, :term:`date` and :term:`day`) are relative to the :term:`clock` of the :term:`suite`. 
   
   For more information, see :ref:`dates-and-clocks`

Text
----

| Let us modify the definition file to add a :term:`family` **f2**. 
| For brevity we have omitted the previous :term:`family` **f1**

::

   # Definition of the suite test
   suite test
    edit ECF_INCLUDE "$HOME/course"  # replace '$HOME' with the path to your home directory
    edit ECF_HOME    "$HOME/course"
 
    family f2 
        edit SLEEP 20 
        task t1 
            time 00:30 23:30 00:30 
        task t2 
            day sunday 
        task t3 
            date 01.*.* 
            time 12:00 
        task t4 
            time +00:02 
        task t5 
            time 00:02 
    endfamily 
   endsuite

Python
------

For brevity we have left out :term:`family` **f1**. In python this would be:

.. literalinclude:: src/add-a-time-dependency.py
   :lines: 1-4, 22-


**What to do:**

1. Make the changes to the :term:`suite definition` file
2. Create all the necessary :term:`ecf script`'s by copying the one from **/test/f1/t7**
3. Load and begin the suite
4. Once you start the suite some of the task will be immediately :term:`complete`. Why?
5. :term:`ecflowview` has a special window to explain why a task is :term:`queued`. 
   Select a :term:`queued` task and press the |why| icon 

.. |why| image:: why.jpg
