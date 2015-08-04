#!/bin/ksh
# cray head.h
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
# These should be unset as appropriate in test.ecf
#
# Note: We do *NOT* _export_ ECF_PORT,ECF_NODE,ECF_RID to avoid interferences
# from the ecflow regression tests. which also use ecflow_client, instead we explicitly
# set the host,port and rid on each ecflow_client call,
# i.e  --host=%ECF_NODE% --port=%ECF_PORT%
# This should be done in <trap.h> above
# This is only required because we are using ecflow to test ecflow


# ============================================================================
# Defines the variables that are needed for any communication with ECF
# The typical head.h file would just export these variables
export ECF_NAME=%ECF_NAME%    # The name of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
export PID_RID=$$
module load ecflow

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

# Export Python to pick up python 2.7
export PATH=/usr/local/apps/python/current/bin:$PATH


# ==================================================================================
# Hack for CRAY, for some reason in batch mode in can do the equivalent of mkdir -p
# The test fails for both gnu and intel compilers
# See file: ACore/test/TestFile.cpp
export ECFLOW_CRAY_BATCH=1

# ===================================================================================
# Load the right environment, default is always cray
#
module swap PrgEnv-cray %PRGENV:%


# For gnu, we will use gnu 4.6.3
if [[ "$PE_ENV" = GNU ]] ; then
   %MODULE_LOAD_GCC%
fi

# Allow cray compiler to be changed
if [[ "$PE_ENV" = CRAY ]] ; then
   module unload cce
   %MODULE_LOAD_CRAY_COMPILER%

   # This seems to interfere with the linking causing undefines
   #/opt/cray/libsci/12.1.3/CRAY/81/sandybridge/lib/libsci_cray_mp.so: undefined reference to `__ALLOCATE'
   #/opt/cray/libsci/12.1.3/CRAY/81/sandybridge/lib/libsci_cray_mp.so: undefined reference to `_GET_ENVIRONMENT_VARIABLE'
   #/opt/cray/libsci/12.1.3/CRAY/81/sandybridge/lib/libsci_cray_mp.so: undefined reference to `__MAXLOC_F08'
   module unload cray-libsci/12.1.3
fi

%MODULE_LOAD_GIT:module load git%

#
set -e;
trap ERROR 0
