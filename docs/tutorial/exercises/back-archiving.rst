.. _tutorial-back-archiving:

Back Archiving
--------------

This is an example of a suite that is designed to run only once.

    * An old archive system contains data from 1990-01-01 to 1995-07-12.
    * This data needs to be copied to a new archive system.
    * The data needs to be processed before it is written to the new archive system.
    * These data types can be copied independently.
    * The data is organised daily in the old archive. The system should copy one day of data at a time.
    * Only two tasks can access the old archive system at the same time.

**What to do**

1. Write the :term:`suite definition`  
2. Design it so new data types can be easily added with their own periods


Useful pointers:

    * :ref:`tutorial-limits`  
    * :ref:`tutorial-inlimit`  
    * :ref:`tutorial-add-variable`
    * :ref:`tutorial-add-trigger`  
    * :ref:`tutorial-repeat` 
    * :ref:`tutorial-using-python-scripting`
    * :ref:`python_based_suite_definition`
    
One possible :ref:`tutorial-back_archiving_soln`
