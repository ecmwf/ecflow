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
 
From within the course directory do the following from the unix shell:

.. code-block:: shell

   ecflow_client --load=test.def
   

This will check and load the :term:`suite definition` into the :term:`ecflow_server`. If the check fails, the suite is not loaded.

You will have already seen :term:`ecflow_client` being used in :ref:`head_h` and :ref:`tail_h` include files. 
   
Python
------

We can ask the python script to write out the defs as '.def' definition file. This can also be useful for debugging when you have a complex :term:`suite definition`:

.. literalinclude:: src/defining-a-new_suite.py
   :language: python
   :caption: $HOME/course/test.py
   
If you called "defs.save_as_defs()" the file :file:`test.def` will be written.

This can be loaded in the server as described earlier. (See :ref:`loading_via_cli`). Since the :ref:`suite_definition_python_api` allows the definition to be built in memory, it can be directly loaded into the :term:`ecflow_server`. This can be done by using :py:class:`ecflow.Client` python class.

.. code-block:: python

   try:
      print("Load the in memory definition(defs) into the server")
      ci = ecflow.Client()
      ci.load(defs)           # load the in memory python definition(def) into server
   except RuntimeError as e:
      print("Failed:",e)

However it is **recommended** that the building of the suite definition is separated from loading it into the server. The loading should be placed into a file. :file:`client.py`.

.. literalinclude:: src/load-the-file.py
   :language: python
   :caption: $HOME/course/client.py
   
If everything is OK, you should have defined a :term:`suite` in the server.

Have a look in the window running the :term:`ecflow_server`, and look at the log file

**What to do**

#. Load the definition file. Choose between loading as a text file, or using python API (update $HOME/course/test.py to write out the definition to disk).
#. If using python examine :file:`test.def` and create the file :file:`client.py`.
#. Check the log file

**You cannot use the load command IF the suite exists**:  If you encounter errors associated with loading the suite twice, then you can delete the 'test' suite in the server.

.. code-block:: shell

   ecflow_client --delete /test

**If the suite exists on the server**: The definition can then be re-loaded part or all of the suite. You need to be careful if the suite is set up to run automatically. The replace command is used to re-load part or all of the suite. Please type the following for help on replace.

.. code-block:: shell

   ecflow_client --help replace
   ecflow_client --replace=/test test.def        # e.g. to replace the whole suite
   ecflow_client --replace=/test/t1 test.def     # or just the one task
