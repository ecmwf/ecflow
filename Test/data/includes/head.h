#!/bin/ksh

%manual
#This is the manual from the head.h file
%end

set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

# Defines the three variables that are needed for any
# communication with ECF

export ECF_PORT=%ECF_PORT%    # The port number on the ecflow server
export ECF_HOST=%ECF_HOST:%   # The hostname where the server is running
export ECF_NAME=%ECF_NAME%    # The name of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task

# to debug client communication with the server, enable this environment
#export ECF_DEBUG_CLIENT=

# Typically we dont set this, however the zombie automated test require this.
# it allows us to disambiguate a zombie from a real job.
export ECF_RID=$$

# Tell ECF_ we have stated
# The ECF_ variable ECF_RID will be set to parameter of smsinit
# Here we give the current PID.



# Defined a error hanlder
ERROR() {
	echo "ERROR called"
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
%ECF_CLIENT_EXE_PATH% --init=$$

# record shell time
%ecfmicro !
start_time=$(date +%s)
!ecfmicro %
