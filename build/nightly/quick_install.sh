#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# =======================================================================
# Create build scripts files. Must be before python $WK/build/nightly/build.py
# =======================================================================
rm -rf nightly
cp -r $WK/build/nightly .

# =======================================================================
# Generate the defs, the is loaded into the server by load.py
# =======================================================================
python $WK/build/nightly/build.py
if [[ $? = 1 ]] ; then
   exit 1
fi

# =======================================================================
# Kill the server
# =======================================================================
which ecflow_client
ecflow_client --version
ecflow_client --terminate=yes --port=4141

# =======================================================================
# Start server. 
# This *MUST* be a compatible version with the clients on different hosts
# If the client request is ok, and does not need any data sent back, we
# just close the socket, however older clients will treat this as an error
# since they want a response back. We use --reply_back_if_ok to keep
# compatibility with old clients
# =======================================================================
rm -rf `hostname`.4141.*

export ECF_ALLOW_OLD_CLIENT_NEW_SERVER=9
ecflow_server --port=4141 --reply_back_if_ok &
sleep 4

# =======================================================================
# load the build defs, in the server then delete generated defs.
# Make sure server is running
# =======================================================================
python $WK/build/nightly/load.py

# =======================================================================
# Start the viewer
# =======================================================================
ecflowview &
