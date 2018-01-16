.. index::
   single: Starting the suite
   
.. _starting-suite:
   
Starting the suite
==================
 
The **ecf_start.sh** script will automatically set up and start your :term:`ecflow_server`. 

If you started an ecFlow manually then your server will start in a :term:`halted` state. 

In this case you you will have to "restart" your server before you can use it. 

In a :term:`halted` state the server will not schedule any tasks.
 
Text
----

To check the status of the server, type the following at the unix prompt::

   > ecflow_client --stats

Examine the output. If the :term:`ecflow_server` is :term:`halted` you would restart with::

   > ecflow_client --restart
 
Once the :term:`ecflow_server` is :term:`running` you have to start the :term:`suite` by typing::

   > ecflow_client --begin test

Python
------

| Restarting and begin'ing the suite can also be done with the :ref:`client_server_python_api`.
| Modify your :file:`client.py` file and then run it.

.. warning::

   If you had previously loaded the suite, then comment out the ci.load(..) statement

.. literalinclude:: src/starting-the-suite.py


**What to do:**

1. Restart the :term:`ecflow_server`
2. Begin the :term:`suite`