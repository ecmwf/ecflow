.. index::
   single: Client
   
.. _understanding-the-client:
   
Understanding the client
========================

All communication with the :term:`ecflow_server` is done with :term:`ecflow_client`

| For any communication with the server, the client needs to know the machine
| where the server is running and the port on the server. There can be multiple 
| servers running on the same machine, each with unique port numbers.

This tutorial will show examples of using the client via the shell and in a Python script.

Client Shell Interface
----------------------
For a full list of available commands type::

   > ecflow_client --help
   
| The :term:`ecflow_client` uses the following method of determining the **host** and **port**

* Default host and port is **localhost**:**3141**

* These defaults are overridden by ECF_HOST and ECF_PORT environment variables

* | This can be further overridden by using --port and --host options
  | and can be used for any of shell level command shown with --help option.
  | For example to ping a server on the command line we can use:
  
  ::

   > ecflow_client --ping --host=machineX --port=4141
   

Client Python Interface
-----------------------
| The functionality provided by :term:`ecflow_client` is also available via the :ref:`client_server_python_api`.  

| The python interface uses the same algorithm for determining the host and port,
| and allows the host and port to be set explicitly. See class :py:class:`ecflow.Client`
| This is shown by the following python example:
  
.. literalinclude:: src/understanding-the-client.py
    
    
**What to do:**

| If your :term:`ecflow_server` was started with **ecf_start.sh** and you want to use the shell
| interface, then set ECF_HOST and ECF_PORT environment variables.
 
| It should be noted that, if the server was started with :file:`ecf_start.sh` script
| then the default "localhost:3141" will be incorrect. If the server was started
| on your local machine, use **netstat** to determine the port number.

:: 

   > netstat -lnptu