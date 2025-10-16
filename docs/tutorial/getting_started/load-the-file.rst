.. index::
   single: Load the file (tutorial)
   single: Loading Suite created in python (tutorial)
   single: Client (tutorial)
   single: add_variable (tutorial)
   single: add_task (tutorial)
   
.. _tutorial-load-file:
   
Loading the suite
=================
 
The next step is to *load* the :term:`suite definition` to the :term:`ecflow_server`, and thus inform the server about the :term:`suite` structure and components.
As part of loading the :term:`suite definition`, the :term:`ecflow_server` performs several validation steps, and only if the validation is successful will the :term:`suite definition` be accepted.

There are several options to load the :term:`suite definition` to the :term:`ecflow_server`.
If a :term:`suite definition` file, such as :file:`test.def`, was manually created then the CLI :term:`ecflow_client` provides the :code:`--load` command.
If the :term:`suite definition` was programmatically generated, e.g. by a Python script using the :ref:`Python API <python_api>`, the same script can load the :term:`suite definition` file.

.. note::

   Choose only one of two methods below to avoid errors associated with loading :term:`suite definition` twice.


.. tabs::

    .. tab:: CLI

        Load the :file:`test.def` using the command line interface :term:`ecflow_client`:

        .. code-block:: shell

            cd $HOME/course
            ecflow_client --load test.def


        The :term:`suite definition` will be validated and loaded into the :term:`ecflow_server`. If the validation fails, the suite is not loaded.

    .. tab:: Python

        As seen previously, the :ref:`Python API <python_api>` can be used to generate the :term:`suite definition` file.
        This file can be loaded into the server as described earlier using the  the :ref:`Python API <python_api>`,
        as done with the CLI :term:`ecflow_client`. The following code snippet shows how to load the :term:`suite definition` file
        :file:`test.def` into the server using the :ref:`Python API <python_api>`.

        .. literalinclude:: src/load-the-file.py
           :language: python
           :caption: $HOME/course/client.py

        When the script is executed, the :term:`suite definition` will be validated and loaded into the :term:`ecflow_server`.
        If the validation fails, the suite is not loaded.

        Notice that, when using the :ref:`Python API <python_api>`, the :term:`suite definition` is generated in memory
        and can be directly loaded into the server. However it is **highly recommended** to effectively save the :term:`suite definition`
        to a file before loading it into the server, and thus separate the following steps:

        * the generation of the :term:`suite definition`,
        * loading the :term:`suite definition` into the server.

        .. code-block:: python
           :caption: Example of how to load the in-memory definition into the server

           defs = ... # create the suite definition in memory

           try:
              ci = Client()
              print("[1] Load the in memory definition(defs) into the server")
              ci.load(defs)           # load the in memory python definition(def) into server

           except RuntimeError as e:
              print("Failed:",e)



**What to do**

#. Load the :term:`suite definition` file using the CLI :term:`ecflow_client`
#. (Optional) Create the :code:`$HOME/course/client.py` script and :term:`suite definition` file.
#. Inspect the :term:`ecflow_server` log file, and check that the :term:`suite definition` was successfully loaded.

.. warning::

   The same :term:`suite definition` cannot be loaded multiple times.
   If a suite :code:`test` already exists on the server, it can be deleted before loading the :term:`suite definition`
   again, using the :code:`--delete` command:

   .. code-block:: shell

      ecflow_client --delete /test

      # find more about the --delete command with: ecflow_client --help delete

   Alternatively, the :code:`--replace` command can be used to re-load part or all of the suite, as as follows:

   .. code-block:: shell

      ecflow_client --replace /test test.def        # replace the suite test
      ecflow_client --replace /test/t1 test.def     # replace just task t1

      # find more about the --replace command with: ecflow_client --help replace
