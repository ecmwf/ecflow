.. index::
   single: Load the file
   single: Loading Suite created in python
   single: Client
   single: add_variable
   single: add_task
   
.. _load-file:
   
Load the file
=============
 
The next step is to let :term:`ecflow_server` know about your :term:`suite` or to "load" the :term:`suite definition` file. 

This checks the file :file:`test.def` and describes the :term:`suite` to the :term:`ecflow_server`.

This can be done in several ways, depending on how the :term:`suite` was created.

.. note::

   Choose between the two methods below, to avoid errors associated with loading :term:`suite definition` twice.

.. _loading_via_cli:

Text
----
 
From within the course directory do the following from the unix shell::

   > ecflow_client --load=test.def
   
| This will check and load the :term:`suite definition` into the :term:`ecflow_server`. 
| If the check fails, the suite is not loaded.

You will have already seen :term:`ecflow_client` being used in :ref:`head_h` and :ref:`tail_h` include files. 
   
Python
------

| We can ask the python script to write out the defs as '.def' definition file
| This can also be useful for debugging when you have a complex :term:`suite definition`:


.. literalinclude:: src/defining-a-new_suite.py
   
If you called "defs.save_as_defs()" the file :file:`test.def` will be written.

This can be loaded in the server as described earlier. (See :ref:`loading_via_cli`)

| Since the :ref:`suite_definition_python_api` allows the definition to be built in memory,  
| it can be directly loaded into the :term:`ecflow_server`.
| This can be done by using :py:class:`ecflow.Client` python class.

::

   try:
      print "Load the in memory definition(defs) into the server"
      ci = ecflow.Client();   
      ci.load(defs)             
   except RuntimeError, e:
      print "Failed: " + str(e); 

| However it is **recommended** that the building of the suite definition is separated
| from loading it into the server. The loading should be placed into a file. :file:`client.py`.

.. literalinclude:: src/load-the-file.py

If everything is OK, you should have defined a :term:`suite` in the server.

Have a look in the window running the :term:`ecflow_server`, and look at the log file


**What to do:**

1. Load the definition file. Choose between loading as a text file, or using python api.
2. If using python examine :file:`test.def` and create the file :file:`client.py`.
3. Check the log file

| **If** you encounter errors associated with loading the suite twice, then you
| can delete all the suites in the server.

::

   > ecflow_client --delete=_all_
   
| The definition can then be re-loaded.
| Alternatively you can use replace a :term:`suite` or any :term:`node`,  please
| type the following for help on replace.

::

   > ecflow_client --help replace