.. _how_a_job_file_is_created_from_an_ecflow_file:

How a job file is created from an ecFlow file
/////////////////////////////////////////////

The job file is the actual file that ecFlow will submit to the system.
Starting with the following ecFlow file:

.. code-block:: shell
    :caption: task.ecf

    %manual
        OPERATORS: Set the task complete and report next day
    %end
    %include <head.h>
    
        echo do some work
        sleep %SLEEPTIME%
        echo end of job
    
    %include <end.h>


This uses the header files :ref:`head.h <head_h_code>` and :ref:`tail.h <tail_h_code>`. for example as given earlier and with SLEEPTIME defined as having a value **60**. After pre-processing the joillde the header files and variables and exclude comments and man pages. It would look something like:

.. code-block:: shell
    :caption: task.job1

    #!/bin/ksh
    set -e          # stop the shell on first error
    set -u          # fail when using an undefined variable
    set -x          # echo script lines as they are executed
    set -o pipefail # fail if last(rightmost) command exits with a non-zero status
    
    # Defines the variables that are needed for any communication with ECF
    export ECF_PORT=%ECF_PORT%    # The server port number
    export ECF_HOST=%ECF_HOST%    # The name of ecf host that issued this task
    export ECF_NAME=%ECF_NAME%    # The name of this current task
    export ECF_PASS=%ECF_PASS%    # A unique password
    export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
    export ECF_RID=$$             # record the process id. Also used for zombie detection
    
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
    
    echo do some work
    sleep 60
    echo end of job
    
    wait
    ecflow_client --complete
    trap 0
    exit
