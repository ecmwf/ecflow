.. _tutorial-getting-started:

Getting Started
===================

.. toctree::
   :maxdepth: 1
   :hidden:

   defining-a-new-suite 
   understanding-includes
   defining-the-first-task
   checking-job-creation
   checking-the-job
   understanding-the-client
   load-the-file
   starting-the-suite
   checking-the-results
   using-ecflowui
   execute_rerun_and_requeue

First ensure that the paths to ecFlow executables are accessible. At ECMWF this is done using **use** command. Hence type the following at the command line.

.. code-block:: shell

      module load ecflow/new
      module load python3

Create a directory called course in your home directory and change to that directory. If you do not use modules you will need to add the correct path to your ecFlow binaries: e.g. in ksh  ``export PATH=/usr/local/apps/ecflow/5.5.0/bin:$PATH``

.. code-block:: shell
      
      cd $HOME
      mkdir course; cd course

In order to use ecFlow we first need to start the :term:`ecflow_server`

**Shared Machine**

On a shared machine multiple users and ecFlow servers can coexist. To allow this we have a startup script "ecflow_start.sh" that will start an :term:`ecflow_server`.  This will start an :term:`ecflow_server` running on your system with a port number unique to your user ID. By default this script creates ecFlow log and :term:`check point` files in the directory $HOME/ecflow_server. You can change the location of the log and checkpoint files using the -d option, e.g. to output these file in the course directory:

.. code-block:: shell
      
      ecflow_start.sh -d $HOME/course

.. note::
      
      Please keep a note of the **Host** and **Port** given from your ecf_start.sh output for later. 
      The host and port number uniquely identify your :term:`ecflow_server`. 
      When you want to access this server using :term:`ecflow_client`, :ref:`python_api` 
      or :term:`ecflow_ui` you need to know these information.

      By setting the value of the environment variables ECF_HOST and ECF_PORT you 
      identify the server you wish to access. Multiple :term:`ecflow_server`'s can run on the same system.

**Local Machine**

We prefer to start the ecFlow server with the ecflow_start.sh script to help prevent unintentional shared usage of the server. You could have used the default ECF_PORT and started a server running on your own local machine using the following command:

.. code-block:: shell
  
      ecflow_server
  
at the unix prompt. 

This will start an :term:`ecflow_server` running on your system with a default host name of "localhost" and port number of 3141. If another program on your machine is using this same port number, then you will get an "Address in use" error. To start the server on a specific port number you can use:

.. code-block:: shell
   
      ecflow_server --port=3500
      
or

.. code-block:: shell
   
      export ECF_PORT=3500; ecflow_server

:term:`ecflow_server` log files and :term:`check point` files are created in the current directory by default, and have a prefix ``<machine_name>.<port_number>``. As this allows multiple servers to run on the same machine. If you had previously run the same :term:`ecflow_server` in the past it will also attempt to recover the :term:`suite definition` from the :term:`check point` file. 
 
**What to do**

#. Type 'use ecflow' to setup up the paths.

      .. code-block:: shell
        :caption: Access ecFlow command line interface and python interface
            
        module load ecflow/5new
        module load python3

#. Create $HOME/course directory
#. Start the server using:
      
      .. code-block:: shell
        :caption: Start the server, and set ECF_HOME
      
        ecflow_start.sh -d $HOME/course

#. Make a note of the **ECF_HOST** and **ECF_PORT** variables.  
#. Make sure the following does not error:
 
      .. code-block:: shell
        :caption: Check ecFlow python API
      
        python3 -c "import ecflow"

.. note::

      If in the subsequent sections, you have the need to start a new shell and want access to the server, then ensure ECF_PORT is set. (also call **module load ecflow/new**, and **module load python3** in each new shell). The python is needed to access ecFlow python API only.

