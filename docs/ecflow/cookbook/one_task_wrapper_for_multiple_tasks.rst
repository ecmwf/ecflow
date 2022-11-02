.. _one_task_wrapper_for_multiple_tasks:

One task wrapper for multiple tasks
///////////////////////////////////

We can keep maintenance simple using a single "task wrapper file",
called in multiple contexts, for multiple tasks. It can be seen as a
single task in research mode, for example, while it is used to generate
multiple jobs in parallel in operation so we can get the expected data
faster, still respecting the blocks that cannot be run in parallel,
calling the task again as a single exclusive job:

.. code-block:: shell
   :caption: task wrapper example: process.ecf

   %manual
   %end
   %include <head.h>
   if [[ %PARALLEL:1% == 1 ]]; then
      for param in %PARAM:2t u v rh%; do
         # process: create ${param}.grib
      done
   fi
   
   if [[ %SERIAL:1% == 1 ]]; then
      for param in %PARAM:2t u v rh%; do
         # push into data base
      done
   fi
   %include <tail.h>  
 
.. code-block:: shell
   :caption: suite definition example

   suite example
   # task process; # research mode, call one task do-it-all
   # operational mode: split parallel and serial blocks:
   family parallel
      edit PARALLEL 1
      edit SERIAL   0
      limit mutex 1
      limit   count 5
      inlimit count
      family 2t
         edit PARAM 2t
         task process
      endfamily
      family u
         edit PARAM u
         task process
      endfamily
      family v
         edit PARAM v
         task process
      endfamily
      family rh
         edit PARAM rh
         task process
      endfamily
   endfamily # parallel
   
   family serial
      trigger parallel eq complete
      inlimit mutex
      edit PARALLEL 0
      edit SERIAL   1
      task process   

A real case example is given by the family ensms (ensemble mean
statistics) where few parameters and multiple level shall generate tasks
and grib files in parallel, while a single task by the end moves the
grib files into the fields data base (" **limit em 1"** is used as a
mutex)

.. image:: /_static/one_task_wrapper_for_multiple_tasks/image1.png
   :width: 2.19583in
   :height: 2.60417in
