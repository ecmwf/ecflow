%include <qsub.h>
%include <step1.h>
# defines WK, ECFLOW, BOOST_ROOT, PATH
%include <trap.h>

# Load the latest c++ compiler and python 2.7
module unload c++/vacpp/11.1.0.9
module load %COMPILER_VERSION%
module load python/2.7.2-01

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
