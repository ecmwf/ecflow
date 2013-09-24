#=======================================================================
# step2.h
# Define second step for copying the logfile when on the ibm-cluster
#=======================================================================

#;;
#step_2)

if [[ $ARCH = ibm_power* ]] ; then
%include <rcp.h>

ECF_RCP

fi
exit 0

#;;
#esac

exit 0
