# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import time
import argparse # for argument parsing  
from ecflow import Client, debug_build

def timing_decorator(function_to_time):
    def scafold(*function_args):
        # function_args is a tuple, the second argument is the suite name
        start = time.time()
        function_return = function_to_time(*function_args)
        end = time.time()
        print('%s function took %0.3f ms for %s' % (function_to_time.__name__, (end-start)*1000.0, function_args[1]))
        return function_return
    return scafold

@timing_decorator
def sync_local(ci,suite_name):
    ci.sync_local()

if __name__ == "__main__":
    
    DESC = """This test is use to show the performance of sync local on the given server
              It should show that it is quicker to register and then sync_local()
              as this limits the amount of data that needs to be down loaded from the
              server.
              Usage:
                   TestSyncLocalPerf.py --host <hostname> --port <portname>  
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--host', default="localhost",   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default="3141",   
                        help="The port on the host, defaults to 3141")
    ARGS = PARSER.parse_args()
    print(ARGS)   
     
    print("####################################################################")
    print(("Test performance of sync local using " + Client().version() + " debug build(" + str(debug_build()) +")"))
    print("####################################################################")

    CL = Client(ARGS.host, ARGS.port)
    sync_local(CL,"* All suites *")   # timing for downloading all suites
    print("")

    suites = CL.get_defs().suites
    for suite in suites:
        CL.ch_register(False,[suite.name()])
        
        sync_local(CL,suite.name()) # time this
        
        CL.ch_drop()  # drop the last registered handle
