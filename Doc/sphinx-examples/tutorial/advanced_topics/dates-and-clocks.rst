.. index::
   single: dates
   single: clocks
  
.. _dates-and-clocks:
 
Dates and Clocks
================

| Because ecFlow was designed with ECMWF suites in mind, the date is a very important notion. 
| ecFlow defines the time using clocks. A :term:`clock` is an attribute of a :term:`suite`. 
| Different suites can have different clocks. 
| There are two kinds of clocks:

* :term:`real clock` 
    
* :term:`hybrid clock` 

| A suite clock can be modified by a gain. This is useful for suites running on older 
| data (e.g. cleaning up old data).

| The value of the date is in the generated :term:`variable` ECF_DATE, and the value of the time is in ECF_TIME. 
| ECF_CLOCK contains other information such as the day of week.

| It is safer for a job to always use the suite generated time and date :term:`variable`'s, 
| and not access directly the system date to prevent confusion.


**What to do:**

1. Try to modify the :term:`suite` to run with a :term:`clock` date from the previous week use ::term:`ecflow_client` --alter
2. Check the values of the ecFlow :term:`variable`'s

.. note::

   * The attributes :term:`date`, :term:`day` and :term:`clock` are currently not usable if you choose to use a :term:`repeat` date type structure.
   * Be aware that :term:`cron` with a single time dependency will automatically resubmit indefinitely
   * Altering the clock, requires that the suite is re-queued

 