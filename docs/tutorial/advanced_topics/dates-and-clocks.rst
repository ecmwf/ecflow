.. index::
   single: dates (tutorial)
   single: clocks (tutorial)
  
.. _tutorial-dates-and-clocks:
 
Dates and Clocks
================

Because ecFlow was designed with ECMWF suites in mind, the date is a very important notion. ecFlow defines the time using clocks. A :term:`clock` is an attribute of a :term:`suite`. Different suites can have different clocks. There are two kinds of clocks:

* :term:`real clock` 
* :term:`hybrid clock` 

A suite clock can be modified by a gain. This is useful for suites running on older data (e.g. cleaning up old data).

The value of the date is in the generated :term:`variable` ECF_DATE, and the value of the time is in ECF_TIME. ECF_CLOCK contains other information such as the day of week.

It is safer for a job to always use the suite generated time and date :term:`variable`'s, and not access directly the system date to prevent confusion.


**What to do:**

#. Try to modify the :term:`suite` to run with a :term:`clock` date from the previous week use the :term:`ecflow_client` ::
 
    ecflow_client --alter change clock_date <day>.<month>.<year>  /test

   E.g.::

    ecflow_client --alter change clock_date 1.4.2020   /test

#. Check the values of the ecFlow :term:`variable`'s
#. Set the suite clock to sync  with the computer::
  
    ecflow_client --alter change clock_sync   /test
  
   Check with::
   
    client --get /test | grep clock

.. note::

   * Be aware that :term:`cron` with a single time dependency will automatically resubmit indefinitely
   * Altering the clock, requires that the suite is re-queued

 