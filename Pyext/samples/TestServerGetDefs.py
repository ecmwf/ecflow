#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

# This test is a simple client that, repeatedly calls gets defs
# This is used to stress test the server to determine how many defs
# can be called before the server gets overloaded

import datetime
import time, sys
from ecflow import Client   
      
def main(ci,inc = 2):
    while True:
        start_time = datetime.datetime.now()
        try:
            ci.get_server_defs() 
        except RuntimeError, e : 
            print "Error:" + str(e)

        print "get_server_defs took: " + str(datetime.datetime.now() - start_time)
        time.sleep(inc)

if __name__ == "__main__":

    numargs = len(sys.argv) - 1
    #print "numargs = " + str(numargs)
    #i = 0;
    #while i < len(sys.argv):
        #print "arg " + str(i) + ": " + sys.argv[i]
        #i =  i + 1
        
    if  numargs > 3:
        print "usage: " + sys.argv[0] + " host port  seconds_delay"
        sys.exit(1)
        
    port = "3142"
    host = "localhost"
    inc  = 1
    if numargs >= 1:  host = sys.argv[1];
    if numargs >= 2:  port = sys.argv[2]
    if numargs >= 3:  inc = int(sys.argv[3])
        
    #print "host(" + host + ") port(" + port + ") delay between get_server_defs(" + str(inc)   +")"

    ci = Client(host,port)
    ci.ping()    

    main(ci,inc)
    