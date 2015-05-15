#!/bin/ksh
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ----------------------------------------------------------------------
# LXOP: specific ??
# ----------------------------------------------------------------------
HOST=${HOST:=$(uname -n)}
if [[ $HOST = lxop* ]]; then
# QSUB -q %QUEUE:test%
  ln -sf $(echo ${PBS_NODEFILE:=} | sed -e 's:aux:spool:').OU %ECF_JOBOUT%.running
fi

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

#==================================================================================================

export BOOST_ROOT=%BOOST_DIR%/%BOOST_VERSION%
ECFLOW_INSTALL_PATH=%ECFLOW_LAST_INSTALLED_VERSION%

#==================================================================================================

# Used to locate ecrcp,git,python and ecflow_client
export PATH=/usr/local/bin:/usr/bin:$ECFLOW_INSTALL_PATH/bin:$PATH

# ==================================================================================================
# make sure path to the python interpreter is accessible.
# Boost: This can be hard coded in site-config.jam i.e via using python Or amend the path here
# cmake: allow cmake to work out python path

if [[ %ARCH% = linux64 ]] ; then
   #
   # NOTE: /usr/local/bin provides path to 2.7 PYTHON interpreter
   #       *MAKE* sure it becomes *BEFORE* /usr/bin as that points to python 2.6
   #        This is what 'using python' will pick up and add to the include path
   #
   export PATH=/usr/local/apps/python/2.7/bin:$PATH

elif [[ %ARCH% = opensuse113  ]] ; then

   export PATH=/usr/local/apps/python/2.7/bin:$PATH
#export PATH=/usr/local/bin:$PATH

elif [[ %ARCH% = redhat  ]] ; then

   export PATH=/usr/local/apps/python/2.7/bin:$PATH

elif [[ %ARCH% = opensuse103  ]] ; then

   #
   # NOTE: /usr/local/apps/python/2.7.2-01/bin/ provides path to PYTHON interpreter
   #       This is what 'using python' will pick up and add to the include path
   #
   export PATH=/usr/local/apps/python/2.7.2-01/bin/:/usr/local/bin:$PATH

elif [[ %ARCH% = linux64intel  ]] ; then

   #
   # NOTE: /usr/local/apps/python/2.7/bin/ provides path to PYTHON interpreter
   #       This is what 'using python' will pick up and add to the include path
   #
   export PATH=/usr/local/apps/python/2.7/bin/:/usr/local/bin:$PATH

elif [[ %ARCH% = opensuse103  ]] ; then

   #
   # NOTE: /usr/local/apps/python/2.7.2-01/bin/ provides path to PYTHON interpreter
   #       This is what 'using python' will pick up and add to the include path
   #
   export PATH=/usr/local/apps/gcc/4.2.1/ILP32/bin:/usr/local/apps/python/2.7/bin:$PATH
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

   echo "environment was:"; printenv | sort
   trap 0                      # Remove the trap
   exit 0                      # End the script
}


# Trap any calls to exit and errors caught by the -e flag
trap ERROR 0


# Trap any signal that may cause the script to fail
trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15


# Enable module capability for korn shell
# The alternative is to source .profile, in ECF_JOB_CMD, however that affect all jobs
source /usr/local/apps/module/init/ksh

%MODULE_LOAD_GIT:module load git%

# last line of head.h
