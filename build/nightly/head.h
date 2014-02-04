#!/bin/ksh
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# -----------------------------------------------------------------------------------------
# **NOTE** do not use -e for file existence it WONT work on HPUX/HP_UX, use -r
# -----------------------------------------------------------------------------------------

# ============================================================================
# Defines the variables that are needed for any communication with ECF
#
# Note: We do *NOT* _export_ ECF_PORT,ECF_NODE,ECF_RID to avoid interferences
# from the ecflow regression tests. which also use ecflow_client, instead we explicitly
# set the host,port and rid on each ecflow_client call,
# i.e  --host=%ECF_NODE% --port=%ECF_PORT%
# This is only required because we are using ecflow to test ecflow
#
# The typical head.h file would just export these variables
# =============================================================================
export ECF_NAME=%ECF_NAME%    # The name of this current task
export ECF_PASS=%ECF_PASS%    # A unique password
export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
export PID_RID=$$

# =============================================================================

# Shorten normal timeout for child commands to one hour
export ECF_TIMEOUT=3600

###################################################################################

export BOOST_ROOT=%BOOST_DIR%/%BOOST_VERSION%
ECFLOW_INSTALL_PATH=%ECFLOW_LAST_INSTALLED_VERSION%

####################################################################################

# Used to locate ecrcp,python and ecflow_client
export PATH=/usr/local/bin:$ECFLOW_INSTALL_PATH/bin:$PATH

if [[ %ARCH% = redhat ]] ; then

   # where to find python
   export PATH=/usr/local/apps/python/2.7.3-02/bin:$PATH

elif [[ %ARCH% = hpux ]] ; then

   # we need pick the 'ar' in /bin rather than /usr/bin otherwise we get unsatisfied symbols
   # We pick aCC compiler from /usr/local/bin
   # On HPUX which and whence report different paths, hence beware. whence on HPUX is more reliable ?
   #
   # NOTE: /opt/python/usr/local/bin provides path to 2.5 PYTHON interpreter.
   #
   export PATH=/usr/local/apps/python/2.7/bin:/bin:/usr/bin:$PATH

elif [[ %ARCH% = opensuse103  ]] ; then

   #
   # NOTE: /usr/local/apps/python/2.7.2-01/bin/ provides path to PYTHON interpreter
   #       This is what 'using python' will pick up and add to the include path
   #
   export PATH=/usr/local/apps/gcc/4.2.1/ILP32/bin:/usr/local/apps/python/2.7.2-01/bin:$PATH
fi

# -----------------------------------------------------------------------
# Check paths especially for python
echo "ECF_INCLUDE = %ECF_INCLUDE%"
which ecflow_client
which python
ulimit

####################################################################################
# Tell ecFlow we have stated
ecflow_client --init=$PID_RID --host=%ECF_NODE% --port=%ECF_PORT%


# Defined a error hanlder
ERROR() {
   # Clear -e flag, so we don't fail
   set +e
   wait
   ecflow_client --abort=trap  --host=%ECF_NODE% --port=%ECF_PORT%

   echo "environment was:" printenv | sort
   trap 0                      # Remove the trap
   exit 0                      # End the script
}


# Trap any calls to exit and errors caught by the -e flag
trap ERROR 0


# Trap any signal that may cause the script to fail
trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15
# last line of head.h
