#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2016 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#  code for testing derivation works
import ecflow

class MyDefs(ecflow.Defs): pass
class MySuite(ecflow.Suite): pass
class MyFamily(ecflow.Family): pass
class MyTask(ecflow.Task): pass
class MyClient(ecflow.Client): pass

if __name__ == "__main__":
    print "####################################################################"
    print "Running ecflow version " + ecflow.Client().version() + " debug build(" + str(ecflow.debug_build()) +")"
    print "####################################################################"
 
    print "All tests pass"