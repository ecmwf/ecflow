# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

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
        except RuntimeError as e : 
            print("Error:" + str(e))

        print("get_server_defs took: " + str(datetime.datetime.now() - start_time))
        time.sleep(inc)

if __name__ == "__main__":

    numargs = len(sys.argv) - 1
    #print "numargs = " + str(numargs)
    #i = 0;
    #while i < len(sys.argv):
        #print "arg " + str(i) + ": " + sys.argv[i]
        #i =  i + 1
        
    if  numargs > 3:
        print("usage: " + sys.argv[0] + " host port  seconds_delay")
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
