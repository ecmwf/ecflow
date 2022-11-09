.. index::
   single: cron (tutorial)

.. _tutorial-add-a-cron:  
   
Add a cron
==============

The ecFlow server records all commands sent to it, in a log file (<host>.<port>.ecf.log)
This log file will grow over time to a considerable size.
In this exercise, we will create a task, whose job is to periodically back up and clear this log file.
This will be done with :term:`cron` attribute introduced in the previous page. A cron will run indefinitely. i.e. once it has completed it will automatically re-queue itself.

For more examples of adding a cron see the :ref:`adding_time_dependencies` and :ref:`python_api`.

Ecf Script
------------

We will create a new script, :file:`clear_log.ecf`.

.. code-block:: shell
    :caption: $HOME/course/test/house_keeping/clear_log.ecf

    %include <head.h>
    
    # copy the log file to the ECF_HOME/log directory
    cp %ECF_LOG% %ECF_HOME%/log/.
    
    # clear the log file
    ecflow_client  --log=clear
    
    %include <tail.h>

Text
------------

For brevity, the previous families have been omitted.

.. code-block:: shell
        
    # Definition of the suite test.
    suite test
        edit ECF_INCLUDE "$HOME/course"    # replace '$HOME' with the path to your home directory
        edit ECF_HOME    "$HOME/course"
        family house_keeping
            task clear_log
            cron -w 0 22:30  # run every Sunday at 10:30pm
        endfamily
    endsuite

Python
----------
For brevity, the previous families have been omitted.


.. literalinclude:: src/add-a-cron.py
   :language: python
   :caption: $HOME/course/test.py
  

**What to do:**

#. Make the changes to the :term:`suite definition` file
#. Create all the necessary :term:`ecf script`\ s by copying the one from **/test/f1/t7**
#. Replace the :term:`suite`.

   | Python: ``python3 test.py ; python3 client.py``
   | Text: ``ecflow_client --suspend=/test ;  ecflow_client --replace=/test test.def``

#. :term:`ecflow_ui` has a special window to explain why a task is :term:`queued`. Select a queued task and click on the 'Why tab'
#. Manually run the task. Examine the log file on disk.

