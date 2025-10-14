.. index::
   single: time triggers (tutorial)

.. _tutorial-time-triggers:

Adding a time trigger
=====================

This section show an alternative to time-based dependencies, using time triggers.
The following suite-based generated variables are available for time-based triggers:

* :code:`DD`: day of the month
* :code:`DOW`: day of the week, 0-6, where 0 is Sunday
* :code:`DOY`: day of the year
* :code:`ECF_DATE`: ``YYYYMMDD`` year, month, day format
* :code:`MM`: month 01-12
* :code:`TIME`: ``HHMM`` hour, minute format
* :code:`YYYY`: year

.. note::

   Find these variables using the :term:`ecflow_ui`, by selecting the suite, then opening the *Variables* tab.

These time-based variables on the :term:`suite` consider the suite calendar, and can be customised via the suite :term:`clock`.

The following example maps several time dependencies and the corresponding time triggers:

.. code-block:: shell

    time 23:00    # trigger :TIME == 2300
    date 1.*.*    # trigger :DD == 1
    day monday    # trigger :DOW == 1

.. note::

    The :code:`:` indicates that the variable is found by searching up the node tree.

Triggers can also use :code:`AND`/:code:`OR` logic, and the full range of operators: :code:`<`, :code:`>`, :code:`<=`, :code:`>=`.

.. code-block:: shell

    task t          # using time attributes
        day monday
        time 13:00


    task t          # using time trigger
        trigger :DOW == 1 and :TIME >= 1300


    task t         # combination of time attributes and time trigger
        day monday
        trigger :TIME >= 1300
    
.. important::

    Relative time (e.g. :code:`time +00:01`) are not possible with time-based triggers.
    The use of time series with time-based triggers is also problematic.

Update Suite Definition
-----------------------

Update the suite definition file so that :term:`family` :code:`f2` uses several time triggers.

.. tabs::

    .. tab:: Text

        .. code-block:: shell

            # Definition of the suite test
            suite test
                edit ECF_INCLUDE "{{HOME}}/course" # replace '$HOME' appropriately
                edit ECF_HOME    "{{HOME}}/course"

                [... previous family f1 omitted for brevity ..]

                family f2
                    edit SLEEP 20
                    task t1
                        trigger :ECF_DATE ==20200720 and :TIME >= 1000
                    task t2
                        trigger :DOW == 4 and :TIME >= 1300
                    task t3
                        trigger :DD == 1 and :TIME >= 1200
                    task t4
                        trigger (:DOW == 1 and :TIME >= 1300) or (:DOW == 5 and :TIME >= 1000)
                    task t5
                        trigger :TIME == 0002              # 2 minutes past midnight
                endfamily
            endsuite

    .. tab:: Python

        .. literalinclude:: src/time-triggers.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Modify the suite definition to update family :code:`f2`, as shown above.
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
