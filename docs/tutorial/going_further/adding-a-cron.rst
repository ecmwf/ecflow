.. index::
   single: cron (tutorial)

.. _tutorial-add-a-cron:  
   
Adding a cron
=============

The ecFlow server records all commands sent to it, in a log file (<host>.<port>.ecf.log)
This log file will grow over time to a considerable size.
This section will create a task, whose job is to periodically back up and clear this log file.

This can be done with a :term:`cron` attribute introduced previously.
A :term:`cron` will run indefinitely i.e. once it has completed it will automatically requeue itself.

More examples of how to add a :code:`cron`, using the :ref:`python_api`, can be found at :ref:`adding_time_dependencies`.

Task Script
-----------

Create a new task script named :file:`clear_log.ecf`.

.. code-block:: shell
    :caption: $HOME/course/test/house_keeping/clear_log.ecf

    %include <head.h>
    
    # copy the log file to the ECF_HOME/log directory
    cp %ECF_LOG% %ECF_HOME%/log/.
    
    # clear the log file
    ecflow_client  --log=clear
    
    %include <tail.h>

Update Suite Definition
-----------------------

Create a new family named :code:`house_keeping` and add the task :code:`clear_log` to it, that will run every Sunday at 10:30pm.

.. tabs::

    .. tab:: Text

        .. code-block:: shell

            suite test
                edit ECF_INCLUDE "{{HOME}}/course" # replace '{{HOME}}' appropriately
                edit ECF_HOME    "$HOME/course"

                [... previous families omitted for brevity ..]

                family house_keeping
                    task clear_log
                    cron -w 0 22:30  # run every Sunday at 10:30pm
                endfamily
            endsuite

    .. tab:: Python

        .. literalinclude:: src/add-a-cron.py
           :language: python
           :caption: $HOME/course/test.py
  
**What to do:**

#. Update the :term:`suite definition` to include the new family and task.
#. Create the new task script :file:`clear_log.ecf` in the appropriate directory.
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
#. Manually run the task, and examine the log file on disk.
