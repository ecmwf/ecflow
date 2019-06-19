#!/bin/ksh

set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

## Copyright 2009-2019 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.

# Defines the variables that are needed for any communication with ecflow server

export ECF_PORT=%ECF_PORT%    # ECF port number on the server
export ECF_HOST=%ECF_HOST%    # The hostname where the server is running
export ECF_NAME=%ECF_NAME%    # The name of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
export ECF_RID=$$             # Let All child commands pass process id to server

echo "About to run ecflow_client --version"
%ECF_CLIENT_EXE_PATH:ecflow_client% --version

echo "About to run ecflow_client --ping"
%ECF_CLIENT_EXE_PATH:ecflow_client% --ping

echo "About to run ecflow_client --init"
%ECF_CLIENT_EXE_PATH:ecflow_client% --init=$$

# Define a error hanlder

ERROR() {
	set +e                                       # Clear -e flag, so we don't fail
	wait                                         # wait for background process to stop
	%ECF_CLIENT_EXE_PATH:ecflow_client% --abort  # Notify ECF_ that something went wrong
	trap 0                                       # Remove the trap
	exit 0                                       # End the script
}

# Trap any calls to exit and errors caught by the -e flag

trap ERROR 0

# Trap any signal that may cause the script to fail

trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15
