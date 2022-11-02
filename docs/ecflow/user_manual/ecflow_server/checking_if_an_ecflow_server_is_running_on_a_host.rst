.. _checking_if_an_ecflow_server_is_running_on_a_host:

Checking if an ecFlow server is running on a host
/////////////////////////////////////////////////

You can check if an ecFlow server is running on a system using the ping
command, e.g.

.. code-block:: shell

    # Check if server is running on 'localhost' on port 3141*
    ecflow_client --ping
    
    # Check if server running on machine 'fred' with default port 3141
    ecflow_client –-ping -–host=fred
    
    # Check if server running on 'fred' with port 3222
    ecflow_client –-ping –-host=fred –-port=3222
    
    # Check if server running using ECF_PORT and ECF_HOST
    export ECF_PORT=3144
    export ECF_HOST=fred
    ecflow_client --ping
                                      

.. note::
    
    When ECF_NODE and ECF_PORT are used in conjunction with command-line arguments, then the command line argument takes precedence.
