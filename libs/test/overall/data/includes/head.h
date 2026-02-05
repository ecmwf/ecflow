#!/usr/bin/env bash

%manual
#This is the manual from the head.h file
%end

set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

# Record JOB time
%ecfmicro !
job_start_time=$(date +%s)
!ecfmicro %

# Define all variables needed to communicate with ecflow server

export ECF_PORT=%ECF_PORT%    # The port number where the ecflow server is listening
export ECF_HOST=%ECF_HOST:%   # The hostname where the ecflow server is listening
export ECF_NAME=%ECF_NAME%    # The name (i.e. the absolute task path ) of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task

if [[ "%ECF_SSL:%" != "" ]] ; then
   export ECF_SSL=%ECF_SSL:%  # if the ecflow server is using SSL communications, enable it for the client
fi

#
# At this point, a sanity check could be performed,
# to ensure that ${ECF_HOST} is resolvable to an IP address!
#

# The following could be used to enable client debuging
#export ECF_DEBUG_CLIENT=1

# Typically we dont set this, however the zombie automated test require this.
# it allows us to disambiguate a zombie from a real job.
export ECF_RID=$$

# Define a error handler
ERROR() {
    echo "ERROR() called"
    set +e        # Clear -e flag, so we don't fail
    wait          # wait for background process to stop

    # when the following signals arrive do nothing, stops recursive signals/error function being called
    trap 0 1 2 3 4 5 6 7 8 10 12 13 15

    # Notify ECF_ that something went wrong
    %ECF_CLIENT_EXE_PATH% --abort
    trap 0        # Remove the trap
    exit 0        # End the script
}

# Trap any calls to exit and errors caught by the -e flag

trap ERROR 0

# Trap any signal that may cause the script to fail

trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15

%ADD_DELAY_BEFORE_INIT:% # ADD_DELAY_BEFORE_INIT is used by some tests, to slow things down on fast systems

# Record INIT time
%ecfmicro !
init_start_time=$(date +%s)
!ecfmicro %

%ECF_CLIENT_EXE_PATH% --init=$$ %INIT_ADD_VARIABLES:%

# Record TIME between init and complete
%ecfmicro !
start_time=$(date +%s)
!ecfmicro %
echo "Job start: *INIT* took : $((start_time - init_start_time)) secs"
