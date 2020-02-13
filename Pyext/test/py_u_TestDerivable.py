#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2020 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#  code for testing derivation works
import os
import sys
import ecflow
import ecflow_test_util as Test

class MyDefs(ecflow.Defs): pass
class MySuite(ecflow.Suite): pass
class MyFamily(ecflow.Family): pass
class MyTask(ecflow.Task): pass
class MyClient(ecflow.Client): pass

if sys.version_info[0] > 2:
    class MySuite2(ecflow.Suite):
        def __init__(self, name):
            super().__init__(name)
    
    class MyFamily2(ecflow.Family):
        def __init__(self, name):
            super().__init__(name)
    
    class MyTask2(ecflow.Task):
        def __init__(self, name):
            super().__init__(name)

    n = MySuite2('s')
    print(n)

    n = MyFamily2('f')
    print(n)

    n = MyTask2('t')
    print(n)

else:
    print("Python2 ignoring derivation test")


if __name__ == "__main__":
    
    Test.print_test_start(os.path.basename(__file__))
    print("All tests pass")