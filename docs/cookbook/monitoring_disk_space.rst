.. _monitoring_disk_space:

Monitoring disk space
/////////////////////

A :term:`meter` can be used to monitor the disk space available in the
working directory of the suite's tasks. This will enable the
possibility to postpone the execution of space-demanding tasks by defining a trigger condition on
that meter. In this simple pattern, a *check_disk* family contains
the :term:`meter` (named *use*) while an *update_disk* task runs periodically, by
:term:`cron`, to modify the value of the meter.

.. image:: /_static/monitoring_disk_space/image1.png
   :width: 3.61528in
   :height: 0.39583in

.. code-block:: python
   :caption: Family definition

   from ecf import Family,Meter,Task,Cron, ...
   
   ...
   
   Family("check_disk").add(
      Meter("use",-1,100),
      Task("update_disk").add(
         Cron("00:00 23:59 00:05"))
      
   ...


.. code-block:: shell
   :caption: Task definition

   #!/usr/bin/ksh
   
   %manual
   DESCRIPTION
   Check available disk space in working directory's fileset and update meter
   %end
   
   %include <init.h>
   
   value=$(df %WORKDIR% | grep %WORKDIR% | awk '{print $5}' | sed s/%%//g)
   ecflow_client --alter=change meter "use" "$value" /%SUITE%/%FAMILY%
   
   %include <endt.h>


This pattern has been used, for instance, in a suite that reads
re-analysis data from mars and re-archives it with a different
experiment version number. This sort of processing is inherent parallel but, quite
often, it requires the creation of files to temporarily store the
data. So it is a common case that the amount of **disk space** available for those files is
the **main constraint** that limits the parallelism of the process.

.. image:: /_static/monitoring_disk_space/image2.png
   :width: 5in
   :height: 2.54583in
