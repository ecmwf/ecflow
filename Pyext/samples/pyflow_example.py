#pyflow Method

#pyflow is a pure python module on top of ecflow native API ;
# it is proposed to facilitate suites definitions. 
# It is not yet provided in default standard location. 
# It can be used, adding this one line to the definition script.

import sys; sys.path.append("/home/ma/emos/def/pyflow")

# Enter the following python code into a file i.e. test.py :

#!/usr/bin/env python2.7
import os
from pyflow import (Suite, Family, Task, Variable)
   
print "Creating suite definition"   
with Suite("test" ) as suite: 
 ECF_HOME = os.path.join(os.getenv("HOME"),  "course") # python vriable
 Variable("ECF_HOME", ECF_HOME)  # suite variable
 Task("t1")
print suite.ecflow_definition() 

#Then run as a python script:


#pyflow
#Using pyflow, header files are generated and placed in the expected location (ECF_INCLUDE) when the suite is deployed:
suite.deploy_suite()