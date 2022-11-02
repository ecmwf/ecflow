.. _include_files:

Include files
/////////////

**Include files** are used where the same piece of code can be inserted
into multiple files. This allows all files using that include to be
easily changed. A group of files may have their own include file; e.g.
all the tasks in an archiving family could include one common file for
the variable definitions needed. This makes the maintenance of the tasks
much easier.

In the same way as the *C-pre-processor* , ecFlow **include files** do
**nest** . There is no limit within ecFlow on how many times they nest
beyond system limitations.

In the simplest case, an ecFlow file would have at least two **include
statements** . One include at the beginning and one at the end of the
file. An example is given below. There are two extra lines apart from
the lines needed for the task itself. This helps to understand the
script since only the lines needed for *ecFlow code* is not visible.
this task are visible. The extra

Example of using include statements in ecFlow file:

.. code-block:: shell

    %include <head.h>
        do the steps for the task
    %include <end.h>

When ecFlow needs to read an include-file it tries to locate them     
from the directory pointed to by variable **ECF_INCLUDE** (unless     
full pathname was given.) Typically this variable is set in the suite definition file   
at the same time as **ECF_FILES.**                                    
                                                                    
The start of the definition for a suite will normally be something    
like:                                                                 

.. code-block:: shell

    suite my_suite
        edit ECF_FILES /home/ma/map/def/SUITE/ECFfiles
        edit ECF_INCLUDE /home/ma/map/def/SUITE/include
        edit ECF_HOME /tmp/map/ECF
        ...

You need to declare the ECF-variables needed. In the start of an ecFlow
script you need to make sure that any command failing will be trapped
and calls:

.. code-block:: shell

    ecflow_client --abort="<Reason>" # the reason text, can be         
    displayed in the GUI                                               

You also need to tell ecFlow that the task is active by using:

.. code-block:: shell

    ecflow_client --init=<process id>                                  

In a large suite, with hundreds of tasks, you would need to execute the
same commands in each of them. Editing just a single (header) file is
somewhat easier than editing them all.

.. code-block:: shell
    :caption: head.h
    :name: head_h_code

    #!%SHELL:/bin/ksh% # allow the shell to be configured
    set -e          # stop the shell on first error
    set -u          # fail when using an undefined variable
    set -x          # echo script lines as they are executed
    set -o pipefail # fail if last(rightmost) command exits with a non-zero status
    
    # Defines the variables that are needed for any communication with ECF
    export ECF_PORT=%ECF_PORT%    # The server port number
    export ECF_HOST=%ECF_HOST%    # The host name where the server is running
    export ECF_NAME=%ECF_NAME%    # The name of this current task
    export ECF_PASS=%ECF_PASS%    # A unique password, used for job validation & zombie detection
    export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
    export ECF_RID=$$             # record the process id. Also used for zombie detection
    # export NO_ECF=1             # uncomment to run as a standalone task on the command line
    
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
        exit 0                      # End the script cleanly, server monitors child, an exit 1, will cause another abort and zombie
    }
      
    # Trap any calls to exit and errors caught by the -e flag
    trap ERROR 0
      
    # Trap any signal that may cause the script to fail
    trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15

The same applies to the end of the task. You want to tell the ecFlow  
that the task is complete by using **ecflow_client --complete(CLI)**  
and **un-trap** the shell.                                                                

.. code-block:: shell
    :caption: tail.h
    :name: tail_h_code

    wait                      # wait for background process to stop
    ecflow_client --complete  # Notify ecFlow of a normal end
    trap 0                    # Remove all traps
    exit 0  

Generally, you would have more than just a single **include file** at
the beginning of an ecFlow file, e.g. one to have common options for
your queuing system, then a few lines for the queuing options unique to
that job. There may be an include file to specify options for an
experimental suite, and so on. There are around ten different include
files used in the ECMWF operational suite.
