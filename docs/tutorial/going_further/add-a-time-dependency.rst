.. index::
   single: Dependencies (tutorial) 
   single: time (tutorial) 
   single: date (tutorial) 
   single: day (tutorial) 
   single: cron (tutorial) 

.. _tutorial-time-dependencies:

Time Dependencies
=================

Sometimes you want a :term:`task` to run at a given time, or to run every three hours, or to run only on the first of the month, or on Mondays,... For that ecFlow supports :term:`date` and :term:`time` :term:`dependencies`.

.. _time:

time  
---- 

:term:`time` :term:`dependencies` can be **absolute**, i.e. they will run at the exact time. They can also be **relative**; in this case we provide the time from the moment the :term:`suite` is begun. Time :term:`dependencies` can be repeated at regular intervals.  The nodes stay complete once all-time instances have run.

.. code-block:: shell

   time 23:00                  # at next 23:00
   time 10:00 20:00 01:00      # every hour from 10am to 8pm
   time +00:01                 # one minute after the begin suite
   time +00:10 01:00 00:05     # 10 to 60 minutes after begin every 5 minutes

In the last example, we have a task that runs every five minutes, however, what happens if the task takes longer? 
When this happens, the time slot is **missed**.

.. _cron:

cron
----
Cron :term:`dependencies` can be specified using the :term:`cron` keyword. Cron differs from time as when the node is complete it queues again immediately.  Cron also only works with a real time clock (not a hybrid clock).

.. code-block:: shell

   cron 23:00                 # every day at 23:00
   cron 08:00 12:00 01:00     # every hour between 8 and 12
   cron -w 0,2    11:00       # every sunday and tuesday at 11 am
   cron -d 1,15   02:00       # every 1st and 15th of each month at 2 am
   cron -m 1 -d 1 14:00       # every first of January at 2 pm
   cron -w 5L 23:00           # run on *last* Friday(5L) of each month at 23pm
   cron -d 1,L  23:00         # Run on the first and last of the month at 23pm

.. note::

   **Time,Today,Cron**: When the time has expired, the associated node is free to run. The time will stay expired until the node is re-queued.

.. _date-or-day:

date or day  
----------- 

Date :term:`dependencies` can be specified using the :term:`date` or the :term:`day` keyword. Date :term:`dependencies` are always absolute, but wild cards can be used.

.. code-block:: shell

   date 31.12.2012             # the 31st of December 2012
   date 01.*.*                 # every first of the month
   date *.10.*                 # every day in October
   date 1.*.2008               # every first of the month, but only in 2008
   day monday                  # every monday
   
Mixing time dependencies on the same node
--------------------------------------------   

A task can have many :term:`time` and :term:`date` :term:`dependencies`. For example:

.. code-block:: shell

   task tt
      day monday   # Here Day/date acts like a guard over the time. i.e. time is not considered until Monday
      time 10:00   # run on Monday at 10 am
         
.. code-block:: shell

   day sunday                   
   day wednesday                
   date 01.*.*                 # The first of every month and year
   date 10.*.*                 # The tenth of every month and year
   time 01:00
   time 16:00


The :term:`task` will run on sunday's and wednesday's at 1am and 4pm, but only if the day is the 1st or the 10th of the month.

.. note::

   With multiple time dependencies on the **same** node, the dependencies of the same type are **or' ed** together, then **and' ed** with the different types.

Mixing time dependencies on different nodes
---------------------------------------------

When time dependencies are placed on different nodes in the hierarchy, the results may seem surprising.

.. code-block::
      
   family fam
      day monday    # The day STILL guards the time attribute.
      task tt
         time 10:00 # Will run on Monday at 10 am

.. code-block::

   family fam2
      time 10:00
      task tt
         day monday # This will run on Monday morning at 00:00 and Monday at 10 am

The example above assumes we have a suite, with an infinite repeat loop. So why does the task run on Monday morning at  00:00?
This is because time dependencies on different nodes act independently of each other.  In this case, the time attribute was set free on Sunday at 10 am ( and once free it stays free until it is re-queued).  Hence task **tt** is free to run on Monday morning. After the task has run and re-queued. It will then run on Monday at 10 am.

Like :term:`trigger`\ s, :term:`date` and :term:`time` :term:`dependencies` can be set for a :term:`family`. 
In this case, the tasks of this family will only run according to these :term:`dependencies`.

.. note::

   All time related dependencies(like :term:`cron`, :term:`time`, :term:`today`, :term:`date` and :term:`day`) are relative to the :term:`clock` of the :term:`suite`. 
   
   For more information, see :ref:`tutorial-dates-and-clocks`

Text
----

Let us modify the definition file to add a :term:`family` **f2**. For brevity we have omitted the previous :term:`family` **f1**

.. code-block:: shell

   # Definition of the suite test
   suite test
      edit ECF_INCLUDE "$HOME/course"  # replace '$HOME' with the path to your home directory
      edit ECF_HOME    "$HOME/course"
      
      family f2
         edit SLEEP 20
         task t1
               time 00:30 23:30 00:30  # start(hh:mm) end(hh:mm) increment(hh:mm)
         task t2
               day thursday
               time 13:00
         task t3
               date 1.*.*              # Date(day,month,year) - * means every day,month,year
               time 12:00              # Time is not considered until date is free
         task t4
               time +00:02.            # + means relative to suite begin/requeue time
         task t5
               time 00:02              # 2 minutes past midnight
      endfamily
   endsuite 

Python
------

For brevity we have left out :term:`family` **f1**. In python this would be:

.. literalinclude:: src/add-a-time-dependency.py
   :language: python
   :caption: $HOME/course/test.py


**What to do:**

#. Make the changes to the :term:`suite definition` file
#. Create all the necessary :term:`ecf script`\ s by copying the one from **/test/f1/t7**
#. Replace the :term:`suite`.

   | Python: ``python3 test.py ; python3 client.py``
   | Text: ``ecflow_client --suspend=/test ;  ecflow_client --replace=/test test.def``

#. :term:`ecflow_ui` has a special window to explain why a task is :term:`queued`. Select a queued task and click on the 'Why tab'
#. Vary the time attributes so that all task runs

