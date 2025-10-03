.. _tutorial-getting-started:

Getting Started
***************

.. toctree::
   :maxdepth: 1
   :hidden:

   defining-a-new-suite
   defining-the-first-task
   checking-job-creation
   checking-the-job
   understanding-the-client
   load-the-file
   starting-the-suite
   checking-the-results
   using-ecflowui
   execute_rerun_and_requeue

This section will guide you through the steps needed to get ecFlow up and running on your environment.
ecFlow is available on several platforms such as Linux and MacOS, including HPC environments as the one available at ECMWF.

The instructions below will help you to get started on your individual machine, a shared machine or the ECMWF HPC.


.. tabs::

    .. tab:: Single Machine

        An individual machine is a machine that is used by a single user, and typically where the user has full control of the environment.

        In an individual machine, an :term:`ecflow_server` can be started by simply calling the following command at the unix prompt:

        .. code-block:: shell

              ecflow_server

        This will launch the server in the foreground, which can be stopped by typing Ctrl-C. To enable a verbose output, use the :code:`-d` option, as follows:

        .. code-block:: shell

              ecflow_server -d

        This command will start an :term:`ecflow_server` running on your system with a default host name of :code:`localhost` and port number :code:`3141`.

        .. warning::

              If another program on the machine is already using the same port number, then an "Address in use" error will be reported and
              the server will not start.

        To start the server on a specific port number you can either specify the :code:`--port` option or define the :code:`ECF_PORT` environment
        variable, as follows:

        .. code-block:: shell

              # using --port option
              ecflow_server --port 3500

              # using ECF_PORT environment variable
              export ECF_PORT=3500; ecflow_server


        While the above is the simplest way to start ecflow_server, the preferred way to start an :term:`ecflow_server` is to use the script
        :code:`ecflow_start.sh`, as this script prevents unintentional shared usage of the server by automatically selecting a port number
        unique to your user ID.

    .. tab:: Shared Machine

        A machine can be shared by multiple users, and multiple ecFlow servers can be executed in parallel.

        To launch an :term:`ecflow_server` on a shared machine, consider using the :code:`ecflow_start.sh` startup script as it will automatically
        run on a port number unique to the user ID.

        By default, this script defines the :term:`ecflow_server` working directory as :code:`$HOME/ecflow_server`, where the log and
        :term:`check point` files are stored. The  :term:`ecflow_server` working directory can be customised using the `-d` option, as follows:

        .. code-block:: shell

              ecflow_start.sh -d /alternative/working/directory/for/ecflow_server

        .. note::

              Please take note of the *host* and *port* reported by :code:`ecf_start.sh`.

              The *host* and *port* number uniquely identify the :term:`ecflow_server`, and will be needed to access the
              server when using :term:`ecflow_client`, :ref:`python_api` or :term:`ecflow_ui`.

    .. tab:: ECMWF HPC

        .. warning::

              The execution of :term:`ecflow_server` directly on the ECMWF HPC environment is **strongly** discouraged.

              Users are instead recommended to request a dedicated ecFlow virtual machine (VM) and run their own Single/Shared machine.


        In the ECMWF HPC environment, the ecFlow executables are made available by the :code:`module` command.
        Run the following commands to load Python 3 and ecFlow:

        .. code-block:: shell

              module load python3
              module load ecflow/new

        As in a Single or Shared machine, an :term:`ecflow_server` can be started by running the following command at the unix prompt:

        .. code-block:: shell

              ecflow_server


Regardless of the selected way to run the :term:`ecflow_server`, the log files and :term:`check point` files are by default created in the working
directory, and will have the prefix :code:`<machine_name>.<port_number>`. This naming schema avoids overwriting information when multiple servers are,
for example, run on different ports.

When relaunching an :term:`ecflow_server`, the server will attempt to automatically reload the :term:`suite definition` from the :term:`check point` file.

**What to do**

#. Ensure that ecFlow is installed and accessible on your system. Run the following command to check that ecFlow is installed:

    .. code-block:: shell
        :caption: Check ecFlow installation
            
        ecflow_server --version
        ecflow_client --version

#. Setup the Tutorial directory, and start the :term:`ecflow_server` using:
      
      .. code-block:: shell
        :caption: Start the ecflow_server (with verbose output)

        mkdir $HOME/course
        cd $HOME/course

        ecflow_server -d
        # n.b. the server runs in the foreground, on port 3141 -- use Ctrl-C to stop it

#. Check if the server is running by using the command:
 
      .. code-block:: shell
        :caption: Ping the server
      
        ecflow_client --ping
