.. _ecflow_cli_details:

CLI details
//////////////////

.. The command level interface is provided by the '**ecflow_client'**
.. executable. By calling:

.. .. code-block:: shell

..     ecflow_client -help


.. We can get the list of all the available commands.

.. Most users of ecFlow will be happy to use GUI, but some things cannot
.. be done using GUI. Note also that most of the commands that you
.. execute using GUI are actually CLI commands.

.. The very first argument to '**ecflow_client'** must begin with '-'

.. .. code-block:: shell

..     ecflow_client --load=/my/home/fred.def
..     ecflow_client --load=/my/home/exotic.def            # will error if suites of same name exists
..     ecflow_client --load=/my/home/exotic.def force      # overwrite suite's of same name in the server
..     ecflow_client --load=/my/home/exotic.def check_only # Just check, don't send to server
..     ecflow_client --load=/my/home/exotic.def stats      # Show defs statistics, don't send to server
..     ecflow_client --load=host1.3141.check               # Load checkpoint file to the server
..     ecflow_client --load=host1.3141.check print         # print definition to standard out in defs format


.. toctree::
    :maxdepth: 2
    
    get
    cli_scripting_in_batch
    configuring_ecflow/index.rst
    compiler_and_os_requirements
    query