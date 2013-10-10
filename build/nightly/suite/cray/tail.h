# We will look for the includes in ECF_INCLUDE ~emos/bin/cray/includes

export ECF_PORT=%ECF_PORT%
export ECF_NODE=%ECF_NODE%
export ECF_RID=%ECF_RID%
export ECF_HOME=%ECF_HOME%
export ECF_JOBOUT=%ECF_JOBOUT%
export ECF_OUT=%ECF_OUT%
export ECF_RID=%ECF_RID%

%include <rcp.h>
%include <endt.h>
