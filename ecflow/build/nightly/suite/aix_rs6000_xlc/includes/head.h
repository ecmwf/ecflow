%include <qsub.h>

# =================================================================
# Since we have at least one QSUB, it will mean that smssubmit
# will add @queue for us.
# The q-sub below will be translated --> # @ job_cpu_limit = 4200,3950
#
# <TODO> currently qsub.h is subset of loadleveler commands (minus the queue)
# However having it as pure QSUB commands, means that output file
# is not written, ie having # QSUB -o %ECF_JOBOUT%              does *NOT* work
# but:               having #@ output          = %ECF_JOBOUT%   works
# =================================================================
# QSUB -lT 16000
# QSUB -lMM 8000Mb

export LL_NOFILTER=1

# defines WK, ECFLOW, BOOST_ROOT, PATH
%include <trap.h>

# Load the latest c++ compiler
# use vacpp11101
use %COMPILER_VERSION%

# Hack for test, so that we can undefine ECF_RID in the tests. See file test.ecf:
export PID_RID=$ECF_RID

%manual
# ################################################################
#   If the job appears to hang in submitted state then look for
#   the .sub file. this file is written to by smssubmit
#   Ideally if using smssubmit ecflowview should also show this file if it is accesible
#
#        # less <path_to_job_file>.sub
#        # less /scratch/ma/ma0/nightly/suite/aix_xlc/build_debug/build.job1.sub
#
#   Also if you login as emos: there is a special alias called 'llqf'':
#   This will list the jobs on the load levelever
#
#   # llqf | grep ma0
# #################################################################
%end
