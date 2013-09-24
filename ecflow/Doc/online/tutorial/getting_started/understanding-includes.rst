.. index::
   single: Understanding Includes
   single: head.h
   single: tail.h
   
.. _understanding-includes:


Understanding Includes
======================
 
In the previous section we created a :term:`task`. 

A task has corresponding :term:`ecf script` which defines the work to be carried out. 
Scripts are similar to UNIX shell scripts.

However :term:`ecf script` includes the addition of "c" like :term:`pre-processing` :term:`directives` and
:term:`variable` s.

The default :term:`pre-processing` :term:`directives` are specified by default using the % character.
 
One of the :term:`pre-processing` :term:`directives` is an include.

The include is used to inject code into a script, and provide a mechanism for code reuse.
If the same code appears in several different :term:`ecf script` files, it should
be placed in a include file instead. This then provides a single point of maintenance.
For example, every :term:`task` needs to set up the communication with
the :term:`ecflow_server` and then tell the server that it (task) has started. 
This 'boilerplate' code is placed in an include file.

.. _head_h:

head.h
------
The :file:`head.h` include file is placed at the start of :term:`ecf script`. It:

* Provides the environment for communication with the :term:`ecflow_server`
* Defines script error handling. When the script fails a trap is raised, we inform the server the :term:`task` has :term:`aborted`.
* Issues a :term:`child command` to inform the server that job has started.

::
    
   #!/bin/ksh
   set -e # stop the shell on first error
   set -u # fail when using an undefined variable
   set -x # echo script lines as they are executed


   # Defines the variables that are needed for any communication with ECF
   export ECF_PORT=%ECF_PORT%    # The server port number
   export ECF_NODE=%ECF_NODE%    # The name of ecf host that issued this task
   export ECF_NAME=%ECF_NAME%    # The name of this current task
   export ECF_PASS=%ECF_PASS%    # A unique password
   export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
   export ECF_RID=$$

   # Define the path where to find ecflow_client
   # make sure client and server use the *same* version. 
   # Important when there are multiple versions of ecFlow
   export PATH=/usr/local/apps/ecflow/%ECF_VERSION%/bin:$PATH

   # Tell ecFlow we have started
   ecflow_client --init=$$


   # Define a error handler
   ERROR() {
      set +e                      # Clear -e flag, so we don't fail
      wait                        # wait for background process to stop
      ecflow_client --abort=trap  # Notify ecFlow that something went wrong, using 'trap' as the reason
      trap 0                      # Remove the trap
      exit 0                      # End the script
   }


   # Trap any calls to exit and errors caught by the -e flag
   trap ERROR 0


   # Trap any signal that may cause the script to fail
   trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15

.. _tail_h:

tail.h
------
The :file:`tail.h` include file is placed at the end of :term:`ecf script` and is 
used to inform the server that job has completed.  It issues the complete :term:`child command`

::

   wait                      # wait for background process to stop
   ecflow_client --complete  # Notify ecFlow of a normal end
   trap 0                    # Remove all traps
   exit 0                    # End the shell

   
**What to do:**
 
* Create the :ref:`head_h` and :ref:`tail_h` files in your $HOME/course directory.
 
 
