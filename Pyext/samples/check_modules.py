#!/usr/bin/env python2.7
import argparse  
import re     
if __name__ == "__main__":
    DESC = """Will check module are correct:
              Usage:
                ./check_modules.py --m "module load git module swap PrgEnv-cray PrgEnv-gnu " --f module_list_file
                ./check_modules.py --m "module swap PrgEnv-cray PrgEnv-gnu" --f module_list_file
                ./check_modules.py --m "module unload grib_api" --f module_list_file
            """    
    PARSER = argparse.ArgumentParser(description=DESC,formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--m',help="The module command of some sort")
    PARSER.add_argument('--f',help="Path to a file that is the output of module list")
    PARSER.add_argument('--remote_host',help="remote host")
    ARGS = PARSER.parse_args()
    #print ARGS  
    
    delimter = "module"
    module_cmds = [delimter+e for e in ARGS.m.split(delimter) if e != ""]
    #print module_cmds
    
    for mod in module_cmds:
        module_cmd = mod.split()
        #print ">>",module_cmd,"<<"
        if len(module_cmd) < 3 and module_cmd[0] != "module": 
            print "Error: Module command ",module_cmds[0]," is not correctly formed, first string must be module"
            exit(1)
        if module_cmd[1] != "swap" and module_cmd[1] != "load" and module_cmd[1] != "unload" and module_cmd[1] != "switch": 
            print "Error: Module command ",ARGS.m," is not correctly formed, second arg must be one of swap,load,unload,switch"
            exit(1)
    
    # open file,for list of modules,  
    #   Currently Loaded Modulefiles:
    #   1) version/3.2.10(default)             7) ecfs/2.2.1-rc2(new:prodn:default)
    #   2) verbose/true(default)               8) metview/4.6.4(default)
    #   3) mode/64(default)                    9) ecaccess/4.0.2(default)
    #   4) gnu/4.8.1(default)                 10) fftw/3.3.4(default)
    #   5) python/2.7.8-01(default)           11) emos/437(default)
    #   6) sms/4.4.14(default)                12) ecmwf/1.0(default)
 
    module_list = list()
    module_list_file = open(ARGS.f) 
    try:
        for line in module_list_file:
            #print line
            line = line.rstrip()
            if line.find("Currently Loaded") == 0: continue;
            tokens = re.split(r'\s|\d+\)',line)
            while '' in tokens:
                tokens.remove('')
            #print tokens
            for tok in tokens:
                module_list.append(tok)
    finally:
         module_list_file.close()  
    #print module_list
    
    for mod in module_cmds:
        module_cmd = mod.split()
        #print ">>",module_cmd,"<<"
        if module_cmd[1] == "load":
            if not any(module_cmd[2] in s for s in module_list):
                if ARGS.remote_host =="sappa" or ARGS.remote_host =="sappb":
                    if module_cmd[2] == "git": continue  # ignore, uses system git
                print "Error: package",module_cmd[2],"not found in the module list"
                exit(1)
        if module_cmd[1] == "swap" or module_cmd[1] == "switch":
            # swap could be
            # module swap PrgEnv-cray PrgEnv-gnu
            # module swap cdt/15.06
            expected_package = module_cmd[-1] # get last element
            if not any(module_cmd[-1] in s for s in module_list):
                print "Error: package",module_cmd[-1],"not found in the module list"
                exit(1)
        if module_cmd[1] == "unload":
           if any(module_cmd[2] in s for s in module_list):
                print "Error:",mod,"found in the module list"
                exit(1)

# 
# module list > module_list_file 2>&1 && less module_list_file
# FAIL:
# ./check_modules.py --m "module unload emos" --f module_list_file
# ./check_modules.py --m "module load python module load xxx" --f module_list_file
# PASS:
# ./check_modules.py --m "module load python module swap fftw/3.3.1 fftw/3.3.4" --f module_list_file
# ./check_modules.py --m "module load python module swap sms/4.4.14" --f module_list_file --remote_host sappa
