.. _status_of_a_suite_family_or_server:

Status of a suite, family or server
/////////////////////////////////////

The status of a family or suite is the inherited most significant status
of all its children.

The table below shows the order of importance of the different statuses and
some examples of the result of family status, depending on its children.

.. role:: red
.. role:: yellow
.. role:: orange
.. role:: darkgreen
.. role:: lightblue
.. role:: pink
.. role:: magenta
.. role:: mediumturquoise

.. list-table::
   :header-rows: 1

   * - Status
     - Significance   
   * - :yellow:`█` Complete
     - Least Significant
   * - :lightblue:`█` Queued 
     - 
   * - :mediumturquoise:`█` Submitted
     - 
   * - :darkgreen:`█` Active 
     - 
   * - :orange:`█` Suspended 
     - 
   * - :red:`█` Aborted 
     - Most important for a task
   * - :pink:`█` Shutdown 
     - ecFlow server node check_only
   * - :magenta:`█` Halted
     - Most important, only for ecFlow server node


Example of how the status of a family is reported:

.. list-table::
   :header-rows: 1

   * - After begin command
     - First job sent  
     - Second job sent
     - A few tasks running
     - One task aborts!
     - In the end, complete
   * - :lightblue:`█` family
     - :mediumturquoise:`█` family
     - :darkgreen:`█` family
     - :darkgreen:`█` family
     - :red:`█` family
     - :yellow:`█` family
   * -   :lightblue:`█` task1
     -   :mediumturquoise:`█` task1
     -   :darkgreen:`█` task1
     -   :yellow:`█` task1
     -   :yellow:`█` task1
     -   :yellow:`█` task1
   * -   :lightblue:`█` task2
     -   :lightblue:`█` task2
     -   :mediumturquoise:`█` task2
     -   :darkgreen:`█` task2
     -   :darkgreen:`█` task2
     -   :yellow:`█` task2
   * -   :lightblue:`█` task3
     -   :lightblue:`█` task3
     -   :lightblue:`█` task3
     -   :darkgreen:`█` task3
     -   :red:`█` task3
     -   :yellow:`█` task3
   * -   :lightblue:`█` task4
     -   :lightblue:`█` task4
     -   :lightblue:`█` task4
     -   :mediumturquoise:`█` task4
     -   :mediumturquoise:`█` task4
     -   :yellow:`█` task4
   

The status of the ecFlow server itself can be:

-  Shutdown: Server is not scheduling jobs anymore but allows tasks to
   communicate, or user to execute jobs.

-  Halted: Server is not scheduling and does not allow tasks to
   communicate with it. This is the default status when the server is
   started. This is needed since **recovery** is only possible if the
   server is halted.
