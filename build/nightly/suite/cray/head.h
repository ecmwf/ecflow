#!/bin/ksh
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# We will look for the includes in ECF_INCLUDE ~emos/bin/cray/includes
%include <qsub.h>

# defines WK, ECFLOW, BOOST_ROOT, PATH
%include <trap.h>

# ===================================================================================
# re-use standard emos headers. i.e <qsub.h>, <trap.h>.
# However these define variables that effect the test
# Hence we need to unset. See below

# Note: We do *NOT* _export_ ECF_PORT,ECF_NODE,ECF_RID to avoid interferences
# from the ecflow regression tests. which also use ecflow_client, instead we explicitly
# set the host,port and rid on each ecflow_client call,
# i.e  --host=%ECF_NODE% --port=%ECF_PORT%
# This is only required because we are using ecflow to test ecflow
unset ECF_PORT
unset ECF_NODE
unset ECF_RID
unset ECF_HOME
unset ECF_HOSTFILE
unset ECF_JOBOUT
unset ECF_OUT

# ============================================================================
# Defines the variables that are needed for any communication with ECF
# The typical head.h file would just export these variables
export ECF_NAME=%ECF_NAME%    # The name of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
export PID_RID=$$


# =============================================================================
# Shorten normal timeout for child commands to one hour
export ECF_TIMEOUT=3600

# =============================================================================
# BOOST
export BOOST_ROOT=%BOOST_DIR%/%BOOST_VERSION%

# ==========================================================================
# Access to ecflow_client
ECFLOW_INSTALL_PATH=%ECFLOW_LAST_INSTALLED_VERSION%

# -----------------------------------------------------------------------
# Check paths especially for python
echo "ECF_INCLUDE = %ECF_INCLUDE%"
which ecflow_client
which python
ulimit

# ===================================================================================
# Load the right environment, first unload all
module unload PrgEnv-cray
module unload PrgEnv-intel
module unload PrgEnv-gnu

%MODULE_LOAD%

# For gnu, we will use gnu 4.6.3
if [[ "$PE_ENV" = GNU ]] ; then
   %MODULE_LOAD_GCC%
fi

