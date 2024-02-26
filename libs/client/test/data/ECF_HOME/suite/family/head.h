#!/bin/ksh

set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# Defines the three variables that are needed for any
# communication with ECF_

export ECF_PORT=%ECF_PORT%    # The port number on the server
export ECF_HOST=%ECF_HOST%    # The hostname where the server is running
export ECF_NAME=%ECF_NAME%    # The name of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
if [[ "%ECF_SSL:%" != "" ]] ; then
   export ECF_SSL=%ECF_SSL:%
fi

# Tell ECF_ we have stated
# The ECF_ variable ECF_RID will be set to parameter of smsinit
# Here we give the current PID.

smsinit $$

# Defined a error hanlder

ERROR() {
	set +e        # Clear -e flag, so we don't fail
	smsabort      # Notify ECF_ that something went wrong
	trap 0        # Remove the trap
	exit 0        # End the script
}

# Trap any calls to exit and errors caught by the -e flag

trap ERROR 0

# Trap any signal that may cause the script to fail

trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15
