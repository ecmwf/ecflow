.. index::
   single: Dependencies (tutorial) 
   single: time (tutorial) 
   single: date (tutorial) 
   single: day (tutorial) 
   single: cron (tutorial) 

.. _tutorial-time-dependencies:

Adding a time dependency
========================



Certain :term:`tasks <task>` must run at a expected time, or at a specific time interval, or to run only on the first of the month, or on Mondays,...
For this purpose, ecFlow enables the specification of time :term:`time dependencies`. These time dependencies include: :term:`time`, :term:`day`, :term:`date`, :term:`cron` and :term:`today`.

.. important::

   :term:`time`, :term:`today`, and :term:`cron` dependencies will allow the associated node
   to run when the time has *expired* (i.e. the specified time has been reached).
   The *expired* status of the dependency will remain until the associated node is requeued.

Time Dependencies
-----------------

.. _time:

time  
~~~~

A :term:`time` dependency can be:

* **absolute**, meaning it will run at the exact specified time;
* **relative**, meaning it will run at a time specified in reference to the time the :term:`suite` begun.

:term:`Time <time>` dependencies can be repeated at regular intervals. The nodes remain in complete status once all term:`time` instances have run.

.. code-block:: shell
   :caption: Examples of :code:`time`

   time 23:00                  # at next 23:00
   time 10:00 20:00 01:00      # every hour from 10am to 8pm
   time +00:01                 # one minute after the begin suite
   time +00:10 01:00 00:05     # 10 to 60 minutes after begin every 5 minutes

.. warning::

    The last line of the example above specifies a :term:`time` which will enable the task to run every five minutes.

    In case the task takes longer than five minutes to complete, the time slot is skipped and the task will run only at the next time slot.

.. _cron:

cron
~~~~

A :term:`cron` dependency differs from :term:`time` by immediately requeing a task that completes.
:term:`Cron <cron>` only works with a real time clock (not a hybrid clock).

.. code-block:: shell
   :caption: Examples of :code:`cron`

   cron 23:00                 # every day at 23:00
   cron 08:00 12:00 01:00     # every hour between 8 and 12
   cron -w 0,2    11:00       # every sunday and tuesday at 11 am
   cron -d 1,15   02:00       # every 1st and 15th of each month at 2 am
   cron -m 1 -d 1 14:00       # every first of January at 2 pm
   cron -w 5L 23:00           # run on *last* Friday(5L) of each month at 23pm
   cron -d 1,L  23:00         # Run on the first and last of the month at 23pm

.. _date-or-day:

date or day  
~~~~~~~~~~~

A :term:`date` dependency is always absolute (i.e. no relative dates are allowed), but wildcards specification are allowed.

.. code-block:: shell
   :caption: Examples of :code:`date` and :code:`day`

   date 31.12.2012             # the 31st of December 2012
   date 01.*.*                 # every first of the month
   date *.10.*                 # every day in October
   date 1.*.2008               # every first of the month, but only in 2008
   day monday                  # every monday

today
~~~~~

A :term:`today` is a special case of :term:`time` which, in case that time instant has already passed today, does not wait for the next time instant.

This means that, for example:

* a suite that begins at 3 pm, and has a :term:`today` dependency set to 1 pm, will be immediately allowed to execute.
* a suite that begins at 3 pm, and has a :term:`today` dependency set to 5 pm, will wait until 5 pm to be allows to execute.

Mixing time dependencies
------------------------

On the same node
~~~~~~~~~~~~~~~~

A task can have several :term:`time` and :term:`date` dependencies.

The following example enables the :term:`task` :code:`t` to run on Monday at 10 am.

.. code-block:: shell
   :caption: Examples of running a task on Monday at 10 am

   task t
     day monday   # day allows only Monday's
     time 10:00   # time enables execution at 10 am

The following example enables the :term:`task` :code:`t` to run on the 1st or
the 10th of each month, if these happen to be Sunday or Wednesday, at 1 am or 4 pm.

.. code-block:: shell
   :caption: Examples of running a task with combined time dependencies

   task t
     day sunday    # day allows only Sunday's
     day wednesday # day allows only Wednesday's
     date 01.*.*   # date allows only first of every month/year
     date 10.*.*   # date allows only tenth of every month/year
     time 01:00    # time enables execution at 1 am
     time 16:00    # time enables execution at 4 pm

.. important::

   With multiple time dependencies on the **same** node, the dependencies of the same
   type are **OR**'ed together, and then **AND**'ed amongst the different types.

On different nodes
~~~~~~~~~~~~~~~~~~

When time dependencies are placed at different levels of the nodes hierarchy, the results can seem surprising.

The following example enables the :term:`task` :code:`t` to run on Monday at 10 am.

.. code-block::
      
   family f
      day monday    # day allows only Monday's
      task t
         time 10:00 # time enables execution at 10 am

While the following example enables a seemingly unexpected bebaviour, where the :term:`task` :code:`t` runs on Monday morning at 00:00 and Monday at 10 am.

.. code-block::

   family f
      time 10:00
      task t
         day monday # runs on Monday morning at 00:00 and Monday at 10 am

Assuming the previous example part of a suite with an infinite repeat loop.
Task :code:`t` runs on Monday morning at 00:00, because time dependencies on different nodes act independently of each other.
In this case, the :term:`time` attribute was set free on Sunday at 10 am, and once free it stays free until it is requeued.
This means that task :code:`t` is able to run on Monday morning, and after the task runs and is requeued it will then run again on Monday at 10 am.

Like :term:`triggers <trigger>`, :term:`date` and :term:`time` dependencies can be set for a :term:`family`.
In this case, any task in this family will only run according to these :term:`dependencies`.

.. note::

   All time related dependencies(like :term:`cron`, :term:`time`, :term:`today`, :term:`date` and :term:`day`) are relative to the :term:`clock` of the :term:`suite`. 
   
   For more information, see :ref:`tutorial-dates-and-clocks`

Update Suite Definition
-----------------------

Consider the following modification to the suite definition file, which adds a new :term:`family` :code:`f2` with several :term:`time dependencies`.

.. tabs::

    .. tab:: Text

        .. code-block:: shell

           suite test
              edit ECF_INCLUDE "{{HOME}}/course" # replace '{{HOME}}' appropriately
              edit ECF_HOME    "{{HOME}}/course"

              [... previous family f1 omitted for brevity ..]

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

    .. tab:: Python

        .. literalinclude:: src/add-a-time-dependency.py
           :language: python
           :caption: $HOME/course/test.py

**What to do:**

#. Modify the suite definition to include the new family :code:`f2`, as shown above.
#. Create all the necessary :term:`task scripts <ecf script>` by copying the one from task :code:`/test/f1/t7`.
#. Replace the :term:`suite`, using:

   .. tabs::

      .. tab:: Text

         .. code-block:: shell

            ecflow_client --suspend /test
            ecflow_client --replace /test test.def

      .. tab:: Python

         .. code-block:: shell

            python3 test.py
            python3 client.py

#. Use :term:`ecflow_ui` to inspect why a task is :term:`queued`, by selecting a queued task and clicking on the *Why* tab.
#. (Optional) Adjust the time attributes to make all task runs.
