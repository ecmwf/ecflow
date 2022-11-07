.. _tutorial-data-acquisition:

Data acquisition
----------------

* Every hour, we receive data from Exeter, Toulouse and Offenbach.
* Every three hours we receive data from Washington.
* Once a day we receive data from Tokyo.
* Every Monday from Melbourne.
* Every first of the month from Montreal.

Three kinds of data are received::

* Observations
* GRIB fields
* Satellite images

The acquisition is done in three steps::

    * The data is received from the outside world.
    * The data is processed.
    * The data is stored in a database. 

Once a day the data received the day before is extracted from the database and written into the archive.


**What to do:**

1. Write the :term:`suite definition` for this suite 

Useful pointers:

    * :ref:`tutorial-add-trigger`  
    * :ref:`tutorial-dates-and-clocks`
    * :ref:`tutorial-time-dependencies`  
    * :ref:`tutorial-repeat` 
    * :ref:`python_based_suite_definition` 
 
Because there is no standard unix date manipulation command, you can use ecf_date. 
 
One possible :ref:`tutorial-data_acquisition_soln`
