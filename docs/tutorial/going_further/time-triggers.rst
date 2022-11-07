.. index::
   single: time triggers (tutorial)

.. _tutorial-time-triggers:

Time triggers
================

In this section we show an alternative to time-based attributes, using time triggers.
The following suite based generated variables are available for time-based triggers.
(In ecflow_ui, select a suite, then look at the variables tab)

* DD: day of the month
* DOW: day of the week, 0-6, where 0 is Sunday
* DOY: day of the year
* ECF_DATE: ``YYYYMMDD`` year, month, day format. This has the same format as repeat date. 
* MM: month 01-12
* TIME: ``HHMM``
* YYYY: year

These time-based variables on the suite, use the suites calendar. The suites calendar can be configured with the clock attribute.

Here are examples of time attributes and the corresponding trigger examples.

.. code-block:: shell

    time 23:00                  # trigger :TIME == 2300
    date 1.*.*                  # trigger :DD == 1
    day monday                  # trigger :DOW == 1

The ':' means a search for the variable up the node tree.

Triggers can also use AND/OR logic and the full range of operators <,>,<=,>=

.. code-block:: shell
    :caption: Time attributes

    task t1
        day monday
        time 13:00


.. code-block:: shell
    :caption: time based trigger

    task t1
        trigger :DOW == 1 and :TIME >= 1300


.. code-block:: shell
    :caption: combination

    task t1
        day monday
        trigger :TIME >= 1300
    
.. warning::

    It should be noted that relative time( time +00:01) are not possible with time-based triggers, and time series are more problematic.

Text
--------

Let us modify the previous definition file for :term:`family` **f2**.

For brevity, we have omitted the previous :term:`family` **f1**

.. code-block:: shell

    # Definition of the suite test
    suite test
        edit ECF_INCLUDE "$HOME/course"  # replace '$HOME' with the path to your home directory
        edit ECF_HOME    "$HOME/course"
        
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

Python
--------

For brevity, we have left out :term:`family` **f1**. In python this would be:


.. literalinclude:: src/time-triggers.py
   :language: python
   :caption: $HOME/course/test.py

**What to do**

#. Make the changes to the :term:`suite definition` file
#. Replace the :term:`suite`.

   | Python: ``python3 test.py ; python3 client.py``
   | Text: ``ecflow_client --suspend=/test ;  ecflow_client --replace=/test test.def``

#. :ref:`ecflow_ui` has a special window to explain why a task is :term:`queued`. Select a queued task and click on the 'Why tab'
#. Vary the time triggers so that all task runs